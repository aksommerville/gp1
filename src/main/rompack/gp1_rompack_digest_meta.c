#include "main/gp1_cli.h"
#include "io/gp1_serial.h"
#include "gp1/gp1.h"
#include <time.h>

/* Special primitives from JSON.
 */
 
static int gp1_rompack_decode_delimited_ints(int *dst,int dsta,struct gp1_decoder *decoder) {
  char tmp[32];
  int tmpc=gp1_decode_json_string(tmp,sizeof(tmp),decoder);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) return -1;
  int dstc=0,tmpp=0;
  while (tmpp<tmpc) {
    if (dstc>=dsta) return -1;
    if ((tmp[tmpp]<'0')||(tmp[tmpp]>'9')) return -1;
    int n=0;
    while ((tmpp<tmpc)&&(tmp[tmpp]>='0')&&(tmp[tmpp]<='9')) {
      n*=10;
      n+=tmp[tmpp++]-'0';
    }
    dst[dstc++]=n;
    if ((tmpp<tmpc)&&((tmp[tmpp]=='-')||(tmp[tmpp]=='.'))) tmpp++;
  }
  if (dstc!=dsta) return -1;
  return 0;
}
 
static int gp1_rompack_decode_version(uint8_t *dst,struct gp1_decoder *decoder) {
  int vv[3];
  if (gp1_rompack_decode_delimited_ints(vv,3,decoder)<0) return -1;
  if (vv[0]>0xff) return -1;
  if (vv[1]>0xff) return -1;
  if (vv[2]>0xffff) return -1;
  dst[0]=vv[0];
  dst[1]=vv[1];
  dst[2]=vv[2]>>8;
  dst[3]=vv[2];
  return 0;
}

static int gp1_rompack_decode_date(uint8_t *dst,struct gp1_decoder *decoder) {
  int vv[3];
  if (gp1_rompack_decode_delimited_ints(vv,3,decoder)<0) return -1;
  if (vv[0]>0xffff) return -1;
  if (vv[1]>0xff) return -1;
  if (vv[2]>0xff) return -1;
  dst[0]=vv[0]>>8;
  dst[1]=vv[0];
  dst[2]=vv[1];
  dst[3]=vv[2];
  return 0;
}

static int gp1_rompack_decode_u16(uint8_t *dst,struct gp1_decoder *decoder) {
  int n;
  if (gp1_decode_json_int(&n,decoder)<0) return -1;
  if ((n<0)||(n>0xffff)) return -1;
  dst[0]=n>>8;
  dst[1]=n;
  return 0;
}

static int gp1_rompack_decode_u8(uint8_t *dst,struct gp1_decoder *decoder) {
  int n;
  if (gp1_decode_json_int(&n,decoder)<0) return -1;
  if ((n<0)||(n>0xff)) return -1;
  dst[0]=n;
  return 0;
}

/* Decode meta.languages into a new "lANG" chunk.
 */
 
static int gp1_rompack_digest_meta_languages_cb(struct gp1_decoder *decoder,int p,void *userdata) {
  struct gp1_encoder *dst=userdata;
  if (gp1_decode_json_string_to_encoder(dst,decoder)!=2) return -1;
  return 0;
}
 
static int gp1_rompack_digest_meta_languages(
  struct gp1_rompack *rompack,
  struct gp1_decoder *decoder,
  struct gp1_rompack_input *input
) {
  struct gp1_encoder dst={0};
  int err=gp1_decode_json_array(decoder,gp1_rompack_digest_meta_languages_cb,&dst);
  if (err<0) {
    gp1_encoder_cleanup(&dst);
    return err;
  }
  if (gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('l','A','N','G'),dst.v,dst.c)<0) {
    gp1_encoder_cleanup(&dst);
    return -1;
  }
  return 0;
}

/* Decode the various meta fields that become hTXT chunks.
 */
 
struct gp1_rompack_digest_meta_hTXT_ctx {
  struct gp1_rompack *rompack;
  struct gp1_encoder *encoder;
};

