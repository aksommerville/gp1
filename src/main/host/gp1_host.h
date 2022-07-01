/* gp1_host.h
 * Our main executable, the interactive runtime. (ie everything that isn't a command-line tool).
 */
 
#ifndef GP1_HOST_H
#define GP1_HOST_H

struct gp1_config;
struct gp1_rom;
struct gp1_vm;
struct gp1_video;
struct gp1_audio;
struct gp1_input;
struct gp1_renderer;
struct gp1_inmgr;

struct gp1_host {
// Caller sets directly, all WEAK:
  struct gp1_config *config;
  const char *rompath;
  struct gp1_rom *rom;
// Private:
  struct gp1_vm *vm;
  struct gp1_video *video;
  struct gp1_audio *audio;
  struct gp1_input **inputv;
  int inputc,inputa;
  struct gp1_renderer *renderer;
  int clockfaultc;
  int quit_requested;
  struct gp1_inmgr *inmgr;
  int suspend,step;
};

void gp1_host_cleanup(struct gp1_host *host);

// Modal, runs the whole thing. We log errors.
int gp1_host_run(struct gp1_host *host);

/* Action IDs are always larger than 16 bits, so they can't overlap button IDs.
 */
#define GP1_ACTION_QUIT             0x00010000
#define GP1_ACTION_RESET            0x00010001
#define GP1_ACTION_SUSPEND          0x00010002 /* hard pause */
#define GP1_ACTION_RESUME           0x00010003
#define GP1_ACTION_SCREENCAP        0x00010004
#define GP1_ACTION_LOAD_STATE       0x00010005
#define GP1_ACTION_SAVE_STATE       0x00010006
#define GP1_ACTION_MENU             0x00010007 /* suspend and present a menu */
#define GP1_ACTION_STEP_FRAME       0x00010008 /* advance one video frame, while suspended */
#define GP1_ACTION_FULLSCREEN       0x00010009 /* toggle */

#define GP1_FOR_EACH_ACTION \
  _(QUIT) \
  _(RESET) \
  _(SUSPEND) \
  _(RESUME) \
  _(SCREENCAP) \
  _(LOAD_STATE) \
  _(SAVE_STATE) \
  _(MENU) \
  _(STEP_FRAME) \
  _(FULLSCREEN)

int gp1_host_action_eval(const char *src,int srcc);
int gp1_host_action_repr(char *dst,int dsta,int action);

#endif
