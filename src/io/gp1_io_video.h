/* gp1_io_video.h
 * Generic interface to video drivers.
 * These may be video only, or a full window manager.
 */
 
#ifndef GP1_IO_VIDEO_H
#define GP1_IO_VIDEO_H

struct gp1_video;
struct gp1_video_type;

/* Be prepared for none of these ever to be called; some drivers aren't window managers.
 */
struct gp1_video_delegate {
  void *userdata;
  int (*cb_close)(struct gp1_video *video);
  int (*cb_focus)(struct gp1_video *video,int focus);
  int (*cb_resize)(struct gp1_video *video,int w,int h);
  
  // (keycode) should be USB-HID page 0x07. (value) 2 for auto-repeat if knowable.
  int (*cb_key)(struct gp1_video *video,int keycode,int value);
  
  // Unicode, one character at a time.
  int (*cb_text)(struct gp1_video *video,int codepoint);
  
  int (*cb_mmotion)(struct gp1_video *video,int x,int y);
  int (*cb_mbutton)(struct gp1_video *video,int btnid,int value);
  int (*cb_mwheel)(struct gp1_video *video,int dx,int dy);
};

struct gp1_video {
  const struct gp1_video_type *type;
  struct gp1_video_delegate delegate;
  int refc;
  int w,h,rate,fullscreen;
};

void gp1_video_del(struct gp1_video *video);
int gp1_video_ref(struct gp1_video *video);

/* All params are suggestions only, you'll get what you get.
 */
struct gp1_video *gp1_video_new(
  const struct gp1_video_type *type,
  const struct gp1_video_delegate *delegate,
  int w,int h,int rate,int fullscreen,
  const char *title,int titlec,
  const void *icon_rgba,int iconw,int iconh
);

int gp1_video_update(struct gp1_video *video);
int gp1_video_swap(struct gp1_video *video);

int gp1_video_set_fullscreen(struct gp1_video *video,int fullscreen);
void gp1_video_suppress_screensaver(struct gp1_video *video);

struct gp1_video_type {
  const char *name;
  const char *desc;
  int objlen;
  int provides_system_keyboard;
  void (*del)(struct gp1_video *video);
  
  int (*init)(struct gp1_video *video,
    const char *title,int titlec,
    const void *icon_rgba,int iconw,int iconh
  );
  
  int (*update)(struct gp1_video *video);
  int (*swap)(struct gp1_video *video);
  int (*set_fullscreen)(struct gp1_video *video,int fullscreen);
  void (*suppress_screensaver)(struct gp1_video *video);
};

const struct gp1_video_type *gp1_video_type_by_index(int p);
const struct gp1_video_type *gp1_video_type_by_name(const char *name,int namec);

#endif