static int gp1_rompack_digest_meta_hTXT_cb(struct gp1_decoder *decoder,int p,void *userdata) {
  struct gp1_rompack_digest_meta_hTXT_ctx *ctx=userdata;
  int pvctx=gp1_decode_json_array_start(decoder);
  if (pvctx<0) return -1;
  int lenp=ctx->encoder->c+2;
  
  // Inner array must be two strings, and the first must be exactly two bytes.
  int bodylen=0;
  if (
    (gp1_decode_json_next(0,decoder)!=1)||
    (gp1_decode_json_string_to_encoder(ctx->encoder,decoder)!=2)||
    (gp1_encode_raw(ctx->encoder,"\0\0",2)<0)|| // length placeholder
    (gp1_decode_json_next(0,decoder)!=1)||
    ((bodylen=gp1_decode_json_string_to_encoder(ctx->encoder,decoder))<0)||
    (gp1_decode_json_next(0,decoder)!=0)
  ) return -1;
  if (bodylen>0xffff) return -1;
  ((char*)ctx->encoder->v)[lenp]=bodylen>>8;
  ((char*)ctx->encoder->v)[lenp+1]=bodylen;
  
  if (gp1_decode_json_array_end(decoder,pvctx)<0) return -1;
  return 0;
}
 
static int gp1_rompack_digest_meta_hTXT(
  struct gp1_rompack *rompack,
  struct gp1_decoder *decoder,
  struct gp1_rompack_input *input,
  const char *key, // null to omit leading key (for non-hTXT things)
  struct gp1_encoder *dst // null to add a chunk
) {
  struct gp1_encoder encoder={0};
  if (!dst) dst=&encoder;
  if (key) {
    int keyc=0;
    while (key[keyc]) keyc++;
    if (keyc>8) return -1;
    if (gp1_encode_raw(dst,key,keyc)<0) return -1;
    if (gp1_encode_raw(dst,"\0\0\0\0\0\0\0\0",8-keyc)<0) return -1;
  }
  char jsontype=gp1_decode_json_peek(decoder);
  switch (jsontype) {
  
    case '"': {
        int lenp=dst->c+2;
        int bodylen;
        if (
          (gp1_encode_raw(dst,"  \0\0",4)<0)||
          ((bodylen=gp1_decode_json_string_to_encoder(dst,decoder))<0)||
          ((dst==&encoder)&&(gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('h','T','X','T'),dst->v,dst->c)<0))
        ) {
          gp1_encoder_cleanup(&encoder);
          return -1;
        }
        if (bodylen>0xffff) return -1;
        dst->v[lenp+0]=bodylen>>8;
        dst->v[lenp+1]=bodylen;
        // Don't clean up encoder.
      } return 0;
      
    case '[': {
        struct gp1_rompack_digest_meta_hTXT_ctx ctx={
          .rompack=rompack,
          .encoder=dst,
        };
        int err=gp1_decode_json_array(decoder,gp1_rompack_digest_meta_hTXT_cb,&ctx);
        if (err<0) {
          if (dst==&encoder) gp1_encoder_cleanup(&encoder);
          return err;
        }
        if (dst==&encoder) {
          if (gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('h','T','X','T'),dst->v,dst->c)<0) {
            gp1_encoder_cleanup(&encoder);
            return -1;
          }
        }
        // Don't clean up encoder.
      } return 0;
      
  }
  return -1;
}

/* Finalize and validate gpHD chunk.
 */
 
static int gp1_rompack_finalize_gpHD(uint8_t *v,struct gp1_rompack *rompack,const char *path) {
  
  // Target version defaults to whatever we are.
  if (!memcmp(v+0x08,"\0\0\0\0",4)) {
    v[0x08]=GP1_VERSION>>24;
    v[0x09]=GP1_VERSION>>16;
    v[0x0a]=GP1_VERSION>>8;
    v[0x0b]=GP1_VERSION;
  }
  
  // Publication date defaults to today.
  if (!memcmp(v+0x0c,"\0\0\0\0",4)) {
    struct tm tm={0};
    time_t now=time(0);
    localtime_r(&now,&tm);
    tm.tm_year+=1900;
    tm.tm_mon+=1;
    v[0x0c]=tm.tm_year>>8;
    v[0x0d]=tm.tm_year;
    v[0x0e]=tm.tm_mon;
    v[0x0f]=tm.tm_mday;
  }
  
  // Framebuffer width and height are required and we won't guess.
  if (!v[0x10]||!v[0x11]) {
    fprintf(stderr,"%s: Framebuffer size zero. Please check 'fbWidth' and 'fbHeight'\n",path);
    return -2;
  }
  
  // Player counts.
  if (v[0x14]>GP1_PLAYER_LAST) {
    fprintf(stderr,"%s: 'playerCountMin' must be %d or less, found %d\n",path,GP1_PLAYER_LAST,v[0x14]);
    return -2;
  }
  if (!v[0x15]) v[0x15]=v[0x14];
  else if (v[0x15]<v[0x14]) {
    fprintf(stderr,"%s: 'playerCountMax' < 'playerCountMin', that makes no sense\n",path);
    return -2;
  }
  
  return 0;
}

