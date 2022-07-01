#include "gp1_inmgr_internal.h"

/* Cleanup.
 */
 
void gp1_inmgr_device_cleanup(struct gp1_inmgr_device *device) {
  if (device->buttonv) free(device->buttonv);
}

/* Search devices.
 */
 
int gp1_inmgr_device_search(const struct gp1_inmgr *inmgr,const struct gp1_input *input,int devid) {
  int lo=0,hi=inmgr->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (input<inmgr->devicev[ck].input) hi=ck;
    else if (input>inmgr->devicev[ck].input) lo=ck+1;
    else if (devid<inmgr->devicev[ck].devid) hi=ck;
    else if (devid>inmgr->devicev[ck].devid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add device.
 */
 
struct gp1_inmgr_device *gp1_inmgr_device_add(struct gp1_inmgr *inmgr,int p,struct gp1_input *input,int devid) {
  
  if (p<0) {
    p=gp1_inmgr_device_search(inmgr,input,devid);
    if (p>=0) return 0;
    p=-p-1;
  }
  if ((p<0)||(p>inmgr->devicec)) return 0;
  if (p) {
    struct gp1_inmgr_device *q=inmgr->devicev+p-1;
    if (input<q->input) return 0;
    if ((input==q->input)&&(devid<=q->devid)) return 0;
  }
  if (p<inmgr->devicec) {
    struct gp1_inmgr_device *q=inmgr->devicev+p;
    if (input>q->input) return 0;
    if ((input==q->input)&&(devid<=q->devid)) return 0;
  }
  
  if (inmgr->devicec>=inmgr->devicea) {
    int na=inmgr->devicea+8;
    if (na>INT_MAX/sizeof(struct gp1_inmgr_device)) return 0;
    void *nv=realloc(inmgr->devicev,sizeof(struct gp1_inmgr_device)*na);
    if (!nv) return 0;
    inmgr->devicev=nv;
    inmgr->devicea=na;
  }
  
  struct gp1_inmgr_device *device=inmgr->devicev+p;
  memmove(device+1,device,sizeof(struct gp1_inmgr_device)*(inmgr->devicec-p));
  inmgr->devicec++;
  memset(device,0,sizeof(struct gp1_inmgr_device));
  device->input=input;
  device->devid=devid;
  return device;
}

/* Remove.
 */
 
void gp1_inmgr_device_remove(struct gp1_inmgr *inmgr,int p) {
  if ((p<0)||(p>=inmgr->devicec)) return;
  struct gp1_inmgr_device *device=inmgr->devicev+p;
  gp1_inmgr_device_cleanup(device);
  inmgr->devicec--;
  memmove(device,device+1,sizeof(struct gp1_inmgr_device)*(inmgr->devicec-p));
}

/* Get device.
 */
 
struct gp1_inmgr_device *gp1_inmgr_device_get(const struct gp1_inmgr *inmgr,const struct gp1_input *input,int devid) {
  int p=gp1_inmgr_device_search(inmgr,input,devid);
  if (p<0) return 0;
  return inmgr->devicev+p;
}

/* Search buttons.
 */
 
int gp1_inmgr_device_button_search(const struct gp1_inmgr_device *device,int srcbtnid) {
  int lo=0,hi=device->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (srcbtnid<device->buttonv[ck].srcbtnid) hi=ck;
    else if (srcbtnid>device->buttonv[ck].srcbtnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add button.
 */
 
struct gp1_inmgr_device_button *gp1_inmgr_device_button_insert(struct gp1_inmgr_device *device,int p,int srcbtnid) {
  if ((p<0)||(p>device->buttonc)) return 0;
  // sic '<' and '>': Adjacent duplicates are permitted.
  if (p&&(srcbtnid<device->buttonv[p-1].srcbtnid)) return 0;
  if ((p<device->buttonc)&&(srcbtnid>device->buttonv[p].srcbtnid)) return 0;
  
  if (device->buttonc>=device->buttona) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct gp1_inmgr_device_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct gp1_inmgr_device_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  
  struct gp1_inmgr_device_button *button=device->buttonv+p;
  memmove(button+1,button,sizeof(struct gp1_inmgr_device_button)*(device->buttonc-p));
  device->buttonc++;
  memset(button,0,sizeof(struct gp1_inmgr_device_button));
  button->srcbtnid=srcbtnid;
  return button;
}
 
struct gp1_inmgr_device_button *gp1_inmgr_device_button_insert2(struct gp1_inmgr_device *device,int p,int srcbtnid) {
  if ((p<0)||(p>device->buttonc)) return 0;
  // sic '<' and '>': Adjacent duplicates are permitted.
  if (p&&(srcbtnid<device->buttonv[p-1].srcbtnid)) return 0;
  if ((p<device->buttonc)&&(srcbtnid>device->buttonv[p].srcbtnid)) return 0;
  
  if (device->buttonc>device->buttona-2) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct gp1_inmgr_device_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct gp1_inmgr_device_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  
  struct gp1_inmgr_device_button *button=device->buttonv+p;
  memmove(button+2,button,sizeof(struct gp1_inmgr_device_button)*(device->buttonc-p));
  device->buttonc+=2;
  memset(button,0,sizeof(struct gp1_inmgr_device_button)*2);
  button[0].srcbtnid=srcbtnid;
  button[1].srcbtnid=srcbtnid;
  return button;
}

/* Remove button.
 */
 
void gp1_inmgr_device_remove_button(struct gp1_inmgr_device *device,int p) {
  if ((p<0)||(p>=device->buttonc)) return;
  struct gp1_inmgr_device_button *button=device->buttonv+p;
  device->buttonc--;
  memmove(button,button+1,sizeof(struct gp1_inmgr_device_button)*(device->buttonc-p));
}
