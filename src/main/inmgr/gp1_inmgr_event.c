#include "gp1_inmgr_internal.h"
#include "gp1/gp1.h"
#include "io/gp1_io_input.h"

/* Replace one state bit and fire callbacks.
 */
 
static int gp1_inmgr_replace_state(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_device *device,
  uint16_t btnid,
  int value
) {
  if (!btnid) return 0;
  if (value) {
    if (device->state&btnid) return 0;
    device->state|=btnid;
    value=1;
  } else {
    if (!(device->state&btnid)) return 0;
    device->state&=~btnid;
  }
  if (!inmgr->delegate.state) return 0;
  return inmgr->delegate.state(inmgr,device->playerid,btnid,value);
}

/* Select a playerid to assign to a new device.
 */
 
static int gp1_inmgr_least_used_playerid(const struct gp1_inmgr *inmgr) {
  int count_by_playerid[GP1_PLAYER_COUNT]={0};
  const struct gp1_inmgr_device *device=inmgr->devicev;
  int i=inmgr->devicec;
  for (;i-->0;device++) {
    if (!device->input) continue;
    count_by_playerid[device->playerid]++;
  }
  int lowest_count=count_by_playerid[1];
  int lowest_playerid=1;
  for (i=2;i<=GP1_PLAYER_LAST;i++) {
    if (count_by_playerid[i]<lowest_count) { // sic '<', ties break low (it matters)
      lowest_count=count_by_playerid[i];
      lowest_playerid=i;
    }
  }
  return lowest_playerid;
}

/* Find and apply rules for a newly-connected device.
 */
 
static int gp1_inmgr_welcome_device(struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device) {
  
  int vid=0,pid=0;
  const char *name=0;
  if (device->input) {
    name=gp1_input_device_get_ids(&vid,&pid,device->input,device->devid);
  } else if (device->devid==0) {
    name="System Keyboard";
  } else {
    name="INVALID INPUT DEVICE";
    vid=pid=-1;
  }
  
  struct gp1_inmgr_rules *rules=gp1_inmgr_rules_find(inmgr,name,-1,vid,pid);
  if (rules) {
    if (gp1_inmgr_rules_apply(inmgr,device,rules)<0) return -1;
    fprintf(stderr,"Configured known input device %04x:%04x '%s'.\n",vid,pid,name);
  } else {
    rules=gp1_inmgr_rules_add(inmgr,0); // add to front
    if (gp1_inmgr_autoconfig(rules,inmgr,device)<0) return -1;
    fprintf(stderr,"Created default input map for unknown device %04x:%04x '%s'.\n",vid,pid,name);
  }
  
  if (!device->playerid) {
    if (rules->playerid) { // give what the configuration asks for, if nonzero.
      device->playerid=rules->playerid;
    } else if (device->input) { // assign sequentially, typical.
      device->playerid=gp1_inmgr_least_used_playerid(inmgr);
    } else { // system keyboard always gets player 1, and doesn't participate further in assignments.
      device->playerid=1;
    }
    fprintf(stderr,"Assigned device %04x:%04x '%s' to player %d\n",vid,pid,name,device->playerid);
  }
  
  if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_CD,1)<0) return -1;
  
  return 0;
}

/* Dpad maps.
 * This is for dpads that report as a single rotational control with 8 values and other OOB values for the zero state.
 * That is a fucktarded way to describe the control, and luckily most vendors don't use it.
 * Usually a dpad is two signed axes.
 */
 
static int gp1_dpad_x(int v) {
  switch (v) {
    case 0:
    case 4:
      return 0;
    case 1:
    case 2:
    case 3:
      return 1;
    case 5:
    case 6:
    case 7:
      return -1;
  }
  return 0;
}

static int gp1_dpad_y(int v) {
  switch (v) {
    case 0:
    case 1:
    case 7:
      return -1;
    case 2:
    case 6:
      return 0;
    case 3:
    case 4:
    case 5:
      return 1;
  }
  return 0;
}
 
static int gp1_inmgr_map_event_dpad(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_device *device,
  struct gp1_inmgr_device_button *button,
  int value
) {
  int dstvalue=value-button->srclo;
  if ((dstvalue<0)||(dstvalue>7)) dstvalue=-1;
  if (button->dstvalue==dstvalue) return 0;
  int pv=button->dstvalue;
  button->dstvalue=dstvalue;
  switch (gp1_dpad_x(value)) {
    case -1: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_RIGHT,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_LEFT,1)<0) return -1;
      } break;
    case 0: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_RIGHT,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_LEFT,0)<0) return -1;
      } break;
    case 1: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_LEFT,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_RIGHT,1)<0) return -1;
      } break;
  }
  switch (gp1_dpad_y(value)) {
    case -1: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_DOWN,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_UP,1)<0) return -1;
      } break;
    case 0: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_DOWN,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_UP,0)<0) return -1;
      } break;
    case 1: {
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_UP,0)<0) return -1;
        if (gp1_inmgr_replace_state(inmgr,device,GP1_BTNID_DOWN,1)<0) return -1;
      } break;
  }
  return 0;
}

/* Action maps.
 * Same idea as regular 2-state maps but we only fire the "on" events.
 */
 
