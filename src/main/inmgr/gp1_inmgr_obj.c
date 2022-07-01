#include "gp1_inmgr_internal.h"
#include "io/gp1_serial.h"
#include "io/gp1_io_fs.h"

/* Delete.
 */

void gp1_inmgr_del(struct gp1_inmgr *inmgr) {
  if (!inmgr) return;
  
  if (inmgr->devicev) {
    while (inmgr->devicec-->0) {
      gp1_inmgr_device_cleanup(inmgr->devicev+inmgr->devicec);
    }
    free(inmgr->devicev);
  }
  
  if (inmgr->rulesv) {
    while (inmgr->rulesc-->0) {
      gp1_inmgr_rules_cleanup(inmgr->rulesv+inmgr->rulesc);
    }
    free(inmgr->rulesv);
  }
  
  free(inmgr);
}

/* New.
 */

struct gp1_inmgr *gp1_inmgr_new(const struct gp1_inmgr_delegate *delegate) {
  struct gp1_inmgr *inmgr=calloc(1,sizeof(struct gp1_inmgr));
  if (!inmgr) return 0;
  if (delegate) inmgr->delegate=*delegate;
  return inmgr;
}

/* Trivial accessors.
 */

void *gp1_inmgr_get_userdata(const struct gp1_inmgr *inmgr) {
  if (!inmgr) return 0;
  return inmgr->delegate.userdata;
}

/* Configure.
 */
 
int gp1_inmgr_configure_text(struct gp1_inmgr *inmgr,const char *src,int srcc,const char *refname) {
  while (inmgr->rulesc>0) {
    inmgr->rulesc--;
    gp1_inmgr_rules_cleanup(inmgr->rulesv+inmgr->rulesc);
  }
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct gp1_decoder decoder={.src=src,.srcc=srcc};
  if (gp1_inmgr_decode_rules(inmgr,&decoder,refname)<0) return -1;
  return 0;
}

int gp1_inmgr_configure_file(struct gp1_inmgr *inmgr,const char *path,int require) {
  char *src=0;
  int srcc=gp1_file_read(&src,path);
  if (srcc<0) {
    if (require) {
      fprintf(stderr,"%s: Failed to read input config (file not found or permission denied)\n",path);
      return -1;
    }
    return 0;
  }
  int err=gp1_inmgr_configure_text(inmgr,src,srcc,path);
  free(src);
  if (err<0) return err;
  return 1;
}