/* "net" in ".meta", to multiple "rURL" chunks.
 */
 
struct gp1_rompack_digest_meta_net_ctx {
  struct gp1_rompack *rompack;
  struct gp1_rompack_input *input;
  struct gp1_encoder encoder;
  char host[255];
  int hostc;
  int port;
};

static int gp1_rompack_digest_meta_net_cb_field(
  struct gp1_decoder *decoder,
  const char *k,int kc,
  void *userdata
) {
  struct gp1_rompack_digest_meta_net_ctx *ctx=userdata;
  
  if ((kc==4)&&!memcmp(k,"host",4)) {
    ctx->hostc=gp1_decode_json_string(ctx->host,sizeof(ctx->host),decoder);
    if ((ctx->hostc<0)||(ctx->hostc>sizeof(ctx->host))) {
      fprintf(stderr,
        "%s:%d: net[].host must be a string 0..255 bytes\n",
        ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp)
      );
      return -2;
    }
    return 0;
  }
  
  if ((kc==4)&&!memcmp(k,"port",4)) {
    if (
      (gp1_decode_json_int(&ctx->port,decoder)<0)||
      (ctx->port<0)||(ctx->port>0xffff)
    ) {
      fprintf(stderr,
        "%s:%d: net[].port must be integer 0..65535\n",
        ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp)
      );
      return -2;
    }
    return 0;
  }
  
  if ((kc==9)&&!memcmp(k,"rationale",9)) {
    return gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,0,&ctx->encoder);
  }
  
  fprintf(stderr,
    "%s:%d:WARNING: Ignoring unexpected field net[].%.*s\n",
    ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),kc,k
  );
  if (gp1_decode_json_raw(0,decoder)<0) return -1;
  return 0;
}
 
static int gp1_rompack_digest_meta_net_cb(struct gp1_decoder *decoder,int p,void *userdata) {
  struct gp1_rompack_digest_meta_net_ctx *ctx=userdata;
  
  ctx->hostc=0;
  ctx->port=0;
  ctx->encoder.c=0;
  
  int err=gp1_decode_json_object(decoder,gp1_rompack_digest_meta_net_cb_field,ctx);
  if (err<0) return err;
  
  int pfxlen=1+ctx->hostc+2;
  if (gp1_encoder_replace(&ctx->encoder,0,0,0,pfxlen)<0) return -1;
  uint8_t *dst=ctx->encoder.v;
  dst[0]=ctx->hostc;
  memcpy(dst+1,ctx->host,ctx->hostc);
  dst[1+ctx->hostc]=ctx->port>>8;
  dst[1+ctx->hostc+1]=ctx->port;
  
  if (gp1_rompack_handoff_chunk(ctx->rompack,GP1_CHUNK_TYPE('r','U','R','L'),ctx->encoder.v,ctx->encoder.c)<0) return -1;
  memset(&ctx->encoder,0,sizeof(struct gp1_encoder));
  return 0;
}
 
static int gp1_rompack_digest_meta_net(struct gp1_rompack *rompack,struct gp1_decoder *decoder,struct gp1_rompack_input *input) {
  struct gp1_rompack_digest_meta_net_ctx ctx={
    .rompack=rompack,
    .input=input,
    .encoder={0},
  };
  int err=gp1_decode_json_array(decoder,gp1_rompack_digest_meta_net_cb,&ctx);
  gp1_encoder_cleanup(&ctx.encoder);
  return err;
}

