#ifndef GP1_GLX_INTERNAL_H
#define GP1_GLX_INTERNAL_H

#include "io/gp1_io_video.h"
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1
#include <GL/glx.h>
#include <GL/glxext.h>

#define KeyRepeat (LASTEvent+2)
#define GP1_GLX_KEY_REPEAT_INTERVAL 10

struct gp1_video_glx {
  struct gp1_video hdr;
  
  Display *dpy;
  int screen;
  Window win;
  GLXContext ctx;
  int glx_version_minor,glx_version_major;
  
  Atom atom_WM_PROTOCOLS;
  Atom atom_WM_DELETE_WINDOW;
  Atom atom__NET_WM_STATE;
  Atom atom__NET_WM_STATE_FULLSCREEN;
  Atom atom__NET_WM_STATE_ADD;
  Atom atom__NET_WM_STATE_REMOVE;
  Atom atom__NET_WM_ICON;
  
  int cursor_visible;
  int screensaver_suppressed;
  int focus;
};

#define DRIVER ((struct gp1_video_glx*)driver)

int gp1_glx_codepoint_from_keysym(int keysym);
int gp1_glx_usb_usage_from_keysym(int keysym);
int gp1_glx_update(struct gp1_video *driver);

#endif
