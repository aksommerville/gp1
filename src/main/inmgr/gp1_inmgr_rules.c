#include "gp1_inmgr_internal.h"
#include "io/gp1_serial.h"
#include "io/gp1_io_input.h"
#include "gp1/gp1.h"

/* Cleanup.
 */
 
void gp1_inmgr_rules_cleanup(struct gp1_inmgr_rules *rules) {
  if (rules->name) free(rules->name);
  if (rules->driver) free(rules->driver);
  if (rules->buttonv) free(rules->buttonv);
}

/* Compare rules to criteria.
 */
 
static int gp1_inmgr_rules_match(const struct gp1_inmgr_rules *rules,const char *name,int namec,int vid,int pid) {
  if (!rules->valid) return 0; // rules explicitly neutered
  int score=1;
  if (rules->namec) {
    int sub=gp1_pattern_match(rules->name,rules->namec,name,namec);
    if (sub<1) return 0;
    score+=sub;
  }
  if (rules->vid) {
    if (rules->vid!=vid) return 0;
    score+=100;
  }
  if (rules->pid) {
    if (rules->pid!=pid) return 0;
    score+=100;
  }
  return score;
}

/* Find rules.
 */
 
struct gp1_inmgr_rules *gp1_inmgr_rules_find(const struct gp1_inmgr *inmgr,const char *name,int namec,int vid,int pid) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  struct gp1_inmgr_rules *rules=inmgr->rulesv;
  int i=inmgr->rulesc;
  for (;i-->0;rules++) {
    // First match only. For now, we don't care about the score beyond zero/nonzero.
    if (gp1_inmgr_rules_match(rules,name,namec,vid,pid)) return rules;
  }
  return 0;
}

/* Add rules to master list.
 */
 
struct gp1_inmgr_rules *gp1_inmgr_rules_add(struct gp1_inmgr *inmgr,int p) {
  if (p<0) p=inmgr->rulesc;
  if (p>inmgr->rulesc) return 0;
  if (inmgr->rulesc>=inmgr->rulesa) {
    int na=inmgr->rulesa+16;
    if (na>INT_MAX/sizeof(struct gp1_inmgr_rules)) return 0;
    void *nv=realloc(inmgr->rulesv,sizeof(struct gp1_inmgr_rules)*na);
    if (!nv) return 0;
    inmgr->rulesv=nv;
    inmgr->rulesa=na;
  }
  struct gp1_inmgr_rules *rules=inmgr->rulesv+p;
  memmove(rules+1,rules,sizeof(struct gp1_inmgr_rules)*(inmgr->rulesc-p));
  inmgr->rulesc++;
  memset(rules,0,sizeof(struct gp1_inmgr_rules));
  rules->valid=1;
  return rules;
}

/* Search buttons.
 */
 