/* "storage" in ".meta", to multiple "sTOR" chunks.
 */
 
struct gp1_rompack_digest_meta_storage_ctx {
  struct gp1_rompack *rompack;
  struct gp1_rompack_input *input;
  struct gp1_encoder encoder;
  int key,sizeMin,sizeMax;
};

static int gp1_rompack_digest_meta_storage_cb_field(struct gp1_decoder *decoder,const char *k,int kc,void *userdata) {
  struct gp1_rompack_digest_meta_storage_ctx *ctx=userdata;
  
  if ((kc==9)&&!memcmp(k,"rationale",9)) {
    return gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,0,&ctx->encoder);
  }
  
  #define INTFLD(name) if ((kc==sizeof(#name)-1)&&!memcmp(k,#name,kc)) { \
    if (gp1_decode_json_int(&ctx->name,decoder)<0) { \
      fprintf(stderr, \
        "%s:%d: storage[].%.*s must be an integer\n", \
        ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),kc,k \
      ); \
      return -2; \
    } \
    return 0; \
  }
  INTFLD(key)
  INTFLD(sizeMin)
  INTFLD(sizeMax)
  #undef INTFLD
  
  fprintf(stderr,
    "%s:%d:WARNING: Ignoring unexpected field storage[].%.*s\n",
    ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),kc,k
  );
  if (gp1_decode_json_raw(0,decoder)<0) return -1;
  return 0;
}
 
static int gp1_rompack_digest_meta_storage_cb(struct gp1_decoder *decoder,int p,void *userdata) {
  struct gp1_rompack_digest_meta_storage_ctx *ctx=userdata;
  
  // Wipe per-chunk context and zero the fixed-length header.
  ctx->encoder.c=0;
  ctx->key=0;
  ctx->sizeMin=0;
  ctx->sizeMax=0;
  if (gp1_encode_raw(&ctx->encoder,"\0\0\0\0""\0\0\0\0""\0\0\0\0",12)<0) return -1;
  
  int err=gp1_decode_json_object(decoder,gp1_rompack_digest_meta_storage_cb_field,ctx);
  if (err<0) return err;
  
  // Validate chunk header.
  if (!ctx->key) {
    fprintf(stderr,
      "%s:%d: storage[].key zero or missing\n",
      ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp)
    );
    return -2;
  }
  if (!ctx->sizeMax) ctx->sizeMax=ctx->sizeMin;
  else if ((uint32_t)ctx->sizeMin>(uint32_t)ctx->sizeMax) {
    fprintf(stderr,
      "%s:%d: storage[].sizeMin > storage[].sizeMax (%u > %u)\n",
      ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),
      ctx->sizeMin,ctx->sizeMax
    );
    return -2;
  }
  
  // Populate chunk header.
  uint8_t *dst=ctx->encoder.v;
  dst[0]=ctx->key>>24;
  dst[1]=ctx->key>>16;
  dst[2]=ctx->key>>8;
  dst[3]=ctx->key;
  dst[4]=ctx->sizeMin>>24;
  dst[5]=ctx->sizeMin>>16;
  dst[6]=ctx->sizeMin>>8;
  dst[7]=ctx->sizeMin;
  dst[8]=ctx->sizeMax>>24;
  dst[9]=ctx->sizeMax>>16;
  dst[10]=ctx->sizeMax>>8;
  dst[11]=ctx->sizeMax;
  
  if (gp1_rompack_handoff_chunk(ctx->rompack,GP1_CHUNK_TYPE('s','T','O','R'),ctx->encoder.v,ctx->encoder.c)<0) return -1;
  memset(&ctx->encoder,0,sizeof(struct gp1_encoder));
  return 0;
}
 
static int gp1_rompack_digest_meta_storage(struct gp1_rompack *rompack,struct gp1_decoder *decoder,struct gp1_rompack_input *input) {
  struct gp1_rompack_digest_meta_storage_ctx ctx={
    .rompack=rompack,
    .input=input,
    .encoder={0},
  };
  int err=gp1_decode_json_array(decoder,gp1_rompack_digest_meta_storage_cb,&ctx);
  gp1_encoder_cleanup(&ctx.encoder);
  return err;
}

