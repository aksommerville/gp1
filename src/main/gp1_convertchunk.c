#include "gp1_cli.h"
#include "png.h"
#include "gp1/gp1.h"
#include "io/gp1_serial.h"
#include <zlib.h>

/* Guess type.
 */
 
static uint32_t gp1_guess_chunk_type(const void *src,int srcc,const char *path) {
  
  // If the source path has a known suffix, trust it blindly.
  const char *suffix=gp1_get_suffix(path);
  if (!strcmp(suffix,"png")) return GP1_CHUNK_TYPE('i','m','A','G');
  if (!strcmp(suffix,"bmp")) return GP1_CHUNK_TYPE('i','m','A','G');
  if (!strcmp(suffix,"gif")) return GP1_CHUNK_TYPE('i','m','A','G');
  if (!strcmp(suffix,"jpeg")) return GP1_CHUNK_TYPE('i','m','A','G');
  if (!strcmp(suffix,"webp")) return GP1_CHUNK_TYPE('i','m','A','G');
  if (!strcmp(suffix,"mid")) return GP1_CHUNK_TYPE('s','o','N','G');
  if (!strcmp(suffix,"aucfg")) return GP1_CHUNK_TYPE('a','u','D','O');
  if (!strcmp(suffix,"data")) return GP1_CHUNK_TYPE('d','a','T','A');
  if (!strcmp(suffix,"bin")) return GP1_CHUNK_TYPE('d','a','T','A');
  
  // Check signatures.
  if ((srcc>=8)&&!memcmp(src,"\x89PNG\r\n\x1a\n",8)) return GP1_CHUNK_TYPE('i','m','A','G');
  if ((srcc>=6)&&!memcmp(src,"GIF87a",6)) return GP1_CHUNK_TYPE('i','m','A','G');
  if ((srcc>=6)&&!memcmp(src,"GIF89a",6)) return GP1_CHUNK_TYPE('i','m','A','G');
  if ((srcc>=8)&&!memcmp(src,"MThd\0\0\0\6",8)) return GP1_CHUNK_TYPE('s','o','N','G');
  
  return 0;
}

/* Resource ID and language from path.
 */
 
static void gp1_guess_resid_and_language(uint32_t *resid,uint16_t *lang,const char *path) {
  const char *base=gp1_get_basename(path);
  int pfxlen=0;
  while (base[pfxlen]) {
    if ((base[pfxlen]>='0')&&(base[pfxlen]<='9')) { pfxlen++; continue; }
    if ((base[pfxlen]>='a')&&(base[pfxlen]<='a')) { pfxlen++; continue; }
    if ((base[pfxlen]>='A')&&(base[pfxlen]<='Z')) { pfxlen++; continue; }
    break;
  }
  if ((pfxlen>2)&&(base[pfxlen-1]>='a')&&(base[pfxlen-1]<='z')&&(base[pfxlen-2]>='a')&&(base[pfxlen-2]<='z')) {
    pfxlen-=2;
    *lang=(base[pfxlen]<<8)|base[pfxlen+1];
  }
  int n;
  if (gp1_int_eval(&n,base,pfxlen)>=1) *resid=n;
}

/* Determine output format for imAG from PNG file.
 */
 
