#include "gp1_inmgr_internal.h"
#include "io/gp1_io_input.h"

/* Fake device iterator for the system keyboard.
 */
 
static int gp1_inmgr_iterate_system_keyboard(
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata
) {
  int usage,err;
  for (usage=0x00070004;usage<0x00070067;usage++) if (err=cb(0,0,usage,usage,0,0,2,userdata)) return err; //  04..66: The bulk of US keyboards and keypads.
  // Lots of defined but unusual codes 0x00070067..0x000700de, I'm skipping them.
  for (usage=0x000700e0;usage<0x000700e8;usage++) if (err=cb(0,0,usage,usage,0,0,2,userdata)) return err; // e0..e7: Modifier keys.
  return 0;
}

/* Hard-coded rules for the system keyboard, if we are autoconfigging it.
 * Hopefully this never comes up! We are not privy to host application actions like QUIT and FULLSCREEN, so we don't provide them.
 * Note that we do not use the Digits, F-keys, Escape, or Navigation keys, assuming they will be wanted for app actions.
 * The real "default keyboard mapping" is generated at compile time, see etc/tool/geninputcfg.sh.
 */
 
static struct gp1_keyboard_map {
  int usage;
  int btnid;
} gp1_keyboard_mapv[]={

  // left-hand dpad, traditional
  {0x0007001a,GP1_BTNID_UP}, // w
  {0x00070004,GP1_BTNID_LEFT}, // a
  {0x00070016,GP1_BTNID_DOWN}, // s
  {0x00070007,GP1_BTNID_RIGHT}, // d
  
  // arrow keys, obvious
  {0x00070050,GP1_BTNID_LEFT}, // left
  {0x0007004f,GP1_BTNID_RIGHT}, // right
  {0x00070052,GP1_BTNID_UP}, // up
  {0x00070051,GP1_BTNID_DOWN}, // down
  
  // keypad as dpad, obvious
  {0x00070060,GP1_BTNID_UP}, // 8
  {0x0007005c,GP1_BTNID_LEFT}, // 4
  {0x0007005d,GP1_BTNID_DOWN}, // 5
  {0x0007005e,GP1_BTNID_RIGHT}, // 6
  {0x0007005a,GP1_BTNID_DOWN}, // 2
  
  // right-hand thumb buttons, no reference for this but feels natural
  {0x00070037,GP1_BTNID_SOUTH}, // dot
  {0x0007000f,GP1_BTNID_WEST}, // l
  {0x00070033,GP1_BTNID_EAST}, // semicolon
  {0x00070013,GP1_BTNID_NORTH}, // p
  
  // left-hand thumb buttons where i like them (in a single row so as to avoid WASD)
  {0x0007001d,GP1_BTNID_SOUTH}, // z
  {0x0007001b,GP1_BTNID_WEST}, // x
  {0x00070006,GP1_BTNID_EAST}, // c
  {0x00070019,GP1_BTNID_NORTH}, // v
  
  // keypad thumb buttons, a rough guess
  {0x00070062,GP1_BTNID_SOUTH}, // 0
  {0x00070058,GP1_BTNID_WEST}, // enter
  {0x00070057,GP1_BTNID_EAST}, // plus
  {0x00070063,GP1_BTNID_NORTH}, // dot
  
  // aux buttons vomitted all over the keyboard
  {0x00070028,GP1_BTNID_AUX1}, // enter
  {0x0007002c,GP1_BTNID_AUX1}, // space
  {0x00070030,GP1_BTNID_AUX2}, // right bracket
  {0x0007002f,GP1_BTNID_AUX3}, // left bracket
  
  // triggers at the far corners of the keyboard
  {0x0007002b,GP1_BTNID_L1}, // tab
  {0x00070031,GP1_BTNID_R1}, // backslash
  {0x00070035,GP1_BTNID_L2}, // tilde
  {0x0007002a,GP1_BTNID_R2}, // backspace
  
  // keypad aux and triggers. they fit but it's not obvious
  {0x0007005f,GP1_BTNID_L1}, // 7
  {0x00070061,GP1_BTNID_R1}, // 9
  {0x00070059,GP1_BTNID_L2}, // 1
  {0x0007005b,GP1_BTNID_R2}, // 3
  {0x00070054,GP1_BTNID_AUX1}, // slash
  {0x00070055,GP1_BTNID_AUX2}, // star
  {0x00070056,GP1_BTNID_AUX3}, // dash
};

/* Mapping context.
 */
 
