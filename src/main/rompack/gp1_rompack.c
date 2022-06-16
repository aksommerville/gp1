#include "main/gp1_cli.h"
#include "io/gp1_serial.h"
#include "gp1/gp1.h"
#include <time.h>

int gp1_rompack_digest(struct gp1_rompack *rompack);

/* Cleanup.
 */
 
static void gp1_rompack_input_cleanup(struct gp1_rompack_input *input) {
  if (input->v) free(input->v);
  if (input->path) free(input->path);
}

static void gp1_rompack_chunk_cleanup(struct gp1_rompack_chunk *chunk) {
  if (chunk->v) free(chunk->v);
}
 
void gp1_rompack_cleanup(struct gp1_rompack *rompack) {
  gp1_rompack_strings_cleanup(&rompack->strings);
  if (rompack->inputv) {
    while (rompack->inputc-->0) gp1_rompack_input_cleanup(rompack->inputv+rompack->inputc);
    free(rompack->inputv);
  }
  if (rompack->chunkv) {
    while (rompack->chunkc-->0) gp1_rompack_chunk_cleanup(rompack->chunkv+rompack->chunkc);
    free(rompack->chunkv);
  }
}

/* Add inputs and chunks.
 */

int gp1_rompack_add_input(struct gp1_rompack *rompack,const void *src,int srcc,const char *path) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  void *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc))) return -1;
    memcpy(nv,src,srcc);
  }
  int err=gp1_rompack_handoff_input(rompack,nv,srcc,path);
  if (err<0) { free(nv); return -1; }
  return 0;
}

int gp1_rompack_add_chunk(struct gp1_rompack *rompack,uint32_t type,const void *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  void *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc))) return -1;
    memcpy(nv,src,srcc);
  }
  int err=gp1_rompack_handoff_chunk(rompack,type,nv,srcc);
  if (err<0) { free(nv); return -1; }
  return 0;
}

int gp1_rompack_handoff_input(struct gp1_rompack *rompack,void *src,int srcc,const char *path) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  
  // Check for ".chunk" files. If we have a valid one, insert a chunk instead of an input.
  // This means, the inputs=>chunks work we are able to do, we don't have to.
  // Anything looks fishy, proceed inserting as an input and we'll report the error later. (if it does turn out to be an error).
  if ((srcc>=4)&&!strcmp(gp1_get_suffix(path),"chunk")) {
    uint8_t *S=src;
    uint32_t type=(S[0]<<24)|(S[1]<<16)|(S[2]<<8)|S[3];
    if (gp1_valid_chunk_type(type)) {
      char *nv=malloc(srcc-4);
      if (nv) {
        memcpy(nv,S+4,srcc-4);
        int err=gp1_rompack_handoff_chunk(rompack,type,nv,srcc-4);
        if (err<0) free(nv);
        return err;
      }
    }
  }
  
  if (rompack->inputc>=rompack->inputa) {
    int na=rompack->inputa+32;
    if (na>INT_MAX/sizeof(struct gp1_rompack_input)) return -1;
    void *nv=realloc(rompack->inputv,sizeof(struct gp1_rompack_input)*na);
    if (!nv) return -1;
    rompack->inputv=nv;
    rompack->inputa=na;
  }
  struct gp1_rompack_input *input=rompack->inputv+rompack->inputc++;
  memset(input,0,sizeof(struct gp1_rompack_input));
  input->v=src;
  input->c=srcc;
  if (path&&path[0]) {
    input->path=strdup(path); // ignore errors
  }
  return 0;
}

int gp1_rompack_handoff_chunk(struct gp1_rompack *rompack,uint32_t type,void *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  if (rompack->chunkc>=rompack->chunka) {
    int na=rompack->chunka+32;
    if (na>INT_MAX/sizeof(struct gp1_rompack_chunk)) return -1;
    void *nv=realloc(rompack->chunkv,sizeof(struct gp1_rompack_chunk)*na);
    if (!nv) return -1;
    rompack->chunkv=nv;
    rompack->chunka=na;
  }
  struct gp1_rompack_chunk *chunk=rompack->chunkv+rompack->chunkc++;
  memset(chunk,0,sizeof(struct gp1_rompack_chunk));
  chunk->v=src;
  chunk->c=srcc;
  chunk->type=type;
  return 0;
}

/* Sort chunks.
 * Requirements:
 *  - gpHD must be first.
 *  - Chunks with a language must have language variants adjacent.
 * Not required but we do anyway:
 *  - Metadata before game content.
 *  - Chunks with IDs sorted low to high.
 */
 
static int gp1_rompack_chunk_category(uint32_t type) {
  
  // gpHD in a class by itself.
  if (type==GP1_CHUNK_TYPE('g','p','H','D')) return 1;
  
  // Meta: lANG,hTXT,rURL,sTOR
  if (type==GP1_CHUNK_TYPE('l','A','N','G')) return 2;
  if (type==GP1_CHUNK_TYPE('h','T','X','T')) return 2;
  if (type==GP1_CHUNK_TYPE('r','U','R','L')) return 2;
  if (type==GP1_CHUNK_TYPE('s','T','O','R')) return 2;
  
  // Game: exEC,teXT,imAG,auDO,soNG,daTA
  if (type==GP1_CHUNK_TYPE('e','x','E','C')) return 3;
  if (type==GP1_CHUNK_TYPE('t','e','X','T')) return 3;
  if (type==GP1_CHUNK_TYPE('i','m','A','G')) return 3;
  if (type==GP1_CHUNK_TYPE('a','u','D','O')) return 3;
  if (type==GP1_CHUNK_TYPE('s','o','N','G')) return 3;
  if (type==GP1_CHUNK_TYPE('d','a','T','A')) return 3;
  
  return 4;
}
 
