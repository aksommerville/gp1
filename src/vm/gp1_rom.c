#include "gp1_rom.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <zlib.h>

#define GP1_CHUNK_TYPE(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|d)

/* Delete.
 */
 
static void gp1_rurl_cleanup(struct gp1_rurl *rurl) {
  if (rurl->host) free(rurl->host);
  if (rurl->rationale) free(rurl->rationale);
}

static void gp1_stor_cleanup(struct gp1_stor *stor) {
  if (stor->rationale) free(stor->rationale);
}

static void gp1_imag_cleanup(struct gp1_imag *imag) {
  if (imag->v) free(imag->v);
}

static void gp1_song_cleanup(struct gp1_song *song) {
  if (song->v) free(song->v);
}

static void gp1_data_cleanup(struct gp1_data *data) {
  if (data->v) free(data->v);
}
 
void gp1_rom_del(struct gp1_rom *rom) {
  if (!rom) return;
  if (rom->refc-->1) return;
  
  if (rom->exec) free(rom->exec);
  
  if (rom->ht_title) free(rom->ht_title);
  if (rom->ht_author) free(rom->ht_author);
  if (rom->ht_copyrite) free(rom->ht_copyrite);
  if (rom->ht_website) free(rom->ht_website);
  if (rom->ht_license) free(rom->ht_license);
  if (rom->ht_variant) free(rom->ht_variant);
  if (rom->ht_desc) free(rom->ht_desc);
  
  if (rom->stringv) free(rom->stringv);
  if (rom->text) free(rom->text);
  
  if (rom->rurlv) {
    while (rom->rurlc-->0) gp1_rurl_cleanup(rom->rurlv+rom->rurlc);
    free(rom->rurlv);
  }
  if (rom->storv) {
    while (rom->storc-->0) gp1_stor_cleanup(rom->storv+rom->storc);
    free(rom->storv);
  }
  
  if (rom->imagv) {
    while (rom->imagc-->0) gp1_imag_cleanup(rom->imagv+rom->imagc);
    free(rom->imagv);
  }
  
  if (rom->audo) free(rom->audo);
  
  if (rom->songv) {
    while (rom->songc-->0) gp1_song_cleanup(rom->songv+rom->songc);
    free(rom->songv);
  }
  
  if (rom->datav) {
    while (rom->datac-->0) gp1_data_cleanup(rom->datav+rom->datac);
    free(rom->datav);
  }
  
  free(rom);
}

/* Retain.
 */
 
int gp1_rom_ref(struct gp1_rom *rom) {
  if (!rom) return -1;
  if (rom->refc<1) return -1;
  if (rom->refc==INT_MAX) return -1;
  rom->refc++;
  return 0;
}

/* Select language.
 */
 
static int gp1_langcmp(uint16_t a,uint16_t b,const uint16_t *prefv,int prefc) {
  if (a==b) return 0;
  
  // If one is zero, we definitely want the other.
  if (!a) return 1;
  if (!b) return -1;
  
  // If one is listed first in the preferences, use it.
  for (;prefc-->0;prefv++) {
    if (*prefv==a) return -1;
    if (*prefv==b) return 1;
  }
  
  // Prefer universal.
  if (a==0x2020) return -1;
  if (b==0x2020) return 1;
  
  //TODO Opportunity here to introduce crazy logic like "Dutch is similar to German, so this Dutch speaker would rather see German than Telugu."
  // Not sure I want to open that box tho.
  
  // Whatever.
  return 0;
}

#define RD8(p) src[p];
#define RD16(p) ((src[p]<<8)|src[p+1])
#define RD32(p) ((src[p]<<24)|(src[p+1]<<16)|(src[p+2]<<8)|src[p+3])

/* Receive gpHD
 */
 
