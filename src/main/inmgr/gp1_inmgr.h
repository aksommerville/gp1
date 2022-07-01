/* gp1_inmgr.h
 * General input manager.
 * We are savvy to libgp1io's input drivers on one end, and libgp1vm's button IDs on the other.
 * We do have a concept of system keyboard, but no concept of mouse or encoded text.
 */
 
#ifndef GP1_INMGR_H
#define GP1_INMGR_H

struct gp1_inmgr;
struct gp1_input;

struct gp1_inmgr_delegate {
  void *userdata;
  
  /* Report some change of state in a mapped player control.
   * We track state at the input device level, but not per player, so redundant events are possible.
   */
  int (*state)(struct gp1_inmgr *inmgr,int playerid,int btnid,int value);
  
  int (*action)(struct gp1_inmgr *inmgr,int action);
  
  /* Button IDs to or from string.
   * You don't need to bother with the standard GP1 buttons, we know those.
   * Your Action IDs probably should have names.
   */
  int (*btnid_eval)(struct gp1_inmgr *inmgr,const char *src,int srcc);
  int (*btnid_repr)(char *dst,int dsta,struct gp1_inmgr *inmgr,int btnid);
};

void gp1_inmgr_del(struct gp1_inmgr *inmgr);

struct gp1_inmgr *gp1_inmgr_new(const struct gp1_inmgr_delegate *delegate);

/* Drop all rules and reconfigure from a file or loose text.
 * See etc/doc/input-config.txt for format.
 * The 'file' version logs all errors; 'text' logs if you supply a name.
 * ...unless (!require), then only syntax errors are real.
 */
int gp1_inmgr_configure_text(struct gp1_inmgr *inmgr,const char *src,int srcc,const char *refname);
int gp1_inmgr_configure_file(struct gp1_inmgr *inmgr,const char *path,int require);

void *gp1_inmgr_get_userdata(const struct gp1_inmgr *inmgr);

int gp1_inmgr_has_system_keyboard(struct gp1_inmgr *inmgr,int has);
int gp1_inmgr_key(struct gp1_inmgr *inmgr,int keycode,int value);
int gp1_inmgr_connect(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid);
int gp1_inmgr_disconnect(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid);
int gp1_inmgr_button(struct gp1_inmgr *inmgr,struct gp1_input *driver,int devid,int btnid,int value);

int gp1_btnid_eval(const char *src,int srcc);
const char *gp1_btnid_repr(int btnid);

#endif