static int gp1_rompack_compare_chunks(const struct gp1_rompack_chunk *a,const struct gp1_rompack_chunk *b) {

  // First put chunk types in categories: gpHD, metadata, game content.
  int acat=gp1_rompack_chunk_category(a->type);
  int bcat=gp1_rompack_chunk_category(b->type);
  if (acat<bcat) return -1;
  if (acat>bcat) return 1;
  
  // Next sort by chunk type arbitrarily, just ensure all chunks of the same type are adjacent.
  if (a->type<b->type) return -1;
  if (a->type>b->type) return 1;
  
  // Chunks that start with an ID, sort by ID low to high.
  // Many of these do allow ties (for language variants).
  // That's fine, just keep the ties adjacent, with their inner order undefined.
  if ( // 4-byte ID.
    (a->type==GP1_CHUNK_TYPE('s','T','O','R'))||
    (a->type==GP1_CHUNK_TYPE('i','m','A','G'))||
    (a->type==GP1_CHUNK_TYPE('s','o','N','G'))||
    (a->type==GP1_CHUNK_TYPE('d','a','T','A'))||
  0) {
    if ((a->c<4)||(b->c<4)) return 0;
    const uint8_t *A=a->v,*B=b->v;
    uint32_t aid=(A[0]<<24)|(A[1]<<16)|(A[2]<<8)|A[3];
    uint32_t bid=(B[0]<<24)|(B[1]<<16)|(B[2]<<8)|B[3];
    if (aid<bid) return -1;
    if (aid>bid) return 1;
  }
  
  // Still undecided? No worries, any order is fine.
  return 0;
}
 
static void gp1_rompack_sort_chunks(struct gp1_rompack *rompack) {
  qsort(rompack->chunkv,rompack->chunkc,sizeof(struct gp1_rompack_chunk),(void*)gp1_rompack_compare_chunks);
}

/* Validate chunk TOC (not the payloads).
 *  - Must have exactly one gpHD, in the first position.
 *  - Must have exactly one exEC.
 *  - Must have zero or one lANG.
 */
 
static int gp1_rompack_validate_chunks(struct gp1_rompack *rompack) {
  if (rompack->chunkc<1) {
    fprintf(stderr,
      "%s: ROM ended up with no chunks. Must have at least 'gpHD' and 'exEC'. Are you missing the '.meta' or '.wasm' file?\n",
      rompack->config->dstpath
    );
    return -1;
  }
  if (rompack->chunkv[0].type!=GP1_CHUNK_TYPE('g','p','H','D')) {
    fprintf(stderr,"%s: 'gpHD' chunk must be first. Is the '.meta' file missing?\n",rompack->config->dstpath);
    return -1;
  }
  int gpHDc=0,exECc=0,lANGc=0;
  const struct gp1_rompack_chunk *chunk=rompack->chunkv;
  int i=rompack->chunkc;
  for (;i-->0;chunk++) {
         if (chunk->type==GP1_CHUNK_TYPE('g','p','H','D')) gpHDc++;
    else if (chunk->type==GP1_CHUNK_TYPE('e','x','E','C')) exECc++;
    else if (chunk->type==GP1_CHUNK_TYPE('l','A','N','G')) lANGc++;
  }
  if (gpHDc!=1) {
    fprintf(stderr,"%s: Must have one 'gpHD' chunk, found %d.\n",rompack->config->dstpath,gpHDc);
    return -1;
  }
  if (exECc!=1) {
    fprintf(stderr,"%s: Must have one 'exEC' chunk, found %d. These are created from '.wasm' files.\n",rompack->config->dstpath,exECc);
    return -1;
  }
  if (lANGc>1) {
    fprintf(stderr,"%s: Multiple 'lANG' chunks (%d), limit 1.\n",rompack->config->dstpath,lANGc);
    return -1;
  }
  /* TODO There is more validation we could do.
   * eg chunks with an ID or language, ensure it is unique.
   */
  return 0;
}

/* Last step: Pack all the chunks into one stream.
 */
 
static int gp1_rompack_encode_output(void *dstpp,struct gp1_rompack *rompack) {
  int dsta=0,i=rompack->chunkc;
  const struct gp1_rompack_chunk *chunk=rompack->chunkv;
  for (;i-->0;chunk++) {
    if (!gp1_valid_chunk_type(chunk->type)) return -1;
    if (chunk->c<0) return -1;
    dsta+=8;
    dsta+=chunk->c;
  }
  uint8_t *dst=malloc(dsta);
  if (!dst) return -1;
  int dstc=0;
  for (i=rompack->chunkc,chunk=rompack->chunkv;i-->0;chunk++) {
  
    dst[dstc++]=chunk->type>>24;
    dst[dstc++]=chunk->type>>16;
    dst[dstc++]=chunk->type>>8;
    dst[dstc++]=chunk->type;
    dst[dstc++]=chunk->c>>24;
    dst[dstc++]=chunk->c>>16;
    dst[dstc++]=chunk->c>>8;
    dst[dstc++]=chunk->c;
    
    memcpy(dst+dstc,chunk->v,chunk->c);
    dstc+=chunk->c;
  }
  if (dstc!=dsta) {
    free(dst);
    return -1;
  }
  *(void**)dstpp=dst;
  return dstc;
}

/* Generate output.
 */
 
int gp1_rompack_generate_output(void *dstpp,struct gp1_rompack *rompack) {
  if (gp1_rompack_digest(rompack)<0) return -1;
  gp1_rompack_sort_chunks(rompack);
  if (gp1_rompack_validate_chunks(rompack)<0) return -1;
  return gp1_rompack_encode_output(dstpp,rompack);
}