static int gp1_rom_decode_gpHD(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  if (srcc<22) {
    if (refname) fprintf(stderr,"%s: 'gpHD' chunk length %d, expected 22\n",refname,srcc);
    return -2;
  }
  rom->game_version=RD32(0);
  rom->gp1_version_min=RD32(4);
  rom->gp1_version_target=RD32(8);
  rom->pub_date=RD32(12);
  rom->fbw=RD16(16);
  rom->fbh=RD16(18);
  rom->playerc_min=RD8(20);
  rom->playerc_max=RD8(21);
  return 0;
}

/* Receive exEC
 */
 
static int gp1_rom_decode_exEC(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  if (rom->exec) {
    if (refname) fprintf(stderr,"%s: Multiple 'exEC' chunks\n",refname);
    return -2;
  }
  if (!(rom->exec=malloc(srcc))) return -1;
  memcpy(rom->exec,src,srcc);
  rom->execc=srcc;
  return 0;
}

/* Receive lANG
 */
 
static int gp1_rom_decode_lANG(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  // Not interesting, we take a more detailed approach to language.
  return 0;
}

/* Receive hTXT
 */
 
static int gp1_rom_set_string(char **dstpp,const char *src,int srcc) {
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (*dstpp) free(*dstpp);
  *dstpp=nv;
  return 0;
}
 
static int gp1_rom_decode_hTXT(struct gp1_rom *rom,const uint8_t *src,int srcc,const uint16_t *langv,int langc,const char *refname) {
  if (srcc<8) return 0; // illegal but don't make a fuss over it.
  
  const char *k=(char*)src;
  int kc=0;
  while ((kc<8)&&k[kc]) kc++;
  
  uint16_t lang=0;
  const char *text=0;
  int textc=0;
  
  int srcp=8,stopp=srcc-4;
  while (srcp<=stopp) {
    uint16_t qlang=(src[srcp]<<8)|src[srcp+1];
    uint16_t qlen=(src[srcp+2]<<8)|src[srcp+3];
    srcp+=4;
    if (srcp>srcc-qlen) break; // illegal
    if (gp1_langcmp(lang,qlang,langv,langc)>0) {
      lang=qlang;
      text=(char*)src+srcp;
      textc=qlen;
    }
    srcp+=qlen;
  }
  
  #define _(tag) if ((kc==sizeof(#tag)-1)&&!memcmp(k,#tag,kc)) return gp1_rom_set_string(&rom->ht_##tag,text,textc);
  _(title)
  _(author)
  _(copyrite)
  _(website)
  _(license)
  _(variant)
  _(desc)
  #undef _
  
  if (refname) {
    fprintf(stderr,"%s: Unknown header text: '%.*s' = '%.*s'\n",refname,kc,k,textc,text);
  }
  return 0;
}

/* Receive teXT
 */
 
static int gp1_rom_decode_teXT(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  
  // Read the TOC first. Verify lengths and order.
  uint16_t pvstringid=0;
  int srcp=2,tocc=0,textc=0;
  while (1) {
    if (srcp>srcc-2) return -1;
    uint16_t stringid=(src[srcp]<<8)|src[srcp+1];
    srcp+=2;
    if (!stringid) break;
    if (stringid<=pvstringid) {
      if (refname) fprintf(stderr,"%s: 'teXT' TOC missorted, %d<=%d\n",refname,stringid,pvstringid);
      return -2;
    }
    pvstringid=stringid;
    if (srcp>srcc-2) return -1;
    uint16_t len=(src[srcp]<<8)|src[srcp+1];
    srcp+=2;
    textc+=len;
    tocc++;
  }
  if (srcp<srcc-textc) {
    if (refname) fprintf(stderr,"%s: TOC length short. Found %d, expected %d\n",refname,srcc-srcp,textc);
    return -2;
  }
  
  // Allocate the two buffers.
  if (tocc>rom->stringa) {
    void *nv=realloc(rom->stringv,sizeof(struct gp1_string)*tocc);
    if (!nv) return -1;
    rom->stringv=nv;
    rom->stringa=tocc;
  }
  if (textc>rom->texta) {
    void *nv=realloc(rom->text,textc);
    if (!nv) return -1;
    rom->text=nv;
    rom->texta=textc;
  }
  rom->stringc=0;
  rom->textc=0;
  
  // Walk the TOC again and populate our copy.
  struct gp1_string *string=rom->stringv;
  int i=tocc;
  for (srcp=2;i-->0;srcp+=4,string++) {
    string->stringid=(src[srcp]<<8)|src[srcp+1];
    string->c=(src[srcp+2]<<8)|src[srcp+3];
    string->v=rom->text+rom->textc;
    rom->textc+=string->c;
  }
  rom->stringc=tocc;
  
  return 0;
}