static int gp1_guess_imAG_format_for_png(struct png_image *png,struct gp1_config *config,const char *srcpath) {

  // If a format is encoded in the file name, it trumps all.
  if (srcpath) {
    const char *token=0;
    int p=0,tokenc=0;
    for (;srcpath[p];p++) {
      if (srcpath[p]=='/') {
        token=0;
        tokenc=0;
      } else if (srcpath[p]=='.') {
        int format=gp1_image_format_eval(token,tokenc);
        if (format>=0) return format;
        token=srcpath+p+1;
        tokenc=0;
      } else if (token) {
        tokenc++;
      }
    }
  }
  
  // Examine pixels.
  png_pxrd_fn pxrd=png_get_pxrd(png->depth,png->colortype);
  if (pxrd) {
    int has_transparent=0,has_opaque=0,has_alpha=0;
    int has_rgb=0,has_black=0,has_white=0,has_gray=0;
    const uint8_t *row=png->pixels;
    int yi=png->h;
    for (;yi-->0;row+=png->stride) {
      int x=0; for (;x<png->w;x++) {
        uint32_t rgba=pxrd(row,x);
        uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
        
        if (!a) {
          has_transparent=1;
          goto _ready_; // ...and the only valid option will be RGBA8888 (not sure i'm keeping this rule)
          continue; // ...and don't examine the color channels
        } else if (a==0xff) {
          has_opaque=1;
        } else {
          has_alpha=1;
          goto _ready_; // ...and the only valid option will be RGBA8888
        }
        
        if ((r==g)&&(g==b)) {
          if (r==0x00) has_black=1;
          else if (r==0xff) has_white=1;
          else has_gray=1;
        } else {
          has_rgb=1;
        }
      }
    }
   _ready_:;
    // Having examined every pixel, choose a sensible format.
    if (has_alpha) return GP1_IMAGE_FORMAT_RGBA8888;
    if (has_transparent) return GP1_IMAGE_FORMAT_RGBA8888;
    if (has_rgb) return GP1_IMAGE_FORMAT_RGB888;
    if (has_gray) return GP1_IMAGE_FORMAT_A8;
    return GP1_IMAGE_FORMAT_A1;
  }

  // Guess based on colortype. It is unlikely to reach this.
  switch (png->colortype) {
    case 0: return GP1_IMAGE_FORMAT_A8;
    case 2: return GP1_IMAGE_FORMAT_RGB888;
    case 3: return GP1_IMAGE_FORMAT_RGB888;
    case 4: return GP1_IMAGE_FORMAT_RGBA8888;
    case 6: return GP1_IMAGE_FORMAT_RGBA8888;
  }
  return GP1_IMAGE_FORMAT_RGBA8888;
}

/* Convert one row of pixels from PNG to imAG.
 */
 
static void gp1_convert_pixel_row(
  uint8_t *dst,int dstformat,
  const uint8_t *src,png_pxrd_fn pxrd,
  int w
) {
  int x=0;
  switch (dstformat) {

    case GP1_IMAGE_FORMAT_A1: {
        memset(dst,0,(w+7)>>3);
        uint8_t mask=0x80;
        for (;x<w;x++) {
          uint32_t rgba=pxrd(src,x);
          uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
          if (r+g+b>=384) (*dst)|=mask;
          if (!(mask>>=1)) {
            mask=0x80;
            dst++;
          }
        }
      } return;

    case GP1_IMAGE_FORMAT_A8: {
        for (;x<w;x++,dst+=1) {
          uint32_t rgba=pxrd(src,x);
          uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
          dst[0]=(r+g+b)/3;
        }
      } return;

    case GP1_IMAGE_FORMAT_RGB565: {
        for (;x<w;x++,dst+=2) {
          uint32_t rgba=pxrd(src,x);
          uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
          dst[0]=(r&0xf8)|(g>>5);
          dst[1]=((g<<3)&0xe0)|(b>>3);
        }
      } return;

    case GP1_IMAGE_FORMAT_ARGB1555: {
        for (;x<w;x++,dst+=2) {
          uint32_t rgba=pxrd(src,x);
          uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
          dst[0]=(a&0x80)|((r>>1)&0x7c)|(g>>6);
          dst[1]=((g<<2)&0xe0)|(b>>3);
        }
      } return;

    case GP1_IMAGE_FORMAT_RGB888: {
        for (;x<w;x++,dst+=3) {
          uint32_t rgba=pxrd(src,x);
          dst[0]=rgba>>24;
          dst[1]=rgba>>16;
          dst[2]=rgba>>8;
        }
      } return;

    case GP1_IMAGE_FORMAT_RGBA8888: {
        for (;x<w;x++,dst+=4) {
          uint32_t rgba=pxrd(src,x);
          dst[0]=rgba>>24;
          dst[1]=rgba>>16;
          dst[2]=rgba>>8;
          dst[3]=rgba;
        }
      } return;

  }
}

