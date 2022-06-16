#ifndef GP1_DRM_INTERNAL_H
#define GP1_DRM_INTERNAL_H

#include "io/gp1_io_video.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct gp1_drm_fb {
  uint32_t fbid;
  int handle;
  int size;
};

struct gp1_video_drm {
  struct gp1_video hdr;
  int fd;
  
  int mmw,mmh; // monitor's physical size
  drmModeModeInfo mode; // ...and more in that line
  int connid,encid,crtcid;
  drmModeCrtcPtr crtc_restore;
  
  int flip_pending;
  struct gp1_drm_fb fbv[2];
  int fbp;
  struct gbm_bo *bo_pending;
  int crtcunset;
  
  struct gbm_device *gbmdevice;
  struct gbm_surface *gbmsurface;
  EGLDisplay egldisplay;
  EGLContext eglcontext;
  EGLSurface eglsurface;
};

#define DRIVER ((struct gp1_video_drm*)driver)

int gp1_drm_open_file(struct gp1_video *driver);
int gp1_drm_configure(struct gp1_video *driver);
int gp1_drm_init_gx(struct gp1_video *driver);

#endif