/* Receive rURL
 */
 
static int gp1_rom_decode_rURL(struct gp1_rom *rom,const uint8_t *src,int srcc,const uint16_t *langv,int langc,const char *refname) {
  
  int srcp=0;
  if (srcp>=srcc) return -1;
  int hostc=src[srcp++];
  if (srcp>srcc-hostc) return -1;
  const char *host=src+srcp;
  srcp+=hostc;
  if (srcp>srcc-2) return -1;
  int port=(src[srcp]<<8)|src[srcp+1];
  srcp+=2;
  
  uint16_t lang=0;
  const char *rationale=0;
  int rationalec=0;
  while (srcp<=srcc-4) {
    uint16_t qlang=(src[srcp]<<8)|src[srcp+1];
    uint16_t qlen=(src[srcp+2]<<8)|src[srcp+3];
    srcp+=4;
    if (srcp>srcc-qlen) break;
    if (gp1_langcmp(lang,qlang,langv,langc)>0) {
      lang=qlang;
      rationale=src+srcp;
      rationalec=qlen;
    }
    srcp+=qlen;
  }
  
  if (rom->rurlc>=rom->rurla) {
    int na=rom->rurla+8;
    if (na>INT_MAX/sizeof(struct gp1_rurl)) return -1;
    void *nv=realloc(rom->rurlv,sizeof(struct gp1_rurl)*na);
    if (!nv) return -1;
    rom->rurlv=nv;
    rom->rurla=na;
  }
  
  char *nhost=0,*nrationale=0;
  if (hostc) {
    if (!(nhost=malloc(hostc+1))) return -1;
    memcpy(nhost,host,hostc);
    nhost[hostc]=0;
  }
  if (rationalec) {
    if (!(nrationale=malloc(rationalec+1))) {
      if (nhost) free(nhost);
      return -1;
    }
    memcpy(nrationale,rationale,rationalec);
    nrationale[rationalec]=0;
  }
  
  struct gp1_rurl *rurl=rom->rurlv+rom->rurlc++;
  rurl->host=nhost;
  rurl->hostc=hostc;
  rurl->port=port;
  rurl->rationale=nrationale;
  rurl->rationalec=rationalec;
  
  return 0;
}

/* Receive sTOR
 */
 
static int gp1_rom_decode_sTOR(struct gp1_rom *rom,const uint8_t *src,int srcc,const uint16_t *langv,int langc,const char *refname) {
  
  if (srcc<12) return -1;
  uint32_t key=RD32(0);
  uint32_t min=RD32(4);
  uint32_t max=RD32(8);
  int srcp=12;
  
  uint16_t lang=0;
  const char *rationale=0;
  int rationalec=0;
  while (srcp<=srcc-4) {
    uint16_t qlang=(src[srcp]<<8)|src[srcp+1];
    uint16_t qlen=(src[srcp+2]<<8)|src[srcp+3];
    srcp+=4;
    if (srcp>srcc-qlen) break;
    if (gp1_langcmp(lang,qlang,langv,langc)>0) {
      lang=qlang;
      rationale=src+srcp;
      rationalec=qlen;
    }
    srcp+=qlen;
  }
  
  if (rom->storc>=rom->stora) {
    int na=rom->stora+8;
    if (na>INT_MAX/sizeof(struct gp1_stor)) return -1;
    void *nv=realloc(rom->storv,sizeof(struct gp1_stor)*na);
    if (!nv) return -1;
    rom->storv=nv;
    rom->stora=na;
  }
  
  char *nrationale=0;
  if (rationalec) {
    if (!(nrationale=malloc(rationalec+1))) return -1;
    memcpy(nrationale,rationale,rationalec);
    nrationale[rationalec]=0;
  }
  
  struct gp1_stor *stor=rom->storv+rom->storc++;
  stor->key=key;
  stor->min=min;
  stor->max=max;
  stor->rationale=nrationale;
  stor->rationalec=rationalec;
  
  return 0;
}