/* Apply Paeth predictor.
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
 
static void gp1_filter_pixel_row(
  uint8_t *dst,
  const uint8_t *src,
  const uint8_t *prv,
  int c,
  int xstride
) {
  memcpy(dst,prv,xstride);
  int i=xstride;
  for (;i<c;i++) {
    dst[i]=src[i]-gp1_paeth(src[i-xstride],prv[i],prv[i-xstride]);
  }
}

/* Convert pixels, with established context.
 */
 
static int gp1_convert_pixels(
  struct gp1_encoder *dst,
  int dstformat,
  int dststride,
  uint8_t *buf, // dststride*3
  z_stream *z,
  struct png_image *png
) {
  png_pxrd_fn pxrd=png_get_pxrd(png->depth,png->colortype);
  if (!pxrd) return -1;
  int xstride=gp1_stride_for_image_format(dstformat,1);
  if (xstride<1) return -1;
  uint8_t *row=buf,*pvrow=buf+dststride,*filtered=buf+dststride*2;
  int yi=png->h;
  const uint8_t *srcrow=png->pixels;
  for (;yi-->0;srcrow+=png->stride) {
  
    gp1_convert_pixel_row(row,dstformat,srcrow,pxrd,png->w);
    gp1_filter_pixel_row(filtered,row,pvrow,dststride,xstride);
    
    z->next_in=(Bytef*)filtered;
    z->avail_in=dststride;
    while (z->avail_in>0) {
      if (gp1_encoder_require(dst,1)<0) return -1;
      z->next_out=(Bytef*)dst->v+dst->c;
      z->avail_out=dst->a-dst->c;
      int ao0=z->avail_out;
      if (deflate(z,Z_NO_FLUSH)<0) {
        if (deflate(z,Z_SYNC_FLUSH)<0) return -1;
      }
      int addc=ao0-z->avail_out;
      dst->c+=addc;
    }
    
    uint8_t *tmp=row;
    row=pvrow;
    pvrow=tmp;
  }
  while (1) {
    if (gp1_encoder_require(dst,1024)<0) return -1;
    z->next_out=(Bytef*)(dst->v+dst->c);
    z->avail_out=dst->a-dst->c;
    int ao0=z->avail_out;
    int err=deflate(z,Z_FINISH);
    if (err<0) return -1;
    int addc=ao0-z->avail_out;
    dst->c+=addc;
    if (err==Z_STREAM_END) break;
  }
  return 0;
}

/* imAG from PNG.
 * Emits Format and Pixels. Caller should produce the rest of the header.
 */
 
static int gp1_convert_imAG_png(struct gp1_encoder *dst,struct png_image *png,struct gp1_config *config,const char *srcpath) {
  
  if ((png->w>0xffff)||(png->h>0xffff)) {
    fprintf(stderr,"%s: Image too large (%dx%d, limit 65535x65535)\n",srcpath,png->w,png->h);
    return -2;
  }
  if (
    (gp1_encode_int16be(dst,png->w)<0)||
    (gp1_encode_int16be(dst,png->h)<0)
  ) return -1;
  
  // Select output format.
  int format=gp1_guess_imAG_format_for_png(png,config,srcpath);
  if (gp1_encode_int8(dst,format)<0) return -1;
  
  // Convert, filter, and compress pixels.
  int dststride=gp1_stride_for_image_format(format,png->w);
  if (dststride<1) return -1;
  uint8_t *buf=calloc(dststride,3);
  if (!buf) return -1;
  z_stream z={0};
  if (deflateInit(&z,Z_BEST_COMPRESSION)<0) {
    free(buf);
    return -1;
  }
  int err=gp1_convert_pixels(dst,format,dststride,buf,&z,png);
  free(buf);
  deflateEnd(&z);
  return err;
}

/* imAG from anything.
 */
 
