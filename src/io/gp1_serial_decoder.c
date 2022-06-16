#include "gp1_serial.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define DECODERP (void*)((uint8_t*)decoder->src+decoder->srcp)

/* Fixed-format integers.
 */

int gp1_decode_int32be(int *dst,struct gp1_decoder *decoder) {
  if (gp1_decoder_remaining(decoder)<4) return -1;
  uint8_t *src=DECODERP;
  *dst=(src[0]<<24)|(src[1]<<16)|(src[2]<<8)|src[3];
  decoder->srcp+=4;
  return 0;
}

/* Raw chunks with optional leading length.
 */

int gp1_decode_raw(void *dstpp,struct gp1_decoder *decoder,int c) {
  if (c<0) return -1;
  if (c>gp1_decoder_remaining(decoder)) return -1;
  if (dstpp) *(void**)dstpp=DECODERP;
  decoder->srcp+=c;
  return c;
}

int gp1_decode_int32belen(void *dstpp,struct gp1_decoder *decoder) {
  int c,p0=decoder->srcp;
  if (gp1_decode_int32be(&c,decoder)<0) return -1;
  if (gp1_decode_raw(dstpp,decoder,c)<0) { decoder->srcp=p0; return -1; }
  return c;
}

/* Line of text.
 */

int gp1_decode_line(void *dstpp,struct gp1_decoder *decoder) {
  const char *line=DECODERP;
  int linec=0;
  while ((decoder->srcp<decoder->srcc)&&(line[linec++]!=0x0a)) ;
  if (dstpp) *(const void**)dstpp=line;
  decoder->srcp+=linec;
  return linec;
}

/* Return one JSON expression verbatim.
 */

int gp1_decode_json_raw(void *dstpp,struct gp1_decoder *decoder) {
  if (decoder->jsonctx<0) return -1;
  while ((decoder->srcp<decoder->srcc)&&(*(char*)DECODERP<=0x20)) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return -1;
  int dstc=gp1_json_measure(DECODERP,decoder->srcc-decoder->srcp);
  if (dstc<1) return decoder->jsonctx=-1;
  if (dstpp) *(void**)dstpp=DECODERP;
  decoder->srcp+=dstc;
  return dstc;
}

/* Begin JSON structure.
 */

int gp1_decode_json_array_start(struct gp1_decoder *decoder) {
  if (decoder->jsonctx<0) return -1;
  while ((decoder->srcp<decoder->srcc)&&(*(char*)DECODERP<=0x20)) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return -1;
  if (*(char*)DECODERP!='[') return -1;
  int pvctx=decoder->jsonctx;
  decoder->srcp++;
  decoder->jsonctx='[';
  return pvctx;
}
  
int gp1_decode_json_object_start(struct gp1_decoder *decoder) {
  if (decoder->jsonctx<0) return -1;
  while ((decoder->srcp<decoder->srcc)&&(*(char*)DECODERP<=0x20)) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return -1;
  if (*(char*)DECODERP!='{') return -1;
  int pvctx=decoder->jsonctx;
  decoder->srcp++;
  decoder->jsonctx='{';
  return pvctx;
}

/* End JSON structure.
 */

int gp1_decode_json_array_end(struct gp1_decoder *decoder,int jsonctx) {
  if (decoder->jsonctx!='[') return -1;
  if (jsonctx<0) return decoder->jsonctx=-1;
  while ((decoder->srcp<decoder->srcc)&&(*(char*)DECODERP<=0x20)) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return -1;
  if (*(char*)DECODERP!=']') return -1;
  decoder->srcp++;
  decoder->jsonctx=jsonctx;
  return 0;
}

int gp1_decode_json_object_end(struct gp1_decoder *decoder,int jsonctx) {
  if (decoder->jsonctx!='{') return -1;
  if (jsonctx<0) return decoder->jsonctx=-1;
  while ((decoder->srcp<decoder->srcc)&&(*(char*)DECODERP<=0x20)) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return -1;
  if (*(char*)DECODERP!='}') return -1;
  decoder->srcp++;
  decoder->jsonctx=jsonctx;
  return 0;
}

/* Next member of JSON structure.
 * We'll skip commas like whitespace.
 * That means we are way more tolerant of comma-related errors than the spec requires.
 */

