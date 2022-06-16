#include "gp1_cli.h"
#include "gp1/gp1.h"

/* Basename.
 */
 
const char *gp1_get_basename(const char *path) {
  if (!path) return "";
  const char *base="";
  for (;*path;path++) {
    if (*path=='/') base=path+1;
  }
  return base;
}

/* Path suffix.
 */
 
const char *gp1_get_suffix(const char *path) {
  if (!path) return "";
  const char *suffix="";
  for (;*path;path++) {
    if (*path=='/') suffix="";
    else if (*path=='.') suffix=path+1;
  }
  return suffix;
}

/* Validate chunk type.
 */
 
int gp1_valid_chunk_type(uint32_t type) {
  int i=4; while (i-->0) {
    uint8_t b=type;
    if ((b<0x20)||(b>0x7e)) return 0;
  }
  return 1;
}

/* Image format from string.
 */
 
int gp1_image_format_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char norm[8];
  if (srcc>sizeof(norm)) return -1;
  int i=srcc; while (i-->0) {
    if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
    else norm[i]=src[i];
  }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(norm,#tag,srcc)) return GP1_IMAGE_FORMAT_##tag;
  GP1_FOR_EACH_IMAGE_FORMAT
  #undef _
  return -1;
}

/* Stride for image format.
 */
 
int gp1_stride_for_image_format(int format,int w) {
  if (w<1) return 0;
  switch (format) {
    case GP1_IMAGE_FORMAT_A1: return (w+7)>>3;
    case GP1_IMAGE_FORMAT_A8: return w;
    case GP1_IMAGE_FORMAT_RGB565: return w<<1;
    case GP1_IMAGE_FORMAT_ARGB1555: return w<<1;
    case GP1_IMAGE_FORMAT_RGB888: return w*3;
    case GP1_IMAGE_FORMAT_RGBA8888: return w<<2;
  }
  return 0;
}