static int gp1_convert_chunk_imAG(
  struct gp1_encoder *dst,
  struct gp1_config *config,
  const void *src,int srcc,
  const char *srcpath,
  uint32_t resid,
  uint16_t lang
) {

  // Regardless of the input format, we can emit resource and language IDs directly.
  if (
    (gp1_encode_int32be(dst,resid)<0)||
    (gp1_encode_int16be(dst,lang)<0)
  ) return -1;

  // PNG is my preferred format, and may well be the only one we support.
  if ((srcc>=8)&&!memcmp(src,"\x89PNG\r\n\x1a\n",8)) {
    struct png_image *png=png_decode(src,srcc);
    if (!png) {
      fprintf(stderr,"%s: Failed to decode PNG file.\n",srcpath);
      return -2;
    }
    int err=gp1_convert_imAG_png(dst,png,config,srcpath);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s: Failed to convert PNG file.\n",srcpath);
      png_image_del(png);
      return -2;
    }
    png_image_del(png);
    return 0;
  }
  
  fprintf(stderr,"%s: Unsupported file format. [%s:%d]\n",srcpath,__FILE__,__LINE__);
  return -2;
}

/* daTA and soNG: 4-byte ID, then verbatim payload.
 */
 
static int gp1_convert_chunk_id32(
  struct gp1_encoder *dst,
  struct gp1_config *config,
  const void *src,int srcc,
  const char *srcpath,
  uint32_t chunktype,
  uint32_t resid
) {
  if (
    (gp1_encode_int32be(dst,chunktype)<0)||
    (gp1_encode_int32be(dst,resid)<0)||
    (gp1_encode_raw(dst,src,srcc)<0)
  ) return -1;
  return 0;
}

/* Unknown chunk.
 */
 
static int gp1_convert_chunk_unknown(
  struct gp1_encoder *dst,
  struct gp1_config *config,
  const void *src,int srcc,
  const char *srcpath,
  uint32_t chunktype,
  uint32_t resid,
  uint16_t lang
) {
  if ((resid!=0)||(lang!=0x2020)) {
    fprintf(stderr,"%s: Generic chunks must not have resource ID or language.\n",srcpath);
    return -2;
  }
  if (
    (gp1_encode_int32be(dst,chunktype)<0)||
    (gp1_encode_raw(dst,src,srcc)<0)
  ) return -1;
  return 0;
}

/* Convert chunk, main entry point.
 */
 
int gp1_convert_chunk(void *dstpp,struct gp1_config *config,const void *src,int srcc,const char *srcpath) {
  
  // Chunk type must be knowable.
  uint32_t chunktype=gp1_guess_chunk_type(src,srcc,srcpath);
  if (!chunktype) {
    fprintf(stderr,"%s: Unable to guess chunk type from file name or content.\n",srcpath);
    return -1;
  }
  
  // Resource ID and language. Not applicable for all types.
  uint32_t resid=0;
  uint16_t lang=0x2020;
  gp1_guess_resid_and_language(&resid,&lang,srcpath);
  if (!resid) {
    if (
      (chunktype==GP1_CHUNK_TYPE('i','m','A','G'))||
      (chunktype==GP1_CHUNK_TYPE('s','o','N','G'))||
      (chunktype==GP1_CHUNK_TYPE('d','a','T','A'))||
    0) {
      fprintf(stderr,"%s: File name must begin with decimal resource ID.\n",srcpath);
      return -1;
    }
  }
  
  struct gp1_encoder dst={0};
  if (gp1_encode_int32be(&dst,chunktype)<0) return -1;
  int err=0;
  switch (chunktype) {
    case GP1_CHUNK_TYPE('i','m','A','G'): err=gp1_convert_chunk_imAG(&dst,config,src,srcc,srcpath,resid,lang); break;
    case GP1_CHUNK_TYPE('d','a','T','A'): err=gp1_convert_chunk_id32(&dst,config,src,srcc,srcpath,chunktype,resid); break;
    case GP1_CHUNK_TYPE('s','o','N','G'): err=gp1_convert_chunk_id32(&dst,config,src,srcc,srcpath,chunktype,resid); break;
    default: err=gp1_convert_chunk_unknown(&dst,config,src,srcc,srcpath,chunktype,resid,lang); break;
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to convert chunk.\n",srcpath);
    gp1_encoder_cleanup(&dst);
    return -1;
  }
  
  *(void**)dstpp=dst.v;
  return dst.c;
}
