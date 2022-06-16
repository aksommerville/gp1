#include "gp1_cli.h"

/* Cleanup.
 */
 
void gp1_config_cleanup(struct gp1_config *config) {
  if (config->dstpath) free(config->dstpath);
  if (config->video_driver_name) free(config->video_driver_name);
  if (config->audio_driver_name) free(config->audio_driver_name);
  if (config->input_driver_names) free(config->input_driver_names);
  if (config->posv) {
    int i=config->posc; while (i-->0) {
      if (config->posv[i]) free(config->posv[i]);
    }
    free(config->posv);
  }
}
  
/* Evaluate integer. TODO needs a better home. and a better implementation
 */
 
static int gp1_int_eval(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  *dst=0;
  for (;srcc-->0;src++) {
    int digit=(*src)-'0';
    if ((digit<0)||(digit>9)) return -1;
    if (*dst>INT_MAX/10) return -1;
    (*dst)*=10;
    if (*dst>INT_MAX-digit) return -1;
    (*dst)+=digit;
  }
  return 2;
}

/* Set string.
 */
 
static int gp1_config_set_string(char **dst,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (*dst) free(*dst);
  *dst=nv;
  return 0;
}

/* Print help.
 */
 
void gp1_config_print_help(struct gp1_config *config) {
  fprintf(stderr,"\nUsage: %s COMMAND [OPTIONS] [INPUTS...]\n\n",config->exename);
  fprintf(stderr,
    "COMMAND:\n"
    "  help              Print this message.\n"
    "  run               Play a game. 1 INPUT.\n"
    "  pack              Generate ROM file from prepared chunks. Multiple INPUTs.\n"
    "  convertchunk      Reformat a data file as a ROM chunk. 1 INPUT.\n"
    "  info              Print details about a ROM file. Multiple INPUTs.\n"
    "\n"
    "OPTIONS:\n"
    "  --out=PATH        Output path for pack, convertchunk.\n"
    "  --video=NAME      Video driver (run).\n"
    "  --audio=NAME      Audio driver (run).\n"
    "  --inputs=NAMES    Input drivers, comma delimited (run).\n"
    "  --audio-rate=HZ   Audio output rate (run).\n"
    "  --audio-chanc=1|2 Audio channel count (run).\n"
    "  --mono            same as '--audio-chanc=1'\n"
    "  --stereo          same as '--audio-chanc=2'\n"
    "\n"
  );
}

/* Receive key=value argument.
 */
 
static int gp1_config_kv(struct gp1_config *config,const char *k,int kc,const char *v,int vc) {

  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  // Commands may be supplied positionally, or with two leading dashes.
  int command=gp1_command_eval(k,kc);
  if (command>0) {
    if (config->command==command) return 0;
    if (config->command!=GP1_COMMAND_UNSET) {
      fprintf(stderr,"%s: Multiple commands '%s' and '%.*s'\n",config->exename,gp1_command_repr(config->command),kc,k);
      return -1;
    }
    config->command=command;
    return 0;
  }
  
  #define STRINGPARAM(fieldname,optname) if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,kc)) { \
    if (gp1_config_set_string(&config->fieldname,v,vc)<0) return -1; \
    return 0; \
  }
  #define INTPARAM(fieldname,optname,lo,hi) if ((kc==sizeof(optname)-1)&&!memcmp(k,optname,kc)) { \
    int vn=0; \
    if (gp1_int_eval(&vn,v,vc)<2) { \
      fprintf(stderr,"%s: Failed to evaluate '%.*s' as integer for option '%.*s'\n",config->exename,vc,v,kc,k); \
      return -1; \
    } \
    if ((vn<lo)||(vn>hi)) { \
      fprintf(stderr,"%s: %d out of range for option '%.*s', limit %d..%d\n",config->exename,vn,kc,k,lo,hi); \
      return -1; \
    } \
    config->fieldname=vn; \
    return 0; \
  }
  
  STRINGPARAM(dstpath,"out")
  STRINGPARAM(video_driver_name,"video")
  STRINGPARAM(audio_driver_name,"audio")
  STRINGPARAM(input_driver_names,"inputs")
  INTPARAM(audio_rate,"audio-rate",200,200000)
  INTPARAM(audio_chanc,"audio-chanc",1,2)
  
  #undef STRINGPARAM
  #undef INTPARAM
  
  if ((kc==4)&&!memcmp(k,"mono",4)) { config->audio_chanc=1; return 0; }
  if ((kc==6)&&!memcmp(k,"stereo",6)) { config->audio_chanc=2; return 0; }

  fprintf(stderr,"%s: Unexpected option '%.*s' = '%.*s'\n",config->exename,kc,k,vc,v);
  return -1;
}