/* Produce chunks from ".meta" file.
 * Return -2 if logged.
 */
 
struct gp1_rompack_digest_meta_ctx {
  struct gp1_rompack *rompack;
  struct gp1_rompack_input *input;
  uint8_t gpHD[22];
};
 
static int gp1_rompack_digest_meta_cb(struct gp1_decoder *decoder,const char *k,int kc,void *userdata) {
  struct gp1_rompack_digest_meta_ctx *ctx=userdata;
  
  #define GENERAL_FIELD(key,call,expect) if ((kc==sizeof(key)-1)&&!memcmp(k,key,kc)) { \
    int err=(call); \
    if (err==-2) return -2; \
    if (err<0) { \
      fprintf(stderr, \
        "%s:%d: Failed to decode value for '%.*s', expected %s.\n", \
        ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),kc,k,expect \
      ); \
      return -2; \
    } \
    return 0; \
  }
  
  GENERAL_FIELD("gameVersion",gp1_rompack_decode_version(ctx->gpHD+0,decoder),"'MAJOR.MINOR.REVISION'")
  GENERAL_FIELD("gp1VersionMin",gp1_rompack_decode_version(ctx->gpHD+4,decoder),"'MAJOR.MINOR.REVISION'")
  GENERAL_FIELD("gp1VersionTarget",gp1_rompack_decode_version(ctx->gpHD+8,decoder),"'MAJOR.MINOR.REVISION'")
  GENERAL_FIELD("pubDate",gp1_rompack_decode_date(ctx->gpHD+12,decoder),"'YEAR-MONTH-DAY'")
  GENERAL_FIELD("fbWidth",gp1_rompack_decode_u16(ctx->gpHD+16,decoder),"1..65535")
  GENERAL_FIELD("fbHeight",gp1_rompack_decode_u16(ctx->gpHD+18,decoder),"1..65535")
  GENERAL_FIELD("playerCountMin",gp1_rompack_decode_u8(ctx->gpHD+20,decoder),"1..8")
  GENERAL_FIELD("playerCountMax",gp1_rompack_decode_u8(ctx->gpHD+21,decoder),"1..8")
  
  GENERAL_FIELD("languages",gp1_rompack_digest_meta_languages(ctx->rompack,decoder,ctx->input),"array of language codes")
  
  GENERAL_FIELD("title",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"title",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("copyright",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"copyrite",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("description",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"desc",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("author",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"author",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("website",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"website",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("license",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"license",0),"string or array of [LANGUAGE,STRING]")
  GENERAL_FIELD("variant",gp1_rompack_digest_meta_hTXT(ctx->rompack,decoder,ctx->input,"variant",0),"string or array of [LANGUAGE,STRING]")
  
  GENERAL_FIELD("net",gp1_rompack_digest_meta_net(ctx->rompack,decoder,ctx->input),"array of {host,port,rationale}")
  GENERAL_FIELD("storage",gp1_rompack_digest_meta_storage(ctx->rompack,decoder,ctx->input),"array of {key,sizeMin,sizeMax,rationale}")
  
  #undef GENERAL_FIELD
  
  fprintf(stderr,
    "%s:%d:WARNING: Ignoring unexpected field '%.*s'\n",
    ctx->input->path,1+gp1_count_newlines(decoder->src,decoder->srcp),kc,k
  );
  if (gp1_decode_json_raw(0,decoder)<0) return -1;
  return 0;
}
 
int gp1_rompack_digest_meta(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {

  struct gp1_rompack_digest_meta_ctx ctx={
    .rompack=rompack,
    .input=input,
  };
  int err=gp1_decode_json_object(decoder,gp1_rompack_digest_meta_cb,&ctx);
  if (err<0) return err;
  
  if ((err=gp1_rompack_finalize_gpHD(ctx.gpHD,rompack,input->path))<0) return err;
  if (gp1_rompack_add_chunk(rompack,GP1_CHUNK_TYPE('g','p','H','D'),ctx.gpHD,sizeof(ctx.gpHD))<0) return -1;
  
  return 0;
}
