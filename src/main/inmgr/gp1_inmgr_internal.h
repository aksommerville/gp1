#ifndef GP1_INMGR_INTERNAL_H
#define GP1_INMGR_INTERNAL_H

#include "gp1_inmgr.h"
#include "gp1/gp1.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

struct gp1_decoder;

#define GP1_BTNID_HORZ (GP1_BTNID_LEFT|GP1_BTNID_RIGHT)
#define GP1_BTNID_VERT (GP1_BTNID_UP|GP1_BTNID_DOWN)
#define GP1_BTNID_AXIS (GP1_BTNID_LEFT|GP1_BTNID_UP) /* either axis, lazy pick */
#define GP1_BTNID_DPAD (GP1_BTNID_HORZ|GP1_BTNID_VERT) /* both axes, rotationally */
#define GP1_BTNID_THUMB (GP1_BTNID_SOUTH|GP1_BTNID_WEST|GP1_BTNID_EAST|GP1_BTNID_NORTH)
#define GP1_BTNID_TRIGGER (GP1_BTNID_L1|GP1_BTNID_R1|GP1_BTNID_L2|GP1_BTNID_R2)
#define GP1_BTNID_AUX (GP1_BTNID_AUX1|GP1_BTNID_AUX2|GP1_BTNID_AUX3)
#define GP1_BTNID_BUTTON (GP1_BTNID_THUMB|GP1_BTNID_TRIGGER|GP1_BTNID_AUX)

struct gp1_inmgr_device_button {
  int srcbtnid;
  int srcvalue; // most recent input
  int srclo,srchi;
  int dstbtnid; // single bit if <0x10000
  int dstvalue; // most recent output
};

struct gp1_inmgr_device {
  struct gp1_input *input; // WEAK; null for system keyboard
  int devid; // zero for system keyboard
  int playerid;
  uint16_t state;
  struct gp1_inmgr_device_button *buttonv;
  int buttonc,buttona;
};

struct gp1_inmgr_rules_button {
  int srcbtnid;
  int srclo,srchi; // (srchi<srclo) means unspecified, use the reported range
  int dstbtnid;
};

struct gp1_inmgr_rules {
  char *name; // gp1_pattern_match, or empty matches all
  int namec;
  int vid,pid; // zero matches all
  char *driver;
  int driverc;
  int valid; // zero if we know it will never match (eg driver not found)
  int autoconfig; // nonzero to infer mappings for buttons not listed in (buttonv), ie as if this rules didn't exist
  int playerid; // preferred assignment, zero means "don't care"
  struct gp1_inmgr_rules_button *buttonv;
  int buttonc,buttona;
};

struct gp1_inmgr {
  struct gp1_inmgr_delegate delegate;
  struct gp1_inmgr_device *devicev;
  int devicec,devicea;
  struct gp1_inmgr_rules *rulesv; // order matters
  int rulesc,rulesa;
};

void gp1_inmgr_device_cleanup(struct gp1_inmgr_device *device);
int gp1_inmgr_device_search(const struct gp1_inmgr *inmgr,const struct gp1_input *input,int devid);
struct gp1_inmgr_device *gp1_inmgr_device_add(struct gp1_inmgr *inmgr,int p,struct gp1_input *input,int devid);
void gp1_inmgr_device_remove(struct gp1_inmgr *inmgr,int p); // does not fire events or wipe state
struct gp1_inmgr_device *gp1_inmgr_device_get(const struct gp1_inmgr *inmgr,const struct gp1_input *input,int devid);
int gp1_inmgr_device_event(struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device,int btnid,int value); // fires events and everything
int gp1_inmgr_device_button_search(const struct gp1_inmgr_device *device,int srcbtnid); // may have neighbors both fore and aft
struct gp1_inmgr_device_button *gp1_inmgr_device_button_insert(struct gp1_inmgr_device *device,int p,int srcbtnid);
struct gp1_inmgr_device_button *gp1_inmgr_device_button_insert2(struct gp1_inmgr_device *device,int p,int srcbtnid); // hack to add 2 at once
void gp1_inmgr_device_remove_button(struct gp1_inmgr_device *device,int p);

void gp1_inmgr_rules_cleanup(struct gp1_inmgr_rules *rules);
struct gp1_inmgr_rules *gp1_inmgr_rules_find(const struct gp1_inmgr *inmgr,const char *name,int namec,int vid,int pid);
struct gp1_inmgr_rules *gp1_inmgr_rules_add(struct gp1_inmgr *inmgr,int p); // (p<0) to append
int gp1_inmgr_rules_button_search(const struct gp1_inmgr_rules *rules,int srcbtnid);
struct gp1_inmgr_rules_button *gp1_inmgr_rules_button_insert(struct gp1_inmgr_rules *rules,int p,int srcbtnid);

int gp1_inmgr_decode_rules(struct gp1_inmgr *inmgr,struct gp1_decoder *decoder,const char *path);

/* Call after removing a device to fire the appropriate state events.
 * We will also check the remaining devices and infer the correct CD value.
 */
int gp1_inmgr_drop_state(struct gp1_inmgr *inmgr,int playerid,uint16_t state);

/* "apply" creates buttons in the device according to the rules and some fuzzy autoconfig logic.
 * "autoconfig" queries device capabilities and applies a made-up rule set, which it also returns for your examination.
 * Don't call both; "apply" does all the "autoconfig" stuff if the rules tell it to.
 */
int gp1_inmgr_rules_apply(struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device,struct gp1_inmgr_rules *rules);
int gp1_inmgr_autoconfig(struct gp1_inmgr_rules *rules,struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device);

#endif
