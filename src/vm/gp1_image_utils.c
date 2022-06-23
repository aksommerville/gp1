#include "gp1_vm_render.h"
#include "gp1/gp1.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Calculate stride.
 */
 
int gp1_image_calculate_stride(int format,int w) {
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

/* Generic image access.
 * It's not optimal, but we are going to split read and write, and have all conversion go thru RGBA as an intermediary.
 * This lets us write n*2 instead of n**2 functions, in exchange for a more expensive conversion.
 * I figure conversion shouldn't come up often enough to care about.
 */
 
typedef uint32_t (*gp1_pxrd_fn)(const uint8_t *src,int x);
typedef void (*gp1_pxwr_fn)(uint8_t *dst,int x,uint32_t rgba);

static uint32_t gp1_pxrd_a1(const uint8_t *src,int x) {
  if (src[x>>3]&(0x80>>(x&7))) return 0xffffffff;
  else return 0x000000ff;
}

static void gp1_pxwr_a1(uint8_t *dst,int x,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8;
  if (r+g+b>=384) dst[x>>3]|=(0x80>>(x&7));
  else dst[x>>3]&=~(0x80>>(x&7));
}

static uint32_t gp1_pxrd_a8(const uint8_t *src,int x) {
  uint8_t y=src[x];
  return (y<<24)|(y<<16)|(y<<8)|0xff;
}

static void gp1_pxwr_a8(uint8_t *dst,int x,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8;
  dst[x]=(r+g+b)/3;
}

static uint32_t gp1_pxrd_rgb565(const uint8_t *src,int x) {
  x<<=1;
  uint8_t r=(src[x]&0xf8); r|=r>>5;
  uint8_t g=(src[x]<<5)|((src[x+1]&0xe0)>>3); g|=g>>6;
  uint8_t b=(src[x+1]<<3); b|=b>>5;
  return (r<<24)|(g<<16)|(b<<8)|0xff;
}

static void gp1_pxwr_rgb565(uint8_t *dst,int x,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8;
  x<<=1;
  dst[x]=(r&0xf8)|(g>>5);
  dst[x+1]=((g<<3)&0xe0)|(b>>3);
}

static uint32_t gp1_pxrd_argb1555(const uint8_t *src,int x) {
  x<<=1;
  uint8_t a=(src[x]&0x80)?0xff:0x00;
  uint8_t r=(src[x]<<1)&0xf8; r|=r>>5;
  uint8_t g=(src[x]<<6)|((src[x+1]&0xe0)>>2); g|=g>>6;
  uint8_t b=(src[x+1]<<3); b|=b>>5;
  return (r<<24)|(g<<16)|(b<<8)|a;
}

static void gp1_pxwr_argb1555(uint8_t *dst,int x,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
  x<<=1;
  dst[x]=(a&0x80)|((r>>1)&0xec)|(g>>6);
  dst[x+1]=((g<<2)&0xe0)|(b>>3);
}

static uint32_t gp1_pxrd_rgb888(const uint8_t *src,int x) {
  x*=3;
  return (src[x]<<24)|(src[x+1]<<16)|(src[x+2]<<8)|0xff;
}

static void gp1_pxwr_rgb888(uint8_t *dst,int x,uint32_t rgba) {
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8;
  x*=3;
  dst[x]=r;
  dst[x+1]=g;
  dst[x+2]=b;
}

static uint32_t gp1_pxrd_rgba8888(const uint8_t *src,int x) {
  x<<=2;
  return (src[x]<<24)|(src[x+1]<<16)|(src[x+2]<<8)|src[x+3];
}

static void gp1_pxwr_rgba8888(uint8_t *dst,int x,uint32_t rgba) {
  x<<=2;
  dst[x]=rgba>>24;
  dst[x+1]=rgba>>16;
  dst[x+2]=rgba>>8;
  dst[x+3]=rgba;
}

static gp1_pxrd_fn gp1_get_pxrd(int format) {
  switch (format) {
    case GP1_IMAGE_FORMAT_A1: return gp1_pxrd_a1;
    case GP1_IMAGE_FORMAT_A8: return gp1_pxrd_a8;
    case GP1_IMAGE_FORMAT_RGB565: return gp1_pxrd_rgb565;
    case GP1_IMAGE_FORMAT_ARGB1555: return gp1_pxrd_argb1555;
    case GP1_IMAGE_FORMAT_RGB888: return gp1_pxrd_rgb888;
    case GP1_IMAGE_FORMAT_RGBA8888: return gp1_pxrd_rgba8888;
  }
  return 0;
}

static gp1_pxwr_fn gp1_get_pxwr(int format) {
  switch (format) {
    case GP1_IMAGE_FORMAT_A1: return gp1_pxwr_a1;
    case GP1_IMAGE_FORMAT_A8: return gp1_pxwr_a8;
    case GP1_IMAGE_FORMAT_RGB565: return gp1_pxwr_rgb565;
    case GP1_IMAGE_FORMAT_ARGB1555: return gp1_pxwr_argb1555;
    case GP1_IMAGE_FORMAT_RGB888: return gp1_pxwr_rgb888;
    case GP1_IMAGE_FORMAT_RGBA8888: return gp1_pxwr_rgba8888;
  }
  return 0;
}

/* Convert image.
 */

void *gp1_convert_image(int dstfmt,const void *src,int w,int h,int srcfmt) {
  if (h<1) return 0;
  int srcstride=gp1_image_calculate_stride(srcfmt,w);
  if (srcstride<1) return 0;
  int dststride=gp1_image_calculate_stride(dstfmt,w);
  if (dststride<1) return 0;
  if (dststride>INT_MAX/h) return 0;
  gp1_pxrd_fn pxrd=gp1_get_pxrd(srcfmt);
  if (!pxrd) return 0;
  gp1_pxwr_fn pxwr=gp1_get_pxwr(dstfmt);
  if (!pxwr) return 0;
  void *dst=malloc(dststride*h);
  if (!dst) return 0;
  uint8_t *dstrow=dst;
  const uint8_t *srcrow=src;
  for (;h-->0;dstrow+=dststride,srcrow+=srcstride) {
    int x=0; for (;x<w;x++) pxwr(dstrow,x,pxrd(srcrow,x));
  }
  return dst;
}
