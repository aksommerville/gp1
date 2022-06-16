#include "gp1_io_input.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Type registry.
 ***********************************************/
 
extern const struct gp1_input_type gp1_input_type_evdev;
extern const struct gp1_input_type gp1_input_type_machid;
extern const struct gp1_input_type gp1_input_type_mshid;

static const struct gp1_input_type *gp1_input_typev[]={
#if GP1_USE_evdev
  &gp1_input_type_evdev,
#endif
#if GP1_USE_machid
  &gp1_input_type_machid,
#endif
#if GP1_USE_mshid
  &gp1_input_type_mshid,
#endif
};

static int gp1_input_next_devid=1;

const struct gp1_input_type *gp1_input_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(gp1_input_typev)/sizeof(void*);
  if (p>=c) return 0;
  return gp1_input_typev[p];
}

const struct gp1_input_type *gp1_input_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  const struct gp1_input_type **p=gp1_input_typev;
  int i=sizeof(gp1_input_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

int gp1_input_unused_devid() {
  if (gp1_input_next_devid>=INT_MAX) return -1;
  return gp1_input_next_devid++;
}

/* Generic driver wrapper.
 *****************************************************************/

void gp1_input_del(struct gp1_input *input) {
  if (!input) return;
  if (input->refc-->1) return;
  if (input->type->del) input->type->del(input);
  free(input);
}

int gp1_input_ref(struct gp1_input *input) {
  if (!input) return -1;
  if (input->refc<1) return -1;
  if (input->refc==INT_MAX) return -1;
  input->refc++;
  return 0;
}

struct gp1_input *gp1_input_new(
  const struct gp1_input_type *type,
  const struct gp1_input_delegate *delegate
) {
  if (!type) return 0;
  struct gp1_input *input=calloc(1,type->objlen);
  if (!input) return 0;
  input->type=type;
  input->refc=1;
  if (delegate) input->delegate=*delegate;
  if (type->init&&(type->init(input)<0)) {
    gp1_input_del(input);
    return 0;
  }
  return input;
}

int gp1_input_update(struct gp1_input *input) {
  if (!input) return -1;
  if (!input->type->update) return 0;
  return input->type->update(input);
}

int gp1_input_device_disconnect(struct gp1_input *input,int devid) {
  if (!input) return -1;
  if (!input->type->device_disconnect) return -1;
  return input->type->device_disconnect(input,devid);
}

const char *gp1_input_device_get_ids(int *vid,int *pid,struct gp1_input *input,int devid) {
  if (!input) return 0;
  if (!input->type->device_get_ids) return 0;
  int _vid; if (!vid) vid=&_vid;
  int _pid; if (!pid) pid=&_pid;
  return input->type->device_get_ids(vid,pid,input,devid);
}

int gp1_input_device_iterate(
  struct gp1_input *input,int devid,
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata
) {
  if (!input) return -1;
  if (!cb) return -1;
  if (!input->type->device_iterate) return 0;
  return input->type->device_iterate(input,devid,cb,userdata);
}