struct gp1_inmgr_map_ctx {
  struct gp1_inmgr *inmgr;
  struct gp1_inmgr_device *device;
  struct gp1_inmgr_rules *rules;
};

static void gp1_inmgr_map_ctx_cleanup(struct gp1_inmgr_map_ctx *ctx) {
}

/* Count maps by dstbtnid.
 */

static void gp1_inmgr_device_count_maps_by_dstbtnid(int *dst/*16*/,const struct gp1_inmgr_device *device) {
  const struct gp1_inmgr_device_button *button=device->buttonv;
  int i=device->buttonc;
  for (;i-->0;button++) {
    if (button->dstbtnid>=0x10000) continue;
    uint16_t bits=button->dstbtnid;
    int index=0;
    while (bits) {
      if (bits&1) dst[index]++;
      bits>>=1;
      index++;
    }
  }
}

/* Autoconfig a button.
 */
 
static int gp1_inmgr_rules_guess_button(struct gp1_inmgr_map_ctx *ctx,int btnid,int usage,int lo,int value,int hi) {

  // Range less than 2, we're not interested.
  if (lo>=hi) return 0;
  
  // Already got it? OK, do nothing.
  int dp=gp1_inmgr_device_button_search(ctx->device,btnid);
  if (dp>=0) return 0;
  dp=-dp-1;

  // Consider explicit usage-to-button rules.
  int dstbtnid=0;
  switch (usage) {
    case 0x00010030: dstbtnid=GP1_BTNID_HORZ; break; // x
    case 0x00010031: dstbtnid=GP1_BTNID_VERT; break; // y
    case 0x00010033: dstbtnid=GP1_BTNID_HORZ; break; // rx
    case 0x00010034: dstbtnid=GP1_BTNID_VERT; break; // ry
    case 0x00010039: dstbtnid=GP1_BTNID_DPAD; break; // hat
    case 0x0001003d: dstbtnid=GP1_BTNID_AUX1; break; // start
    case 0x0001003e: dstbtnid=GP1_BTNID_AUX2; break; // select
    case 0x00010085: dstbtnid=GP1_BTNID_AUX3; break; // system main menu
    case 0x00010086: dstbtnid=GP1_BTNID_AUX1; break; // system app menu
    case 0x00010089: dstbtnid=GP1_BTNID_AUX2; break; // system menu select
    case 0x0001008a: dstbtnid=GP1_BTNID_RIGHT; break; // system menu right
    case 0x0001008b: dstbtnid=GP1_BTNID_LEFT; break; // system menu left
    case 0x0001008c: dstbtnid=GP1_BTNID_UP; break; // system menu up
    case 0x0001008d: dstbtnid=GP1_BTNID_DOWN; break; // system menu down
    case 0x00010090: dstbtnid=GP1_BTNID_UP; break; // dpad up
    case 0x00010091: dstbtnid=GP1_BTNID_DOWN; break; // dpad down
    case 0x00010092: dstbtnid=GP1_BTNID_RIGHT; break; // dpad right
    case 0x00010093: dstbtnid=GP1_BTNID_LEFT; break; // dpad left
    case 0x00050020: break; // point of view
    case 0x00050021: dstbtnid=GP1_BTNID_HORZ; break; // turn right/left
    case 0x00050022: dstbtnid=GP1_BTNID_VERT; break; // pitch forward/backward
    case 0x00050023: dstbtnid=GP1_BTNID_HORZ; break; // roll right/left
    case 0x00050024: dstbtnid=GP1_BTNID_HORZ; break; // move right/left
    case 0x00050025: dstbtnid=GP1_BTNID_VERT; break; // move forward/backward
    case 0x00050026: dstbtnid=GP1_BTNID_VERT; break; // move up/down
    case 0x00050027: dstbtnid=GP1_BTNID_HORZ; break; // lean right/left
    case 0x00050028: dstbtnid=GP1_BTNID_VERT; break; // lean forward/backward
    case 0x00050037: dstbtnid=GP1_BTNID_THUMB; break; // gamepad fire/jump (ie thumb button)
    case 0x00050038: dstbtnid=GP1_BTNID_TRIGGER; break; // gamepad trigger (ie triggers)
  }
  if (dstbtnid) {
    goto _ready_;
  }
  
  // More explicit rules, this time for keyboards.
  if ((usage&0xffff0000)==0x00070000) {
    const struct gp1_keyboard_map *map=gp1_keyboard_mapv;
    int i=sizeof(gp1_keyboard_mapv)/sizeof(struct gp1_keyboard_map);
    for (;i-->0;map++) {
      if (map->usage==usage) {
        dstbtnid=map->btnid;
        goto _ready_;
      }
    }
  }
  
  // Page 9 is "generic buttons", that's a thing we can do.
  if ((usage&0xffff0000)==0x00090000) {
    dstbtnid=GP1_BTNID_BUTTON;
    goto _ready_;
  }
  
  // If it has a range at least 3, and a value inside the limits, call it HORZ or VERT.
  if ((lo<value)&&(value<hi)) {
    dstbtnid=GP1_BTNID_AXIS;
    goto _ready_;
  }
  
  // Everything else is "button".
  dstbtnid=GP1_BTNID_BUTTON;
  
 _ready_:;
  
  // AXIS, BUTTON, THUMB, TRIGGER, AUX all need to be made specific.
  // TODO This could be a lot of re-examine-the-whole-device. Could we store these count_by_btnid and work it incrementally?
  switch (dstbtnid) {
    case GP1_BTNID_AXIS: {
        int count_by_btnid[16]={0};
        gp1_inmgr_device_count_maps_by_dstbtnid(count_by_btnid,ctx->device);
        if (count_by_btnid[0]<=count_by_btnid[2]) { // left<=up
          dstbtnid=GP1_BTNID_HORZ;
        } else {
          dstbtnid=GP1_BTNID_VERT;
        }
      } break;
    case GP1_BTNID_BUTTON: {
        int count_by_btnid[16]={0};
        gp1_inmgr_device_count_maps_by_dstbtnid(count_by_btnid,ctx->device);
        int best=0,bestscore=INT_MAX,i;
        for (i=4;i<15;i++) {
          if (count_by_btnid[i]<bestscore) {
            best=i;
            bestscore=count_by_btnid[i];
            if (!bestscore) break;
          }
        }
        dstbtnid=1<<best;
      } break;
    case GP1_BTNID_THUMB: {
        int count_by_btnid[16]={0};
        gp1_inmgr_device_count_maps_by_dstbtnid(count_by_btnid,ctx->device);
        int best=0,bestscore=INT_MAX,i;
        for (i=4;i<8;i++) {
          if (count_by_btnid[i]<bestscore) {
            best=i;
            bestscore=i;
            if (!bestscore) break;
          }
        }
        dstbtnid=1<<best;
      } break;
    case GP1_BTNID_TRIGGER: {
        int count_by_btnid[16]={0};
        gp1_inmgr_device_count_maps_by_dstbtnid(count_by_btnid,ctx->device);
        int best=0,bestscore=INT_MAX,i;
        for (i=8;i<12;i++) {
          if (count_by_btnid[i]<bestscore) {
            best=i;
            bestscore=i;
            if (!bestscore) break;
          }
        }
        dstbtnid=1<<best;
      } break;
    case GP1_BTNID_AUX: {
        int count_by_btnid[16]={0};
        gp1_inmgr_device_count_maps_by_dstbtnid(count_by_btnid,ctx->device);
        int best=0,bestscore=INT_MAX,i;
        for (i=12;i<15;i++) {
          if (count_by_btnid[i]<bestscore) {
            best=i;
            bestscore=i;
            if (!bestscore) break;
          }
        }
        dstbtnid=1<<best;
      } break;
  }
  
  // DPAD rules must cover a range of exactly 8.
  if (dstbtnid==GP1_BTNID_DPAD) {
    if (lo+7!=hi) {
      int vid,pid;
      const char *name=gp1_input_device_get_ids(&vid,&pid,ctx->device->input,ctx->device->devid);
      fprintf(stderr,
        "WARNING: Invalid DPAD rule for button 0x%08x on device '%s'. Range %d..%d must cover exactly 8 values.\n",
        btnid,name,lo,hi
      );
      return 0;
    }
  }
  
  // Special handling for aggregate rules HORZ and VERT.
  // DPAD should enter the device as such.
  if ((dstbtnid==GP1_BTNID_HORZ)||(dstbtnid==GP1_BTNID_VERT)) {
    // Add two rules in the same place (sorted by btnid but adjacent duplicates are allowed).
    struct gp1_inmgr_device_button *buttonlo=gp1_inmgr_device_button_insert2(ctx->device,dp,btnid);
    if (!buttonlo) return -1;
    struct gp1_inmgr_device_button *buttonhi=buttonlo+1;
    buttonlo->srclo=INT_MIN;
    buttonhi->srchi=INT_MAX;
    int mid=(lo+hi)>>1;
    buttonlo->srchi=(lo+mid)>>1;
    buttonhi->srclo=(mid+hi)>>1;
    if (buttonlo->srchi>=mid) buttonlo->srchi--;
    if (buttonhi->srclo<=mid) buttonhi->srclo++;
    if (dstbtnid==GP1_BTNID_HORZ) {
      buttonlo->dstbtnid=GP1_BTNID_LEFT;
      buttonhi->dstbtnid=GP1_BTNID_RIGHT;
    } else {
      buttonlo->dstbtnid=GP1_BTNID_UP;
      buttonhi->dstbtnid=GP1_BTNID_DOWN;
    }
    return 0;
  }
  
  // Add the rule as stated.
  struct gp1_inmgr_device_button *button=gp1_inmgr_device_button_insert(ctx->device,dp,btnid);
  if (!button) return -1;
  button->dstbtnid=dstbtnid;
  if (dstbtnid==GP1_BTNID_DPAD) {
    button->srclo=lo;
    button->srchi=hi;
  } else if (lo==0) {
    button->srclo=1;
    button->srchi=INT_MAX;
  } else {
    button->srclo=(lo+hi)>>1;
    button->srchi=INT_MAX;
  }
  
  return 0;
}

