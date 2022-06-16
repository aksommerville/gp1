#include "gp1_cli.h"
#include "io/gp1_serial.h"
#include "gp1/gp1.h"
#include <time.h>

int gp1_rompack_digest_meta(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
);

int gp1_rompack_chunkify_strings(struct gp1_rompack *rompack);

/* Populate string cache from ".strings" file.
 * Return -2 if logged.
 */
 
static int gp1_rompack_digest_strings(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  if (gp1_rompack_strings_add_text(&rompack->strings,input->v,input->c,input->path)<0) return -2;
  return 0;
}

/* Image, song, and audio config files are unlikely; they are normally processed via `gp1 convertchunk`.
 * But we do accept them. Hook into the same logic convertchunk would use. TODO
 */
 
static int gp1_rompack_digest_image(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  fprintf(stderr,"TODO %s %s\n",__func__,input->path);
  return -2;
}
 
static int gp1_rompack_digest_midi(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  fprintf(stderr,"TODO %s %s\n",__func__,input->path);
  return -2;
}
 
static int gp1_rompack_digest_aucfg(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  fprintf(stderr,"TODO %s %s\n",__func__,input->path);
  return -2;
}

static int gp1_rompack_digest_data(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  fprintf(stderr,"TODO %s %s\n",__func__,input->path);
  return -2;
}

/* Digest input, for the lucky case where we know an input file becomes a chunk exactly.
 * So far this only applies to ".wasm" => "exEC".
 */
 
static int gp1_rompack_digest_1_1(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  uint32_t type
) {
  if (gp1_rompack_handoff_chunk(rompack,type,input->v,input->c)<0) return -1;
  input->v=0;
  input->c=0;
  return 0;
}

/* Digest one input file.
 * A decoder is provided, so you can get line-number reporting implicitly.
 * Return -2 if you logged the error, anything else <0 the wrapper will log it.
 */
 
static int gp1_rompack_digest_input(
  struct gp1_rompack *rompack,
  struct gp1_rompack_input *input,
  struct gp1_decoder *decoder
) {
  const char *suffix=gp1_get_suffix(input->path);
  
  if (!strcmp(suffix,"chunk")) {
    // ".chunk" files are processed when we receive them. If they're still here, something failed.
    fprintf(stderr,"%s: Malformed chunk file.\n",input->path);
    return -2;
  }
  
  if (!strcmp(suffix,"meta")) return gp1_rompack_digest_meta(rompack,input,decoder);
  if (!strcmp(suffix,"wasm")) return gp1_rompack_digest_1_1(rompack,input,GP1_CHUNK_TYPE('e','x','E','C'));
  if (!strcmp(suffix,"png")) return gp1_rompack_digest_image(rompack,input,decoder);
  if (!strcmp(suffix,"gif")) return gp1_rompack_digest_image(rompack,input,decoder);
  if (!strcmp(suffix,"bmp")) return gp1_rompack_digest_image(rompack,input,decoder);
  if (!strcmp(suffix,"jpeg")) return gp1_rompack_digest_image(rompack,input,decoder);
  if (!strcmp(suffix,"webp")) return gp1_rompack_digest_image(rompack,input,decoder);
  if (!strcmp(suffix,"mid")) return gp1_rompack_digest_midi(rompack,input,decoder);
  if (!strcmp(suffix,"aucfg")) return gp1_rompack_digest_aucfg(rompack,input,decoder);
  if (!strcmp(suffix,"strings")) return gp1_rompack_digest_strings(rompack,input,decoder);
  if (!strcmp(suffix,"data")) return gp1_rompack_digest_data(rompack,input,decoder);
  if (!strcmp(suffix,"bin")) return gp1_rompack_digest_data(rompack,input,decoder);
  
  fprintf(stderr,"%s: Unable to determine data type from suffix '%.*s'. Please rename or exclude file.\n",input->path,suffix);
  return 0;
}

/* Add hTXT:title if missing.
 */
 
static int gp1_rompack_default_missing_title(struct gp1_rompack *rompack) {
  const struct gp1_rompack_chunk *chunk=rompack->chunkv;
  int i=rompack->chunkc;
  for (;i-->0;chunk++) {
    if (chunk->type!=GP1_CHUNK_TYPE('h','T','X','T')) continue;
    if (chunk->c<8) continue; // illegal, but it's the wrong time to enforce that
    if (memcmp(chunk->v,"title\0\0\0",8)) continue;
    return 0; // ok we have a title already
  }
  const char *stem=gp1_get_basename(rompack->config->dstpath);
  int stemc=0;
  while (stem[stemc]&&(stem[stemc]!='.')) stemc++;
  if (!stemc) {
    fprintf(stderr,"WARNING: Unable to guess title.\n");
    return -1;
  }
  fprintf(stderr,"WARNING: Setting title by default, all languages: '%.*s'\n",stemc,stem);
  char *body=malloc(8+2+2+stemc);
  if (!body) return -1;
  memcpy(body,"title\0\0\0  ",10);
  body[10]=stemc>>8;
  body[11]=stemc;
  memcpy(body+12,stem,stemc);
  if (gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('h','T','X','T'),body,8+2+2+stemc)<0) {
    free(body);
    return -1;
  }
  // Don't free body.
  return 0;
}

/* Add lANG if missing.
 */
 
