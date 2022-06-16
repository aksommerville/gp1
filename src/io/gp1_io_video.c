#include "gp1_io_video.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/* Type registry.
 *************************************************************/

extern const struct gp1_video_type gp1_video_type_glx;
extern const struct gp1_video_type gp1_video_type_drm;
extern const struct gp1_video_type gp1_video_type_macwm;
extern const struct gp1_video_type gp1_video_type_mswm;

static const struct gp1_video_type *gp1_video_typev[]={
#if GP1_USE_glx
  &gp1_video_type_glx,
#endif
#if GP1_USE_drm
  &gp1_video_type_drm,
#endif
#if GP1_USE_macwm
  &gp1_video_type_macwm,
#endif
#if GP1_USE_mswm
  &gp1_video_type_mswm,
#endif
};

const struct gp1_video_type *gp1_video_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(gp1_video_typev)/sizeof(void*);
  if (p>=c) return 0;
  return gp1_video_typev[p];
}

const struct gp1_video_type *gp1_video_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  int i=sizeof(gp1_video_typev)/sizeof(void*);
  const struct gp1_video_type **p=gp1_video_typev;
  for (;i-->0;p++) {
    if (memcmp(name,(*p)->name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Driver wrapper.
 ******************************************************************/

void gp1_video_del(struct gp1_video *video) {
  if (!video) return;
  if (video->refc-->1) return;
  if (video->type->del) video->type->del(video);
  free(video);
}

int gp1_video_ref(struct gp1_video *video) {
  if (!video) return -1;
  if (video->refc<1) return -1;
  if (video->refc==INT_MAX) return -1;
  video->refc++;
  return 0;
}

struct gp1_video *gp1_video_new(
  const struct gp1_video_type *type,
  const struct gp1_video_delegate *delegate,
  int w,int h,int rate,int fullscreen,
  const char *title,int titlec,
  const void *icon_rgba,int iconw,int iconh
) {
  if (!type) return 0;
  struct gp1_video *video=calloc(1,type->objlen);
  if (!video) return 0;
  
  video->refc=1;
  video->type=type;
  if (delegate) video->delegate=*delegate;
  video->w=w;
  video->h=h;
  video->rate=rate;
  video->fullscreen=fullscreen;
  
  if (type->init&&(type->init(video,title,titlec,icon_rgba,iconw,iconh)<0)) {
    gp1_video_del(video);
    return 0;
  }
  
  return video;
}

int gp1_video_update(struct gp1_video *video) {
  if (!video) return -1;
  if (!video->type->update) return 0;
  return video->type->update(video);
}

int gp1_video_swap(struct gp1_video *video) {
  if (!video) return -1;
  if (!video->type->swap) return 0;
  return video->type->swap(video);
}

int gp1_video_set_fullscreen(struct gp1_video *video,int fullscreen) {
  if (!video) return -1;
  if (!video->type->set_fullscreen) return video->fullscreen;
  return video->type->set_fullscreen(video,fullscreen);
}

void gp1_video_suppress_screensaver(struct gp1_video *video) {
  if (!video) return;
  if (!video->type->suppress_screensaver) return;
  video->type->suppress_screensaver(video);
}
