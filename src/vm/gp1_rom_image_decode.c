#include "gp1_rom.h"
#include "gp1/gp1.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

/* Calculate stride.
 */
 
static int gp1_image_calculate_stride(int format,int w) {
  if (w<1) return -1;
  switch (format) {
    case GP1_IMAGE_FORMAT_A1: return (w+7)>>3;
    case GP1_IMAGE_FORMAT_A8: return w;
    case GP1_IMAGE_FORMAT_RGB565: return w<<1;
    case GP1_IMAGE_FORMAT_ARGB1555: return w<<1;
    case GP1_IMAGE_FORMAT_RGB888: return w*3;
    case GP1_IMAGE_FORMAT_RGBA8888: return w<<2;
  }
  return -1;
}

/* Unfilter one row.
 */
 
static inline uint8_t gp1_paeth(uint8_t a,uint8_t b,uint8_t c) {
  int p=a+b-c;
  int pa=a-p; if (pa<0) pa=-pa;
  int pb=b-p; if (pb<0) pb=-pb;
  int pc=c-p; if (pc<0) pc=-pc;
  if ((pa<=pb)&&(pa<=pc)) return a;
  if (pb<=pc) return b;
  return c;
}
 
static void gp1_image_unfilter(uint8_t *row,const uint8_t *pvrow,int c,int xstride) {
  int p=0;
  for (;p<xstride;p++) row[p]+=pvrow[p];
  for (;p<c;p++) row[p]+=gp1_paeth(row[p-xstride],pvrow[p],pvrow[p-xstride]);
}
 
static void gp1_image_unfilter_sub(uint8_t *row,int c,int xstride) {
  int p=xstride;
  for (;p<c;p++) row[p]+=row[p-xstride];
}

/* Decode in context.
 */
 
static int gp1_image_decode(uint8_t *dst,int dststride,const struct gp1_imag *imag,z_stream *z) {
  uint8_t *pvrow=0;
  int y=0;
  int xstride=gp1_image_calculate_stride(imag->format,1);
  if (xstride<1) return -1;
  z->next_in=(Bytef*)imag->v;
  z->avail_in=imag->c;
  while (y<imag->h) {
    z->next_out=(Bytef*)dst;
    z->avail_out=dststride;
    int panic=5;
    while (z->avail_out) {
      int err=inflate(z,Z_NO_FLUSH);
      if (err<0) {
        if (inflate(z,Z_SYNC_FLUSH)<0) return -1;
      }
      if (!--panic) return -1;
    }
    if (pvrow) gp1_image_unfilter(dst,pvrow,dststride,xstride);
    else gp1_image_unfilter_sub(dst,dststride,xstride);
    pvrow=dst;
    dst+=dststride;
    y++;
  }
  return 0;
}

/* Decode image, main entry point.
 */
 
int gp1_rom_image_decode(
  void *dstpp,
  uint16_t *w,uint16_t *h,uint8_t *fmt,
  const struct gp1_rom *rom,
  uint32_t imageid
) {
  int p=gp1_rom_imag_search(rom,imageid);
  if (p<0) return -1;
  const struct gp1_imag *imag=rom->imagv+p;
  
  if (w) *w=imag->w;
  if (h) *h=imag->h;
  if (fmt) *fmt=imag->format;
  
  int stride=gp1_image_calculate_stride(imag->format,imag->w);
  if (stride<1) return -1;
  int dstc=stride*imag->h;
  void *dst=malloc(dstc);
  if (!dst) return -1;
  
  // Simpler case for empty images.
  if (!imag->c) {
    memset(dst,0,dstc);
    *(void**)dstpp=dst;
    return dstc;
  }
  
  // Data exists, so decompress and unfilter it.
  z_stream z={0};
  if (inflateInit(&z)<0) {
    free(dst);
    return -1;
  }
  
  if (gp1_image_decode(dst,stride,imag,&z)<0) {
    free(dst);
    inflateEnd(&z);
    return -1;
  }
  inflateEnd(&z);
  *(void**)dstpp=dst;
  return dstc;
}