static int gp1_rompack_default_missing_lANG(struct gp1_rompack *rompack) {
  const struct gp1_rompack_chunk *chunk=rompack->chunkv;
  int i=rompack->chunkc;
  for (;i-->0;chunk++) {
    if (chunk->type!=GP1_CHUNK_TYPE('l','A','N','G')) continue;
    return 0; // got one, cool, do nothing
  }
  
  uint16_t languagev[26*26]={0};
  for (chunk=rompack->chunkv,i=rompack->chunkc;i-->0;chunk++) {
  
    if (chunk->type==GP1_CHUNK_TYPE('h','T','X','T')) {
      const uint8_t *src=chunk->v+8;
      int srcc=chunk->c-8;
      while (srcc>=4) {
        if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
          int langid=((src[0]-'a')*26)+src[1]-'a';
          languagev[langid]=1;
        }
        int len=(src[2]<<8)|src[3];
        src+=4+len;
        srcc-=4+len;
      }
      continue;
    }
    
    if (chunk->type==GP1_CHUNK_TYPE('r','U','R','L')) {
      if (chunk->c>3) {
        const uint8_t *src=chunk->v;
        int hdrlen=3+src[0];
        src+=hdrlen;
        int srcc=chunk->c-hdrlen;
        while (srcc>=4) {
          if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
            int langid=((src[0]-'a')*26)+src[1]-'a';
            languagev[langid]=1;
          }
          int len=(src[2]<<8)|src[3];
          src+=4+len;
          srcc-=4+len;
        }
      }
      continue;
    }
    
    if (chunk->type==GP1_CHUNK_TYPE('s','T','O','R')) {
      if (chunk->c>12) {
        const uint8_t *src=chunk->v+12;
        int srcc=chunk->c-12;
        while (srcc>=4) {
          if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
            int langid=((src[0]-'a')*26)+src[1]-'a';
            languagev[langid]=1;
          }
          int len=(src[2]<<8)|src[3];
          src+=4+len;
          srcc-=4+len;
        }
      }
      continue;
    }
    
    if (chunk->type==GP1_CHUNK_TYPE('t','e','X','T')) {
      if (chunk->c>=2) {
        const uint8_t *src=chunk->v;
        if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
          int langid=((src[0]-'a')*26)+src[1]-'a';
          languagev[langid]=1;
        }
      }
      continue;
    }
    
    if (chunk->type==GP1_CHUNK_TYPE('i','m','A','G')) {
      if (chunk->c>=6) {
        const uint8_t *src=chunk->v;
        src+=4;
        if ((src[0]>='a')&&(src[0]<='z')&&(src[1]>='a')&&(src[1]<='z')) {
          int langid=((src[0]-'a')*26)+src[1]-'a';
          languagev[langid]=1;
        }
      }
      continue;
    }
  }
  
  fprintf(stderr,"%s:WARNING: Languages not declared. Assuming:",rompack->config->dstpath);
  char sep=' ';
  int stop=26*26;
  for (i=0;i<stop;i++) {
    if (languagev[i]) {
      char cha='a'+i/26;
      char chb='a'+i%26;
      fprintf(stderr,"%c%c%c",sep,cha,chb);
      sep=',';
    }
  }
  fprintf(stderr,"\n");
  
  struct gp1_encoder encoder={0};
  for (i=0;i<stop;i++) {
    if (languagev[i]) {
      char tmp[2]={'a'+i/26,'a'+i%26};
      if (gp1_encode_raw(&encoder,tmp,2)<0) {
        gp1_encoder_cleanup(&encoder);
        return -1;
      }
    }
  }
  if (gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('l','A','N','G'),encoder.v,encoder.c)<0) {
    gp1_encoder_cleanup(&encoder);
    return -1;
  }
  
  return 0;
}

/* Digest inputs, main entry point.
 * Basically, we examine each input and produce chunks from it.
 * At entry:
 *  - All input files have been read into either (inputv) or (chunkv).
 *  - Chunks are not sorted or validated.
 * On success:
 *  - Input files have been fully consumed and are no longer needed.
 *  - No more chunks should be added.
 *  - Chunks are still not sorted or fully validated.
 */
 
int gp1_rompack_digest(struct gp1_rompack *rompack) {

  // Process all inputs. Populates (strings,chunkv).
  struct gp1_rompack_input *input=rompack->inputv;
  int i=rompack->inputc;
  for (;i-->0;input++) {
    struct gp1_decoder decoder={.src=input->v,.srcc=input->c};
    int err=gp1_rompack_digest_input(rompack,input,&decoder);
    if (err==-2) return -1;
    if (err<0) {
      if (gp1_is_text(decoder.src,decoder.srcc)) {
        int lineno=1+gp1_count_newlines(decoder.src,decoder.srcp);
        fprintf(stderr,"%s:%d: Unknown error.\n",input->path?input->path:"(unknown)",lineno);
      } else {
        fprintf(stderr,"%s:%d/%d: Unknown error.\n",input->path?input->path:"(unknown)",decoder.srcp,decoder.srcc);
      }
      return -1;
    }
  }
  
  // Create "teXT" chunks from (strings).
  if (gp1_rompack_chunkify_strings(rompack)<0) return -1;
  
  // Add optional things that we can default.
  if (gp1_rompack_default_missing_title(rompack)<0) return -1;
  if (gp1_rompack_default_missing_lANG(rompack)<0) return -1;
  
  return 0;
}