int gp1_decode_json_next(void *kpp,struct gp1_decoder *decoder) {
  switch (decoder->jsonctx) {
  
    case '[': {
        if (kpp) return decoder->jsonctx=-1;
        while ((decoder->srcp<decoder->srcc)&&((*(unsigned char*)DECODERP<=0x20)||(*(char*)DECODERP==','))) decoder->srcp++;
        if (decoder->srcp>=decoder->srcc) return decoder->jsonctx=-1;
        if (*(char*)DECODERP==']') return 0;
        return 1;
      }
      
    case '{': {
        if (!kpp) return decoder->jsonctx=-1;
        while ((decoder->srcp<decoder->srcc)&&((*(unsigned char*)DECODERP<=0x20)||(*(char*)DECODERP==','))) decoder->srcp++;
        if (decoder->srcp>=decoder->srcc) return decoder->jsonctx=-1;
        const char *k=DECODERP;
        if (k[0]=='}') return 0;
        int flags=0;
        int kc=gp1_string_measure(k,decoder->srcc-decoder->srcp,&flags);
        if (kc<1) return decoder->jsonctx=-1;
        decoder->srcp+=kc;
        while ((decoder->srcp<decoder->srcc)&&((*(unsigned char*)DECODERP<=0x20)||(*(char*)DECODERP==':'))) decoder->srcp++;
        if (decoder->srcp>=decoder->srcc) return decoder->jsonctx=-1;
        if ((flags&GP1_STRING_SIMPLE)&&(kc>2)) {
          *(const void**)kpp=k+1;
          return kc-2;
        } else {
          *(const void**)kpp=k;
          return kc;
        }
      }
      
  }
  return -1;
}

/* JSON structures, callback form.
 */
 
int gp1_decode_json_array(
  struct gp1_decoder *decoder,
  int (*cb)(struct gp1_decoder *decoder,int p,void *userdata),
  void *userdata
) {
  int pvctx=gp1_decode_json_array_start(decoder);
  if (pvctx<0) return -1;
  int p=0;
  while (gp1_decode_json_next(0,decoder)>0) {
    int err=cb(decoder,p,userdata);
    if (err<0) {
      decoder->jsonctx=-1;
      return err;
    }
    p++;
  }
  if (gp1_decode_json_array_end(decoder,pvctx)<0) return -1;
  return 0;
}

int gp1_decode_json_object(
  struct gp1_decoder *decoder,
  int (*cb)(struct gp1_decoder *decoder,const char *k,int kc,void *userdata),
  void *userdata
) {
  int pvctx=gp1_decode_json_object_start(decoder);
  if (pvctx<0) return -1;
  const char *k;
  int kc;
  while ((kc=gp1_decode_json_next(&k,decoder))>0) {
    int err=cb(decoder,k,kc,userdata);
    if (err<0) {
      decoder->jsonctx=-1;
      return err;
    }
  }
  if (gp1_decode_json_object_end(decoder,pvctx)<0) return -1;
  return 0;
}

/* Decode JSON int.
 */
 
int gp1_decode_json_int(int *dst,struct gp1_decoder *decoder) {
  const char *src=0;
  int srcc=gp1_decode_json_raw(&src,decoder);
  if (srcc<1) return -1;
  if (gp1_json_as_int(dst,src,srcc)<0) return decoder->jsonctx=-1;
  return 0;
}

/* Decode JSON string to caller's buffer.
 */
 
int gp1_decode_json_string(char *dst,int dsta,struct gp1_decoder *decoder) {
  int p0=decoder->srcp;
  const char *src=0;
  int srcc=gp1_decode_json_raw(&src,decoder);
  if (srcc<1) return -1;
  
  if (src[0]=='"') {
    int dstc=gp1_string_eval(dst,dsta,src,srcc);
    if (dstc<0) return decoder->jsonctx=-1;
    if (dstc>dsta) decoder->srcp=p0;
    return dstc;
  }
  
  if (srcc>dsta) {
    decoder->srcp=p0;
  } else {
    memcpy(dst,src,srcc);
    if (srcc<dsta) dst[srcc]=0;
  }
  return srcc;
}

/* Decode JSON string to encoder.
 */

int gp1_decode_json_string_to_encoder(struct gp1_encoder *dst,struct gp1_decoder *decoder) {
  const char *src=0;
  int srcc=gp1_decode_json_raw(&src,decoder);
  if (srcc<1) return -1;
  // Decoding a string token can only make it smaller: Require the token's length, then we don't need to check again.
  if (gp1_encoder_require(dst,srcc)<0) {
    decoder->srcp-=srcc;
    return -1;
  }
  if (src[0]=='"') {
    int err=gp1_string_eval(dst->v+dst->c,dst->a-dst->c,src,srcc);
    if (err<0) return decoder->jsonctx=-1;
    if (dst->c>dst->a-err) return -1; // shouldn't be possible
    dst->c+=err;
    return err;
  } else {
    memcpy(dst->v+dst->c,src,srcc);
    dst->c+=srcc;
    return srcc;
  }
}

/* Peek at the next JSON token.
 */

char gp1_decode_json_peek(struct gp1_decoder *decoder) {
  if (decoder->jsonctx<0) return '!';
  while ((decoder->srcp<decoder->srcc)&&((*(unsigned char*)DECODERP<=0x20)||(*(char*)DECODERP==','))) decoder->srcp++;
  if (decoder->srcp>=decoder->srcc) return '!';
  switch (*(char*)DECODERP) {
    case '[': return '[';
    case '{': return '{';
    case '"': return '"';
    case '+': return '+';
    case '-': return '+';
    case 'n': return 'n';
    case 't': return 't';
    case 'f': return 'f';
  }
  if ((*(char*)DECODERP>='0')&&(*(char*)DECODERP<='9')) return '+';
  return '!';
}