static int gp1_inmgr_map_event_action(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_device *device,
  struct gp1_inmgr_device_button *button,
  int value
) {
  if ((value<button->srclo)||(value>button->srchi)) {
    button->dstvalue=0;
    return 0;
  }
  if (button->dstvalue) return 0;
  button->dstvalue=1;
  if (!inmgr->delegate.action) return 0;
  return inmgr->delegate.action(inmgr,button->dstbtnid);
}

/* State maps.
 * For typical 2-state buttons described by an "on" range in the source values, mapping to a state bit.
 */
 
static int gp1_inmgr_map_event_state(
  struct gp1_inmgr *inmgr,
  struct gp1_inmgr_device *device,
  struct gp1_inmgr_device_button *button,
  int value
) {
  int dstvalue=((value>=button->srclo)&&(value<=button->srchi))?1:0;
  if (dstvalue==button->dstvalue) return 0;
  button->dstvalue=dstvalue;
  return gp1_inmgr_replace_state(inmgr,device,button->dstbtnid,dstvalue);
}

/* Map event and fire callbacks.
 */
 
int gp1_inmgr_device_event(struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device,int btnid,int value) {
  //fprintf(stderr,"%s %s:%d.0x%08x=%d\n",__func__,device->input?device->input->type->name:"(null)",device->devid,btnid,value);
  int p=gp1_inmgr_device_button_search(device,btnid);
  if (p<0) return 0;
  struct gp1_inmgr_device_button *button=device->buttonv+p;
  while (p&&(button[-1].srcbtnid==btnid)) { p--; button--; }
  for (;(p<device->buttonc)&&(button->srcbtnid==btnid);p++,button++) {
    if (button->srcvalue==value) continue;
    button->srcvalue=value;
    if (button->dstbtnid==GP1_BTNID_DPAD) {
      if (gp1_inmgr_map_event_dpad(inmgr,device,button,value)<0) return -1;
    } else if (button->dstbtnid>=0x10000) {
      if (gp1_inmgr_map_event_action(inmgr,device,button,value)<0) return -1;
    } else {
      if (gp1_inmgr_map_event_state(inmgr,device,button,value)<0) return -1;
    }
  }
  return 0;
}

/* Drop state from a newly-disconnected device.
 */
 
int gp1_inmgr_drop_state(struct gp1_inmgr *inmgr,int playerid,uint16_t state) {
  state&=~GP1_BTNID_CD;
  if (!inmgr->delegate.state) return 0;
  uint16_t btnid=0x8000;
  for (;btnid;btnid>>=1) {
    if (state&btnid) {
      if (inmgr->delegate.state(inmgr,playerid,btnid,0)<0) return -1;
    }
  }
  struct gp1_inmgr_device *device=inmgr->devicev;
  int i=inmgr->devicec;
  for (;i-->0;device++) {
    if (device->playerid!=playerid) continue;
    return 0; // ok CD still on
  }
  if (inmgr->delegate.state(inmgr,playerid,GP1_BTNID_CD,0)<0) return -1;
  return 0;
}

/* "Connect" or "disconnect" the system keyboard.
 */
 
int gp1_inmgr_has_system_keyboard(struct gp1_inmgr *inmgr,int has) {
  int p=gp1_inmgr_device_search(inmgr,0,0);
  if (p>=0) {
    if (has) return 0;
    int playerid=inmgr->devicev[p].playerid;
    uint16_t state=inmgr->devicev[p].state;
    gp1_inmgr_device_remove(inmgr,p);
    return gp1_inmgr_drop_state(inmgr,playerid,state);
  } else {
    if (!has) return 0;
    p=-p-1;
    struct gp1_inmgr_device *device=gp1_inmgr_device_add(inmgr,p,0,0);
    if (!device) return -1;
    if (gp1_inmgr_welcome_device(inmgr,device)<0) return -1;
  }
  return 0;
}

/* System keyboard.
 */
 
int gp1_inmgr_key(struct gp1_inmgr *inmgr,int keycode,int value) {
  struct gp1_inmgr_device *device=gp1_inmgr_device_get(inmgr,0,0);
  if (!device) return 0; // no big deal
  return gp1_inmgr_device_event(inmgr,device,keycode,value);
}

/* Device connected.
 */
 
int gp1_inmgr_connect(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid) {
  int p=gp1_inmgr_device_search(inmgr,driver,devid);
  if (p>=0) return 0; // already have it, weird but ok
  p=-p-1;
  struct gp1_inmgr_device *device=gp1_inmgr_device_add(inmgr,p,driver,devid);
  if (!device) return -1;
  if (gp1_inmgr_welcome_device(inmgr,device)<0) return -1;
  return 0;
}

/* Device disconnected.
 */
 
int gp1_inmgr_disconnect(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid) {
  int p=gp1_inmgr_device_search(inmgr,driver,devid);
  if (p<0) return 0; // no big deal
  struct gp1_inmgr_device *device=inmgr->devicev+p;
  int playerid=device->playerid;
  uint16_t state=device->state;
  gp1_inmgr_device_remove(inmgr,p);
  if (gp1_inmgr_drop_state(inmgr,playerid,state)<0) return -1;
  return 0;
}

/* Button state changed.
 */
 
int gp1_inmgr_button(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid,int btnid,int value) {
  int p=gp1_inmgr_device_search(inmgr,driver,devid);
  if (p<0) return 0; // no big deal
  struct gp1_inmgr_device *device=inmgr->devicev+p;
  return gp1_inmgr_device_event(inmgr,device,btnid,value);
}