/* Callback for iterating device capabilities (apply).
 */
 
static int gp1_inmgr_rules_apply_cb(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata) {
  struct gp1_inmgr_map_ctx *ctx=userdata;
  
  // Must have a positive range.
  if (lo>=hi) return 0;
  
  // If the device already has this button, something is wrong but ignore it.
  int dp=gp1_inmgr_device_button_search(ctx->device,btnid);
  if (dp>=0) return 0;
  dp=-dp-1;

  // Search for (btnid) first, then (usage). Use the first to match.
  // Drivers are encouraged to define their (btnid) such that it doesn't overlap reasonable USB-HID codes.
  int rp=gp1_inmgr_rules_button_search(ctx->rules,btnid);
  if (rp<0) rp=gp1_inmgr_rules_button_search(ctx->rules,usage);
  
  // If there's no explicit rule, check (autoconfig). Either guess or ignore it.
  if (rp<0) {
    if (ctx->rules->autoconfig) {
      return gp1_inmgr_rules_guess_button(ctx,btnid,usage,lo,value,hi);
    }
    return 0;
  }
  const struct gp1_inmgr_rules_button *rbutton=ctx->rules->buttonv+rp;
  
  // Special handling for aggregate rules HORZ and VERT.
  // DPAD should enter the device as such.
  if ((rbutton->dstbtnid==GP1_BTNID_HORZ)||(rbutton->dstbtnid==GP1_BTNID_VERT)) {
    // Add two rules in the same place (sorted by btnid but adjacent duplicates are allowed).
    struct gp1_inmgr_device_button *buttonlo=gp1_inmgr_device_button_insert2(ctx->device,dp,btnid);
    if (!buttonlo) return -1;
    struct gp1_inmgr_device_button *buttonhi=buttonlo+1;
    buttonlo->srclo=INT_MIN;
    buttonhi->srchi=INT_MAX;
    if (rbutton->srchi>rbutton->srclo) {
      buttonlo->srchi=rbutton->srclo;
      buttonhi->srclo=rbutton->srchi;
    } else if (rbutton->srchi==rbutton->srclo) {
      buttonlo->srchi=rbutton->srclo-1;
      buttonhi->srchi=rbutton->srchi+1;
    } else {
      int mid=(lo+hi)>>1;
      buttonlo->srchi=(lo+mid)>>1;
      buttonhi->srclo=(mid+hi)>>1;
      if (buttonlo->srchi>=mid) buttonlo->srchi--;
      if (buttonhi->srclo<=mid) buttonhi->srclo++;
    }
    if (rbutton->dstbtnid==GP1_BTNID_HORZ) {
      buttonlo->dstbtnid=GP1_BTNID_LEFT;
      buttonhi->dstbtnid=GP1_BTNID_RIGHT;
    } else {
      buttonlo->dstbtnid=GP1_BTNID_UP;
      buttonhi->dstbtnid=GP1_BTNID_DOWN;
    }
    return 0;
  }
  
  // Add the rule as stated.
  struct gp1_inmgr_device_button *button=gp1_inmgr_device_button_insert(ctx->device,dp,btnid);
  if (!button) return -1;
  button->dstbtnid=rbutton->dstbtnid;
  
  // If the rule provides an explicit range, use it. Otherwise infer from what's reported here.
  if (rbutton->srchi>=rbutton->srclo) {
    button->srclo=rbutton->srclo;
    button->srchi=rbutton->srchi;
  } else if (button->dstbtnid==GP1_BTNID_DPAD) {
    button->srclo=lo;
    button->srchi=hi;
  } else {
    if (lo==0) {
      button->srclo=1;
      button->srchi=INT_MAX;
    } else {
      button->srclo=(lo+hi)>>1;
      button->srchi=INT_MAX;
    }
  }
  
  // One last thing: If it's a DPAD rule, the source range must cover 8 values. If not, issue a warning and drop the rule.
  if (button->dstbtnid==GP1_BTNID_DPAD) {
    if (button->srclo+7!=button->srchi) {
      int vid,pid;
      const char *name=gp1_input_device_get_ids(&vid,&pid,input,devid);
      fprintf(stderr,
        "WARNING: Invalid DPAD rule for button 0x%08x on device '%s'. Range %d..%d must cover exactly 8 values.\n",
        btnid,name,button->srclo,button->srchi
      );
      gp1_inmgr_device_remove_button(ctx->device,dp);
      return 0;
    }
  }
  
  return 0;
}

