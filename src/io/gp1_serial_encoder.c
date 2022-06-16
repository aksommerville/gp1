#include "gp1_serial.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

/* Cleanup.
 */
 
void gp1_encoder_cleanup(struct gp1_encoder *encoder) {
  if (encoder->v) free(encoder->v);
}

/* Grow buffer.
 */

int gp1_encoder_require(struct gp1_encoder *encoder,int addc) {
  if (addc<1) return 0;
  if (encoder->c>INT_MAX-addc) return -1;
  int na=encoder->c+addc;
  if (na<=encoder->a) return 0;
  if (na<INT_MAX-256) na=(na+256)&~255;
  void *nv=realloc(encoder->v,na);
  if (!nv) return -1;
  encoder->v=nv;
  encoder->a=na;
  return 0;
}

/* Replace arbitrary content.
 */
 
int gp1_encoder_replace(struct gp1_encoder *encoder,int p,int c,const void *src,int srcc) {
  if ((p<0)||(c<0)||(p>encoder->c-c)||(srcc<0)) return -1;
  if (srcc!=c) {
    if (gp1_encoder_require(encoder,srcc-c)<0) return -1;
    memmove(encoder->v+p+srcc,encoder->v+p+c,encoder->c-c-p);
    encoder->c+=srcc-c;
  }
  if (srcc) {
    if (src) memcpy(encoder->v+p,src,srcc);
    else memset(encoder->v+p,0,srcc);
  }
  return 0;
}

/* Fixed-format integers.
 */
 
int gp1_encode_int8(struct gp1_encoder *encoder,int src) {
  if (gp1_encoder_require(encoder,1)<0) return -1;
  uint8_t *dst=encoder->v+encoder->c;
  dst[0]=src;
  encoder->c+=1;
  return 0;
}
 
int gp1_encode_int16be(struct gp1_encoder *encoder,int src) {
  if (gp1_encoder_require(encoder,2)<0) return -1;
  uint8_t *dst=encoder->v+encoder->c;
  dst[0]=src>>8;
  dst[1]=src;
  encoder->c+=2;
  return 0;
}

int gp1_encode_int32be(struct gp1_encoder *encoder,int src) {
  if (gp1_encoder_require(encoder,4)<0) return -1;
  uint8_t *dst=encoder->v+encoder->c;
  dst[0]=src>>24;
  dst[1]=src>>16;
  dst[2]=src>>8;
  dst[3]=src;
  encoder->c+=4;
  return 0;
}

/* Encode raw data.
 */

int gp1_encode_raw(struct gp1_encoder *encoder,const void *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }
  if (gp1_encoder_require(encoder,srcc)<0) return -1;
  memcpy(encoder->v+encoder->c,src,srcc);
  encoder->c+=srcc;
  return 0;
}

/* Insert chunk length in the past.
 */
 
int gp1_encode_int32belen(struct gp1_encoder *encoder,int lenp) {
  if ((lenp<0)||(lenp>=encoder->c)) return -1;
  int len=encoder->c-lenp;
  uint8_t tmp[4]={len>>24,len>>16,len>>8,len};
  return gp1_encoder_replace(encoder,lenp,0,tmp,sizeof(tmp));
}
