#include "gp1_cli.h"
#include "io/gp1_serial.h"

/* Cleanup.
 */
 
static void gp1_rompack_strings_entry_cleanup(struct gp1_rompack_strings_entry *entry) {
  if (entry->v) free(entry->v);
}
 
void gp1_rompack_strings_cleanup(struct gp1_rompack_strings *strings) {
  if (strings->entryv) {
    while (strings->entryc-->0) gp1_rompack_strings_entry_cleanup(strings->entryv+strings->entryc);
    free(strings->entryv);
  }
}

/* Search.
 */

int gp1_rompack_strings_search(const struct gp1_rompack_strings *strings,uint16_t language,uint16_t stringid) {
  int lo=0,hi=strings->entryc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (language<strings->entryv[ck].language) hi=ck;
    else if (language>strings->entryv[ck].language) lo=ck+1;
    else if (stringid<strings->entryv[ck].stringid) hi=ck;
    else if (stringid>strings->entryv[ck].stringid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add, preformatted.
 */
 
int gp1_rompack_strings_add_handoff(struct gp1_rompack_strings *strings,uint16_t stringid,uint16_t language,char *src,int srcc) {
  if ((srcc<0)||(srcc&&!src)) return -1;
  if (srcc>0xffff) {
    fprintf(stderr,"String %d(%c%c) exceeds maximum length of 65535.\n",stringid,language>>8,language&0xff);
    return -1;
  }
  
  int p=gp1_rompack_strings_search(strings,language,stringid);
  if (p>=0) {
    fprintf(stderr,"String %d(%c%c) defined more than once.\n",stringid,language>>8,language&0xff);
    return -1;
  }
  p=-p-1;
  
  if (strings->entryc>=strings->entrya) {
    int na=strings->entrya+64;
    if (na>INT_MAX/sizeof(struct gp1_rompack_strings_entry)) return -1;
    void *nv=realloc(strings->entryv,sizeof(struct gp1_rompack_strings_entry)*na);
    if (!nv) return 1;
    strings->entryv=nv;
    strings->entrya=na;
  }
  
  struct gp1_rompack_strings_entry *entry=strings->entryv+p;
  memmove(entry+1,entry,sizeof(struct gp1_rompack_strings_entry)*(strings->entryc-p));
  strings->entryc++;
  entry->stringid=stringid;
  entry->language=language;
  entry->v=src;
  entry->c=srcc;
  
  return 0;
}

/* Add single line.
 */
 
int gp1_rompack_strings_add_line(struct gp1_rompack_strings *strings,const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  
  // Remove comment. Watch for quotes.
  int quote=0,srcp=0;
  for (;srcp<srcc;srcp++) {
    if (quote) {
      if (src[srcp]=='\\') srcp+=2;
      else if (src[srcp]=='"') quote=0;
    } else {
      if (src[srcp]=='"') quote=1;
      else if (src[srcp]=='#') srcc=srcp;
    }
  }
  
  // Remove trailing space. Don't worry about leading. Get out if empty.
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  if (srcc<1) return 0;
  
  // Measure the introducer (ID or IDlang)
  srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *lead=src+srcp;
  int leadc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; leadc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // Evaluate stringid and language.
  uint16_t language=0x2020;
  if ((leadc>2)&&(lead[leadc-1]>='a')&&(lead[leadc-1]<='z')&&(lead[leadc-2]>='a')&&(lead[leadc-2]<='z')) {
    leadc-=2;
    language=(lead[leadc]<<8)|lead[leadc+1];
  }
  int stringid=0;
  if (gp1_int_eval(&stringid,lead,leadc)<2) return -1;
  if ((stringid<1)||(stringid>0xffff)) return -1;
  
  // Evaluate body.
  const char *strsrc=src+srcp;
  int strsrcc=srcc-srcp;
  char *strdst=0;
  int strdstc=0;
  if (strsrcc) {
    if (!(strdst=malloc(strsrcc))) return -1;
    if (strsrc[0]=='"') {
      if (
        ((strdstc=gp1_string_eval(strdst,strsrcc,strsrc,strsrcc))<0)||
        (strdstc>strsrcc)
      ) {
        free(strdst);
        return -1;
      }
    } else {
      memcpy(strdst,strsrc,strsrcc);
      strdstc=strsrcc;
    }
  }
  if (gp1_rompack_strings_add_handoff(strings,stringid,language,strdst,strdstc)<0) {
    if (strdst) free(strdst);
    return -1;
  }
  return 0;
}

/* Add bulk text.
 */
 
int gp1_rompack_strings_add_text(struct gp1_rompack_strings *strings,const char *src,int srcc,const char *refname) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct gp1_decoder decoder={.src=src,.srcc=srcc};
  const char *line=0;
  int linec=0,lineno=0;
  while ((linec=gp1_decode_line(&line,&decoder))>0) {
    lineno++;
    if (gp1_rompack_strings_add_line(strings,line,linec)<0) {
      fprintf(stderr,"%s:%d: Failed to parse strings line.\n",refname,lineno);
      return -1;
    }
  }
  return 0;
}

/* Generate one "teXT" chunk from a set of strings entries of the same language.
 */
 
static int gp1_rompack_generate_teXT(struct gp1_rompack *rompack,const struct gp1_rompack_strings_entry *entryv,int entryc) {
  if (entryc<1) return 0;
  struct gp1_encoder encoder={0};
  if (gp1_encode_int16be(&encoder,entryv->language)<0) {
    gp1_encoder_cleanup(&encoder);
    return -1;
  }
  
  // TOC
  const struct gp1_rompack_strings_entry *v=entryv;
  int i=entryc;
  for (;i-->0;v++) {
    if (
      (gp1_encode_int16be(&encoder,v->stringid)<0)||
      (gp1_encode_int16be(&encoder,v->c)<0)
    ) {
      gp1_encoder_cleanup(&encoder);
      return -1;
    }
  }
  if (gp1_encode_raw(&encoder,"\0\0",2)<0) return -1;
  
  // Body
  for (v=entryv,i=entryc;i-->0;v++) {
    if (gp1_encode_raw(&encoder,v->v,v->c)<0) {
      gp1_encoder_cleanup(&encoder);
      return -1;
    }
  }
  
  if (gp1_rompack_handoff_chunk(rompack,GP1_CHUNK_TYPE('t','e','X','T'),encoder.v,encoder.c)<0) {
    gp1_encoder_cleanup(&encoder);
    return -1;
  }
  // Don't cleanup encoder.
  return 0;
}

/* Generate "teXT" chunks from (rompack->strings).
 */
 
int gp1_rompack_chunkify_strings(struct gp1_rompack *rompack) {
  int entryp=0;
  while (entryp<rompack->strings.entryc) {
    const struct gp1_rompack_strings_entry *entry=rompack->strings.entryv+entryp;
    int c=1;
    while ((entryp+c<rompack->strings.entryc)&&(entry[c].language==entry->language)) c++;
    if (gp1_rompack_generate_teXT(rompack,entry,c)<0) return -1;
    entryp+=c;
  }
  return 0;
}