/* Receive imAG
 */
 
static int gp1_rom_decode_imAG(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  
  if (srcc<11) return -1;
  uint32_t imageid=RD32(0);
  uint16_t w=RD16(6);
  uint16_t h=RD16(8);
  uint8_t format=RD8(10);
  
  int p=gp1_rom_imag_search(rom,imageid);
  if (p>=0) {
    if (refname) fprintf(stderr,"%s: Duplicate 'imAG' ID %u\n",refname,imageid);
    return -2;
  }
  p=-p-1;
  
  struct gp1_imag *imag=gp1_rom_imag_insert(rom,p,imageid,src+11,srcc-11);
  if (!imag) return -1;
  
  imag->w=w;
  imag->h=h;
  imag->format=format;
  
  return 0;
}

/* Receive auDO
 */
 
static int gp1_rom_decode_auDO(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {
  void *nv=malloc(srcc);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  if (rom->audo) free(rom->audo);
  rom->audo=nv;
  rom->audoc=srcc;
  return 0;
}

/* Receive soNG
 */
 
static int gp1_rom_decode_soNG(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {

  if (srcc<4) return -1;
  uint32_t songid=RD32(0);
  int p=gp1_rom_song_search(rom,songid);
  if (p>=0) {
    if (refname) fprintf(stderr,"%s: Duplicate 'soNG' ID %u\n",refname,songid);
    return -2;
  }
  p=-p-1;
  
  struct gp1_song *song=gp1_rom_song_insert(rom,p,songid,src+4,srcc-4);
  if (!song) return -1;
  
  return 0;
}

/* Receive daTA
 */
 
static int gp1_rom_decode_daTA(struct gp1_rom *rom,const uint8_t *src,int srcc,const char *refname) {

  if (srcc<4) return -1;
  uint32_t dataid=RD32(0);
  int p=gp1_rom_data_search(rom,dataid);
  if (p>=0) {
    if (refname) fprintf(stderr,"%s: Duplicate 'daTA' ID %u\n",refname,dataid);
    return -2;
  }
  p=-p-1;
  
  struct gp1_data *data=gp1_rom_data_insert(rom,p,dataid,src+4,srcc-4);
  if (!data) return -1;
  
  return 0;
}

/* Receive unknown chunk.
 */
 
static int gp1_rom_decode_unknown(struct gp1_rom *rom,uint32_t chunktype,const void *src,int srcc,const char *refname) {
  char ca=chunktype>>24,cb=chunktype>>16,cc=chunktype>>8,cd=chunktype;
  if (
    (ca<0x21)||(ca>0x7e)||
    (cb<0x21)||(cb>0x7e)||
    (cc<0x21)||(cc>0x7e)||
    (cd<0x21)||(cd>0x7e)
  ) {
    if (refname) fprintf(stderr,"%s: Illegal chunk type 0x%08x\n",refname,chunktype);
    return -2;
  }
  if (cb&0x20) {
    //TODO Maybe this doesn't need to be fatal.
    if (refname) fprintf(stderr,"%s: Unrecognized critical chunk '%c%c%c%c'\n",refname,ca,cb,cc,cd);
    return -2;
  }
  return 0;
}

/* Decode to fresh object.
 */
 
static int gp1_rom_decode(struct gp1_rom *rom,const uint8_t *src,int srcc,const uint16_t *langv,int langc,const char *refname) {
  int srcp=0,stopp=srcc-8;
  while (srcp<=stopp) {
    int srcp0=srcp,err;
    uint32_t chunktype=(src[srcp]<<24)|(src[srcp+1]<<16)|(src[srcp+2]<<8)|src[srcp+3];
    int len=(src[srcp+4]<<24)|(src[srcp+5]<<16)|(src[srcp+6]<<8)|src[srcp+7];
    srcp+=8;
    if ((len<0)||(srcp>srcc-len)) {
      if (refname) fprintf(stderr,"%s: Illegal length %d for chunk at %d/%d\n",refname,len,srcp0,srcc);
      return -2;
    }
    const uint8_t *chunk=src+srcp;
    srcp+=len;
    switch (chunktype) {
    
      /* Chunks with a single declared language, we must read ahead and then select one.
       */
      case GP1_CHUNK_TYPE('t','e','X','T'): {
          if (len<2) { err=-1; break; }
          uint16_t current_language=(chunk[0]<<8)|chunk[1];
          while (srcp<=stopp) {
            uint32_t next_chunktype=(src[srcp]<<24)|(src[srcp+1]<<16)|(src[srcp+2]<<8)|src[srcp+3];
            if (next_chunktype!=GP1_CHUNK_TYPE('t','e','X','T')) break;
            int next_len=(src[srcp+4]<<24)|(src[srcp+5]<<16)|(src[srcp+6]<<8)|src[srcp+7];
            if (next_len<2) { err=-1; break; }
            srcp+=8;
            if (srcp>srcc-2) { err=-1; break; }
            uint16_t next_language=(src[srcp]<<8)|src[srcp+1];
            if (gp1_langcmp(current_language,next_language,langv,langc)>0) {
              chunk=src+srcp;
              len=next_len;
              current_language=next_language;
            }
            srcp+=next_len;
          }
          if (err<0) break;
          err=gp1_rom_decode_teXT(rom,chunk,len,refname);
        } break;
      case GP1_CHUNK_TYPE('i','m','A','G'): {
          if (len<6) { err=-1; break; }
          uint32_t current_id=(chunk[0]<<24)|(chunk[1]<<16)|(chunk[2]<<8)|chunk[3];
          uint16_t current_language=(chunk[4]<<8)|chunk[5];
          while (srcp<=stopp) {
            uint32_t next_chunktype=(src[srcp]<<24)|(src[srcp+1]<<16)|(src[srcp+2]<<8)|src[srcp+3];
            if (next_chunktype!=GP1_CHUNK_TYPE('i','m','A','G')) break;
            int next_len=(src[srcp+4]<<24)|(src[srcp+5]<<16)|(src[srcp+6]<<8)|src[srcp+7];
            if (next_len<6) { err=-1; break; }
            srcp+=8;
            if (srcp>srcc-6) { err=-1; break; }
            uint32_t next_id=(src[srcp]<<24)|(src[srcp+1]<<16)|(src[srcp+2]<<8)|src[srcp+3];
            uint16_t next_language=(src[srcp+4]<<8)|src[srcp+5];
            if (gp1_langcmp(current_language,next_language,langv,langc)>0) {
              chunk=src+srcp;
              len=next_len;
              current_language=next_language;
            }
            srcp+=next_len;
          }
          if (err<0) break;
          err=gp1_rom_decode_imAG(rom,chunk,len,refname);
        } break;
      
      /* Language-less chunks, or those containing language variations internally.
       */
      case GP1_CHUNK_TYPE('h','T','X','T'): err=gp1_rom_decode_hTXT(rom,chunk,len,langv,langc,refname); break;
      case GP1_CHUNK_TYPE('g','p','H','D'): err=gp1_rom_decode_gpHD(rom,chunk,len,refname); break;
      case GP1_CHUNK_TYPE('e','x','E','C'): err=gp1_rom_decode_exEC(rom,chunk,len,refname); break;
      case GP1_CHUNK_TYPE('l','A','N','G'): err=gp1_rom_decode_lANG(rom,chunk,len,refname); break;
      case GP1_CHUNK_TYPE('r','U','R','L'): err=gp1_rom_decode_rURL(rom,chunk,len,langv,langc,refname); break;
      case GP1_CHUNK_TYPE('s','T','O','R'): err=gp1_rom_decode_sTOR(rom,chunk,len,langv,langc,refname); break;
      case GP1_CHUNK_TYPE('a','u','D','O'): err=gp1_rom_decode_auDO(rom,chunk,len,refname); break;
      case GP1_CHUNK_TYPE('s','o','N','G'): err=gp1_rom_decode_soNG(rom,chunk,len,refname); break;
      case GP1_CHUNK_TYPE('d','a','T','A'): err=gp1_rom_decode_daTA(rom,chunk,len,refname); break;
      default: err=gp1_rom_decode_unknown(rom,chunktype,chunk,len,refname); break;
    }
    if (err<0) {
      if (refname&&(err!=-2)) {
        char ca=chunktype>>24,cb=chunktype>>16,cc=chunktype>>8,cd=chunktype;
        if ((ca<=0x20)||(ca>0x7e)) ca='?';
        if ((cb<=0x20)||(cb>0x7e)) cb='?';
        if ((cc<=0x20)||(cc>0x7e)) cc='?';
        if ((cd<=0x20)||(cd>0x7e)) cd='?';
        fprintf(stderr,
          "%s:%d/%d: Error decoding %d-byte '%c%c%c%c' chunk.\n",
          refname,srcp0,srcc,len,ca,cb,cc,cd
        );
      }
      return -2;
    }
  }
  return 0;
}

/* New.
 */

struct gp1_rom *gp1_rom_new(const void *src,int srcc,const uint16_t *langv,int langc,const char *refname) {
  if (!langv||(langc<0)) langc=0;
  struct gp1_rom *rom=calloc(1,sizeof(struct gp1_rom));
  if (!rom) return 0;
  rom->refc=1;
  
  if (gp1_rom_decode(rom,src,srcc,langv,langc,refname)<0) {
    gp1_rom_del(rom);
    return 0;
  }
  
  return rom;
}

/* Get string.
 */
 
int gp1_rom_string_get(void *dstpp,const struct gp1_rom *rom,uint16_t stringid) {
  int lo=0,hi=rom->stringc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (stringid<rom->stringv[ck].stringid) hi=ck;
    else if (stringid>rom->stringv[ck].stringid) lo=ck+1;
    else {
      *(void**)dstpp=(void*)rom->stringv[ck].v;
      return rom->stringv[ck].c;
    }
  }
  return 0;
}

/* Search images.
 */

int gp1_rom_imag_search(const struct gp1_rom *rom,uint32_t imageid) {
  if (rom->imagc<1) return -1;
  if (imageid>rom->imagv[rom->imagc-1].imageid) return -rom->imagc-1;
  int lo=0,hi=rom->imagc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (imageid<rom->imagv[ck].imageid) hi=ck;
    else if (imageid>rom->imagv[ck].imageid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert image.
 */
 
struct gp1_imag *gp1_rom_imag_insert(struct gp1_rom *rom,int p,uint32_t imageid,const void *src,int srcc) {
  if ((p<0)||(p>rom->imagc)) return 0;
  if (p&&(imageid<=rom->imagv[p-1].imageid)) return 0;
  if ((p<rom->imagc)&&(imageid>=rom->imagv[p].imageid)) return 0;
  
  if (rom->imagc>=rom->imaga) {
    int na=rom->imaga+16;
    if (na>INT_MAX/sizeof(struct gp1_imag)) return 0;
    void *nv=realloc(rom->imagv,sizeof(struct gp1_imag)*na);
    if (!nv) return 0;
    rom->imagv=nv;
    rom->imaga=na;
  }
  
  void *nv=malloc(srcc);
  if (!nv) return 0;
  memcpy(nv,src,srcc);
  
  struct gp1_imag *imag=rom->imagv+p;
  memmove(imag+1,imag,sizeof(struct gp1_imag)*(rom->imagc-p));
  rom->imagc++;
  memset(imag,0,sizeof(struct gp1_imag));
  
  imag->imageid=imageid;
  imag->v=nv;
  imag->c=srcc;
  
  return imag;
}

/* Search songs.
 */
 
int gp1_rom_song_search(const struct gp1_rom *rom,uint32_t songid) {
  if (rom->songc<1) return -1;
  if (songid>rom->songv[rom->songc-1].songid) return -rom->songc-1; // Common and worst-case; pull it forward.
  int lo=0,hi=rom->songc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (songid<rom->songv[ck].songid) hi=ck;
    else if (songid>rom->songv[ck].songid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert song.
 */
 
struct gp1_song *gp1_rom_song_insert(struct gp1_rom *rom,int p,uint32_t songid,const void *src,int srcc) {
  if ((p<0)||(p>rom->songc)) return 0;
  if (p&&(songid<=rom->songv[p-1].songid)) return 0;
  if ((p<rom->songc)&&(songid>=rom->songv[p].songid)) return 0;
  
  if (rom->songc>=rom->songa) {
    int na=rom->songa+16;
    if (na>INT_MAX/sizeof(struct gp1_song)) return 0;
    void *nv=realloc(rom->songv,sizeof(struct gp1_song)*na);
    if (!nv) return 0;
    rom->songv=nv;
    rom->songa=na;
  }
  
  void *nv=malloc(srcc);
  if (!nv) return 0;
  memcpy(nv,src,srcc);
  
  struct gp1_song *song=rom->songv+p;
  memmove(song+1,song,sizeof(struct gp1_song)*(rom->songc-p));
  rom->songc++;
  song->songid=songid;
  song->v=nv;
  song->c=srcc;
  
  return song;
}

/* Search data.
 */
 
int gp1_rom_data_search(const struct gp1_rom *rom,uint32_t dataid) {
  if (rom->datac<1) return -1;
  if (dataid>rom->datav[rom->datac-1].dataid) return -rom->datac-1;
  int lo=0,hi=rom->datac;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (dataid<rom->datav[ck].dataid) hi=ck;
    else if (dataid>rom->datav[ck].dataid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert data.
 */
 
struct gp1_data *gp1_rom_data_insert(struct gp1_rom *rom,int p,uint32_t dataid,const void *src,int srcc) {
  if ((p<0)||(p>rom->datac)) return 0;
  if (p&&(dataid<=rom->datav[p-1].dataid)) return 0;
  if ((p<rom->datac)&&(dataid>=rom->datav[p].dataid)) return 0;
  
  if (rom->datac>=rom->dataa) {
    int na=rom->dataa+64;
    if (na>INT_MAX/sizeof(struct gp1_data)) return 0;
    void *nv=realloc(rom->datav,sizeof(struct gp1_data)*na);
    if (!nv) return 0;
    rom->datav=nv;
    rom->dataa=na;
  }
  
  void *nv=malloc(srcc);
  if (!nv) return 0;
  memcpy(nv,src,srcc);
  
  struct gp1_data *data=rom->datav+p;
  memmove(data+1,data,sizeof(struct gp1_data)*(rom->datac-p));
  rom->datac++;
  data->dataid=dataid;
  data->v=nv;
  data->c=srcc;
  
  return data;
}