/* Apply rules.
 */
 
int gp1_inmgr_rules_apply(struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device,struct gp1_inmgr_rules *rules) {
  
  /* There should be no buttons on (device) initially.
   * If there are, blindly drop them.
   */
  device->buttonc=0;
  
  /* We might have (autoconfig) enabled, and there might be buttons that require the device's declared input range.
   * In theory, we could check (rules) and if neither of those is the case, map blindly without any I/O.
   * I don't think that's worth the trouble.
   * So we will let gp1_input_device_iterate() drive the effort.
   */
  struct gp1_inmgr_map_ctx ctx={
    .inmgr=inmgr,
    .device=device,
    .rules=rules,
  };
  if (device->input) {
    if (gp1_input_device_iterate(device->input,device->devid,gp1_inmgr_rules_apply_cb,&ctx)<0) return -1;
  } else {
    if (gp1_inmgr_iterate_system_keyboard(gp1_inmgr_rules_apply_cb,&ctx)<0) return -1;
  }
  
  if (rules->playerid) {
    device->playerid=rules->playerid;
  }
  
  return 0;
}

/* Callback for iterating device capabilities (autoconfig).
 */
 
static int gp1_inmgr_rules_autoconfig_cb(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata) {
  struct gp1_inmgr_map_ctx *ctx=userdata;
  
  // Ignore if the range is foul or we already have this button.
  if (lo>=hi) return 0;
  int dp=gp1_inmgr_device_button_search(ctx->device,btnid);
  if (dp>=0) return 0;
  dp=-dp-1;
  
  return gp1_inmgr_rules_guess_button(ctx,btnid,usage,lo,value,hi);
}