/* Receive positional argument.
 */
 
static int gp1_config_positional(struct gp1_config *config,const char *src,int srcc) {

  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }

  // First is the command, if we don't have it yet.
  if (config->command==GP1_COMMAND_UNSET) {
    int command=gp1_command_eval(src,srcc);
    if (command<0) {
      fprintf(stderr,"%s: Expected command, found '%.*s'\n",config->exename,srcc,src);
      return -1;
    }
    config->command=command;
    return 0;
  }
  
  // Everything else, add it to the pile.
  if (config->posc>=config->posa) {
    int na=config->posa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(config->posv,sizeof(void*)*na);
    if (!nv) return -1;
    config->posv=nv;
    config->posa=na;
  }
  char *nv=malloc(srcc+1);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  nv[srcc]=0;
  config->posv[config->posc++]=nv;
  return 0;
}

/* Apply argv. Include [0], we'll skip it.
 */
 
static int gp1_config_argv(struct gp1_config *config,int argc,char **argv) {
  int argp=1; while (argp<argc) {
    const char *arg=argv[argp++];
    if (!arg||!arg[0]) continue;
    
    if (arg[0]!='-') {
      if (gp1_config_positional(config,arg,-1)<0) return -1;
      continue;
    }
    
    if (!arg[1]) goto _unexpected_;
    
    if (arg[1]!='-') {
      int i=1; for (;arg[i];i++) {
        if (gp1_config_kv(config,arg+i,1,0,0)<0) return -1;
      }
      continue;
    }
    
    if (!arg[2]) goto _unexpected_;
    
    const char *k=arg+2,*v=0;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argp<argc)&&argv[argp]&&(argv[argp][0]!='-')) v=argv[argp++];
    if (gp1_config_kv(config,k,kc,v,-1)<0) return -1;
    continue;
    
   _unexpected_:;
    fprintf(stderr,"%s: Unexpected argument '%s'\n",config->exename,arg);
    return -1;
  }
  return 0;
}

/* Initial defaults, for anything nonzero.
 */
 
static int gp1_config_default(struct gp1_config *config) {
  config->audio_rate=44100;
  config->audio_chanc=2;
  return 0;
}

/* Validate config before reporting success.
 */
 
static int gp1_config_ready(struct gp1_config *config) {

  // Command unset? Infer if we can, otherwise it's "help".
  if (config->command==GP1_COMMAND_UNSET) {
    if (config->posc==1) config->command=GP1_COMMAND_run;
    else config->command=GP1_COMMAND_help;
  }
  
  return 0;
}

/* Init, the whole enchilada.
 */

int gp1_config_init(struct gp1_config *config,int argc,char **argv) {
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) {
    config->exename=argv[0];
  } else {
    config->exename="gp1";
  }
  if (gp1_config_default(config)<0) return -1;
  if (gp1_config_argv(config,argc,argv)<0) return -1;
  if (gp1_config_ready(config)<0) return -1;
  return 0;
}

/* Command names.
 */
 
int gp1_command_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return GP1_COMMAND_##tag;
  _(help)
  _(run)
  _(pack)
  _(convertchunk)
  _(info)
  #undef _
  return -1;
}

const char *gp1_command_repr(int command) {
  switch (command) {
  #define _(tag) case GP1_COMMAND_##tag: return #tag;
  _(help)
  _(run)
  _(pack)
  _(convertchunk)
  _(info)
  #undef _
  }
  return 0;
}