int gp1_inmgr_rules_button_search(const struct gp1_inmgr_rules *rules,int srcbtnid) {
  int lo=0,hi=rules->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (srcbtnid<rules->buttonv[ck].srcbtnid) hi=ck;
    else if (srcbtnid>rules->buttonv[ck].srcbtnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert button.
 */

struct gp1_inmgr_rules_button *gp1_inmgr_rules_button_insert(struct gp1_inmgr_rules *rules,int p,int srcbtnid) {
  if ((p<0)||(p>rules->buttonc)) return 0;
  if (p&&(srcbtnid<=rules->buttonv[p-1].srcbtnid)) return 0;
  if ((p<rules->buttonc)&&(srcbtnid>=rules->buttonv[p].srcbtnid)) return 0;
  if (rules->buttonc>=rules->buttona) {
    int na=rules->buttona+16;
    if (na>INT_MAX/sizeof(struct gp1_inmgr_rules_button)) return 0;
    void *nv=realloc(rules->buttonv,sizeof(struct gp1_inmgr_rules_button)*na);
    if (!nv) return 0;
    rules->buttonv=nv;
    rules->buttona=na;
  }
  struct gp1_inmgr_rules_button *button=rules->buttonv+p;
  memmove(button+1,button,sizeof(struct gp1_inmgr_rules_button)*(rules->buttonc-p));
  rules->buttonc++;
  memset(button,0,sizeof(struct gp1_inmgr_rules_button));
  button->srcbtnid=srcbtnid;
  return button;
}

/* Begin a config block.
 */
 
static int gp1_inmgr_rules_begin_block(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules **rules,
  const char *src,int srcc,
  const char *path,int lineno
) {
  if (!(*rules=gp1_inmgr_rules_add(inmgr,-1))) return -1;
  
  // First token may be "vvvv:pppp", otherwise the whole thing is the name pattern.
  int tokenc=0;
  while ((tokenc<srcc)&&((unsigned char)src[tokenc]>0x20)) tokenc++;
  if ((tokenc==9)&&(src[4]==':')) {
    if (
      (gp1_hexuint_eval(&(*rules)->vid,src,4)==2)&&
      (gp1_hexuint_eval(&(*rules)->pid,src+5,4)==2)
    ) {
      src+=tokenc;
      srcc-=tokenc;
      while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
    }
  }
  
  if (srcc>0) {
    if (!((*rules)->name=malloc(srcc+1))) return -1;
    memcpy((*rules)->name,src,srcc);
    (*rules)->name[srcc]=0;
    (*rules)->namec=srcc;
  }
  
  return 0;
}

/* Evaluate input range "LO..HI"
 */
 
static int gp1_inmgr_eval_range(int *lo,int *hi,const char *src,int srcc) {
  int sepp=-1,srcp=0;
  for (;srcp<srcc;srcp++) {
    if (src[srcp]=='.') {
      if ((srcp>srcc-2)||(src[srcp+1]!='.')) return -1;
      sepp=srcp;
      break;
    }
  }
  if (sepp<0) return -1;
  if (sepp) {
    if (gp1_int_eval(lo,src,sepp)<2) return -1;
  } else {
    *lo=INT_MIN;
  }
  if (sepp<srcc-2) {
    if (gp1_int_eval(hi,src+sepp+2,srcc-sepp-2)<2) return -1;
  } else {
    *hi=INT_MAX;
  }
  return 0;
}

/* Add a button rule.
 */
 
static int gp1_inmgr_rules_decode_button(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules *rules,
  int srcbtnid,
  const char *src,int srcc,
  const char *path,int lineno
) {

  // Read range if present. Input is either one or two space-delimited tokens, easy.
  int srclo=0,srchi=-1;
  int leadc=0;
  while ((leadc<srcc)&&((unsigned char)src[leadc]>0x20)) leadc++;
  if (leadc<srcc) {
    if (gp1_inmgr_eval_range(&srclo,&srchi,src,leadc)<0) {
      if (path) {
        fprintf(stderr,"%s:%d: Expected two integers 'LO..HI', found '%.*s'\n",path,lineno,leadc,src);
        return -2;
      }
      return -1;
    }
    src+=leadc;
    srcc-=leadc;
    while (srcc&&((unsigned char)src[0]<=0x20)) { src++; srcc--; }
  }
  
  int dstbtnid=gp1_btnid_eval(src,srcc);
  if (dstbtnid<0) {
    if (inmgr->delegate.btnid_eval) {
      dstbtnid=inmgr->delegate.btnid_eval(inmgr,src,srcc);
    }
    if (dstbtnid<0) {
      if (path) {
        fprintf(stderr,"%s:%d: Failed to evaluate '%.*s' as a GP1 button or action name.\n",path,lineno,srcc,src);
        return -2;
      }
      return -1;
    }
  }
  
  int p=gp1_inmgr_rules_button_search(rules,srcbtnid);
  if (p>=0) {
    if (path) {
      fprintf(stderr,"%s:%d: Duplicate definition of button 0x%08x\n",path,lineno,srcbtnid);
      return -2;
    }
    return -1;
  }
  p=-p-1;
  struct gp1_inmgr_rules_button *button=gp1_inmgr_rules_button_insert(rules,p,srcbtnid);
  if (!button) return -1;
  button->srclo=srclo;
  button->srchi=srchi;
  button->dstbtnid=dstbtnid;

  return 0;
}

/* Add a "playerid" rule.
 */
 
static int gp1_inmgr_rules_decode_playerid(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules *rules,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int playerid=0;
  if (
    (gp1_int_eval(&playerid,src,srcc)<2)||
    (playerid<1)||(playerid>GP1_PLAYER_LAST)
  ) {
    if (path) {
      fprintf(stderr,"%s:%d: Expected integer 1..%d, found '%.*s'\n",path,lineno,GP1_PLAYER_LAST,srcc,src);
      return -2;
    }
    return -1;
  }
  if (playerid==rules->playerid) return 0;
  if (rules->playerid) {
    if (path) {
      fprintf(stderr,"%s:%d: Multiple 'player' in rules block\n",path,lineno);
      return -2;
    }
    return -1;
  }
  rules->playerid=playerid;
  return 0;
}

/* Add a "driver" criterion.
 */
 
static int gp1_inmgr_rules_decode_driver(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules *rules,
  const char *src,int srcc,
  const char *path,int lineno
) {
  
  /* Driver name is the full input.
   * "any" is equivalent to empty.
   * If redundant, whatever, get out.
   * If we already have a different one, fail.
   */
  if ((srcc==3)&&!memcmp(src,"any",3)) srcc=0;
  if ((srcc==rules->driverc)&&!memcmp(src,rules->driver,srcc)) return 0;
  if (rules->driverc) {
    if (path) {
      fprintf(stderr,"%s:%d: Multiple 'driver' in rule block.\n",path,lineno);
      return -2;
    }
    return -1;
  }
  
  if (!(rules->driver=malloc(srcc+1))) return -1;
  memcpy(rules->driver,src,srcc);
  rules->driver[srcc]=0;
  rules->driverc=srcc;
  
  /* Everything is legal.
   * If it's not "none" or an existing input driver, neuter the rules, we know it will never match.
   */
  if ((srcc!=4)||memcmp(src,"none",4)) {
    const struct gp1_input_type *type=gp1_input_type_by_name(src,srcc);
    if (!type) {
      rules->valid=0;
    }
  }
  
  return 0;
}

/* Add an "auto" rule.
 */
 
static int gp1_inmgr_rules_decode_auto(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules *rules,
  const char *src,int srcc,
  const char *path,int lineno
) {
  if (srcc) {
    if (path) {
      fprintf(stderr,"%s:%d: Unexpected tokens after 'auto'\n",path,lineno);
      return -2;
    }
    return -1;
  }
  rules->autoconfig=1;
  return 0;
}

/* Decode one line of input config.
 */
 
static int gp1_inmgr_decode_rules_line(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_rules **rules,
  const char *src,int srcc,
  const char *path,int lineno
) {
  int srcp=0,err;
  
  // First token is a keyword, possibly a fence.
  const char *kw=src+srcp;
  int kwc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; kwc++; }
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  
  // Begin a new block?
  if ((kwc==6)&&!memcmp(kw,"device",6)) {
    return gp1_inmgr_rules_begin_block(inmgr,rules,src+srcp,srcc-srcp,path,lineno);
  }
  
  // Anything else requires a block.
  if (!*rules) {
    if (path) {
      fprintf(stderr,"%s:%d: Must introduce block with 'device [VID:PID] NAME'.\n",path,lineno);
      return -2;
    }
    return -1;
  }
  
  // Likeliest: (kw) is integer srcbtnid.
  int srcbtnid;
  if (gp1_int_eval(&srcbtnid,kw,kwc)>0) return gp1_inmgr_rules_decode_button(inmgr,*rules,srcbtnid,src+srcp,srcc-srcp,path,lineno);
  
  // Single-use keywords, not so likely.
  if ((kwc==6)&&!memcmp(kw,"player",6)) return gp1_inmgr_rules_decode_playerid(inmgr,*rules,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==6)&&!memcmp(kw,"driver",6)) return gp1_inmgr_rules_decode_driver(inmgr,*rules,src+srcp,srcc-srcp,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"auto",4)) return gp1_inmgr_rules_decode_auto(inmgr,*rules,src+srcp,srcc-srcp,path,lineno);
  
  if (path) {
    fprintf(stderr,"%s:%d: Expected button ID or (device,playerid,driver,auto), found '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  return -1;
}

/* Decode rules.
 */
 
int gp1_inmgr_decode_rules(struct gp1_inmgr *inmgr,struct gp1_decoder *decoder,const char *path) {
  struct gp1_inmgr_rules *rules=0;
  int lineno=0,linec=0;
  const char *line;
  while ((linec=gp1_decode_line(&line,decoder))>0) {
    lineno++;
    int i=0;
    for (;i<linec;i++) if (line[i]=='#') linec=i;
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec) continue;
    int err=gp1_inmgr_decode_rules_line(inmgr,&rules,line,linec,path,lineno);
    if (err<0) {
      if (path&&(err!=-2)) fprintf(stderr,"%s:%d: Error decoding input rules.\n",path,lineno);
      return -1;
    }
  }
  return 0;
}

/* Btnid names. (TODO not sure these are an inmgr thing, is there a better home for it?)
 */
 
int gp1_btnid_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return GP1_BTNID_##tag;
  GP1_FOR_EACH_BTNID
  _(HORZ)
  _(VERT)
  _(DPAD)
  #undef _
  return -1;
}

const char *gp1_btnid_repr(int btnid) {
  switch (btnid) {
    #define _(tag) case GP1_BTNID_##tag: return #tag;
    GP1_FOR_EACH_BTNID
    _(HORZ)
    _(VERT)
    _(DPAD)
    #undef _
  }
  return 0;
}