/* Autoconfig.
 */
 
int gp1_inmgr_autoconfig(struct gp1_inmgr_rules *rules,struct gp1_inmgr *inmgr,struct gp1_inmgr_device *device) {

  // Set rules matching criteria.
  if (device->input) {
    const char *name=gp1_input_device_get_ids(&rules->vid,&rules->pid,device->input,device->devid);
    if (name&&name[0]) {
      int namec=0; while (name[namec]) namec++;
      if (rules->name) free(rules->name);
      if (!(rules->name=malloc(namec+1))) return -1;
      memcpy(rules->name,name,namec);
      rules->name[namec]=0;
      rules->namec=namec;
    }
  } else {
    rules->vid=0x10000;
    rules->pid=0x10000;
  }

  //TODO add buttons to (rules) as we select them
  struct gp1_inmgr_map_ctx ctx={
    .inmgr=inmgr,
    .device=device,
    .rules=rules,
  };
  if (device->input) {
    if (gp1_input_device_iterate(device->input,device->devid,gp1_inmgr_rules_autoconfig_cb,&ctx)<0) return -1;
  } else {
    // No input device means it's the system keyboard.
    // We have a fake iterator, could do this generically, but really don't need to:
    const struct gp1_keyboard_map *map=gp1_keyboard_mapv;
    int i=sizeof(gp1_keyboard_mapv)/sizeof(struct gp1_keyboard_map);
    for (;i-->0;map++) {
      int p=gp1_inmgr_device_button_search(device,map->usage);
      if (p>=0) continue;
      p=-p-1;
      struct gp1_inmgr_device_button *button=gp1_inmgr_device_button_insert(device,p,map->usage);
      if (!button) return -1;
      button->dstbtnid=map->btnid;
      button->srclo=1;
      button->srchi=INT_MAX;
    }
  }
  return 0;
}
