#include "gp1_glx_internal.h"

/* Cleanup.
 */
 
static void _glx_del(struct gp1_video *driver) {
  if (DRIVER->ctx) {
    glXMakeCurrent(DRIVER->dpy,0,0);
    glXDestroyContext(DRIVER->dpy,DRIVER->ctx);
  }
  if (DRIVER->dpy) {
    XCloseDisplay(DRIVER->dpy);
  }
}

/* Hide cursor.
 */

static void glx_set_cursor_invisible(struct gp1_video *driver) {
  XColor color;
  Pixmap pixmap=XCreateBitmapFromData(DRIVER->dpy,DRIVER->win,"\0\0\0\0\0\0\0\0",1,1);
  Cursor cursor=XCreatePixmapCursor(DRIVER->dpy,pixmap,pixmap,&color,&color,0,0);
  XDefineCursor(DRIVER->dpy,DRIVER->win,cursor);
  XFreeCursor(DRIVER->dpy,cursor);
  XFreePixmap(DRIVER->dpy,pixmap);
}

/* Select initial window bounds.
 * Only effective if not fullscreen, but these are also the bounds you'll see if you toggle fullscreen off.
 */
 
static void glx_select_window_bounds(
  int *winw,int *winh,
  int fbw,int fbh,
  int screenw,int screenh
) {

  // There is window and ui chrome, and for aesthetics, don't consume the whole screen.
  const int margin=40;
  if ((screenw>margin)&&(fbw+margin<=screenw)) screenw-=margin;
  if ((screenh>margin)&&(fbh+margin<=screenh)) screenh-=margin;
  // To keep it pretty, scale up only by integer multiples.
  // So we're looking for the largest N where ((fbw*N<=screenw)&&(fbh*N<=screenh)).
  int xscale=screenw/fbw;
  int yscale=screenh/fbh;
  int scale=(xscale<yscale)?xscale:yscale;
  if (scale>=1) {
    *winw=fbw*scale;
    *winh=fbh*scale;
    return;
  }
  // Framebuffer is larger than the screen. That's weird. OK, go loose proportionate.
  int targetw=screenw;
  int targeth=screenh;
  int wforh=(fbw*targeth)/fbh;
  if (wforh<=targeth) {
    *winw=wforh;
    *winh=targeth;
  } else {
    *winw=targetw;
    *winh=(fbh*targetw)/fbw;
  }
}
 
/* Initialize X11 (display is already open).
 */
 
static int glx_startup(
  struct gp1_video *driver,
  const char *title,int titlec,
  const void *icon_rgba,int iconw,int iconh
) {
  
  #define GETATOM(tag) DRIVER->atom_##tag=XInternAtom(DRIVER->dpy,#tag,0);
  GETATOM(WM_PROTOCOLS)
  GETATOM(WM_DELETE_WINDOW)
  GETATOM(_NET_WM_STATE)
  GETATOM(_NET_WM_STATE_FULLSCREEN)
  GETATOM(_NET_WM_STATE_ADD)
  GETATOM(_NET_WM_STATE_REMOVE)
  GETATOM(_NET_WM_ICON)
  #undef GETATOM

  int attrv[]={
    GLX_X_RENDERABLE,1,
    GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
    GLX_RED_SIZE,8,
    GLX_GREEN_SIZE,8,
    GLX_BLUE_SIZE,8,
    GLX_ALPHA_SIZE,0,
    GLX_DEPTH_SIZE,0,
    GLX_STENCIL_SIZE,0,
    GLX_DOUBLEBUFFER,0,
  0};
  
  if (!glXQueryVersion(DRIVER->dpy,&DRIVER->glx_version_major,&DRIVER->glx_version_minor)) {
    return -1;
  }
  
  int fbc=0;
  GLXFBConfig *configv=glXChooseFBConfig(DRIVER->dpy,DRIVER->screen,attrv,&fbc);
  if (!configv||(fbc<1)) {
    return -1;
  }
  GLXFBConfig config=configv[0];
  XFree(configv);
  
  XVisualInfo *vi=glXGetVisualFromFBConfig(DRIVER->dpy,config);
  if (!vi) {
    return -1;
  }
  
  XSetWindowAttributes wattr={
    .event_mask=
      StructureNotifyMask|
      KeyPressMask|KeyReleaseMask|
      ButtonPressMask|ButtonReleaseMask|
      PointerMotionMask|
      FocusChangeMask|
    0,
  };
  wattr.colormap=XCreateColormap(DRIVER->dpy,RootWindow(DRIVER->dpy,vi->screen),vi->visual,AllocNone);
  
  int screenw=WidthOfScreen(ScreenOfDisplay(DRIVER->dpy,DRIVER->screen));
  int screenh=HeightOfScreen(ScreenOfDisplay(DRIVER->dpy,DRIVER->screen));
  glx_select_window_bounds(&driver->w,&driver->h,driver->w,driver->h,screenw,screenh);
  
  if (!(DRIVER->win=XCreateWindow(
    DRIVER->dpy,RootWindow(DRIVER->dpy,vi->screen),
    0,0,driver->w,driver->h,0,
    vi->depth,InputOutput,vi->visual,
    CWBorderPixel|CWColormap|CWEventMask,&wattr
  ))) {
    XFree(vi);
    return -1;
  }
  
  XFree(vi);
  
  if (title&&title[0]) {
    XStoreName(DRIVER->dpy,DRIVER->win,title);
  }
  
  if (icon_rgba&&(iconw>0)&&(iconh>0)) {
    fprintf(stderr,"TODO: GLX program icon [%s:%d]\n",__FILE__,__LINE__);//TODO
  }
  
  if (driver->fullscreen) {
    XChangeProperty(
      DRIVER->dpy,DRIVER->win,
      DRIVER->atom__NET_WM_STATE,
      XA_ATOM,32,PropModeReplace,
      (unsigned char*)&DRIVER->atom__NET_WM_STATE_FULLSCREEN,1
    );
  }
  
  XMapWindow(DRIVER->dpy,DRIVER->win);
  
  if (!(DRIVER->ctx=glXCreateNewContext(DRIVER->dpy,config,GLX_RGBA_TYPE,0,1))) {
    return -1;
  }
  glXMakeCurrent(DRIVER->dpy,DRIVER->win,DRIVER->ctx);
  
  XSync(DRIVER->dpy,0);
  
  XSetWMProtocols(DRIVER->dpy,DRIVER->win,&DRIVER->atom_WM_DELETE_WINDOW,1);
  
  return 0;
}

/* Init.
 */
 
static int _glx_init(
  struct gp1_video *driver,
  const char *title,int titlec,
  const void *icon_rgba,int iconw,int iconh
) {

  if (!(DRIVER->dpy=XOpenDisplay(0))) {
    return -1;
  }
  DRIVER->screen=DefaultScreen(DRIVER->dpy);
  
  if (glx_startup(driver,title,titlec,icon_rgba,iconw,iconh)<0) {
    return -1;
  }
  
  glx_set_cursor_invisible(driver);
  
  return 0;
}

/* Swap.
 */
 
static int _glx_swap(struct gp1_video *driver) {
  DRIVER->screensaver_suppressed=0;
  glXSwapBuffers(DRIVER->dpy,DRIVER->win);
  return 0;
}

/* Fullscreen.
 */

static void glx_set_fullscreen(struct gp1_video *driver,int full) {
  XEvent evt={
    .xclient={
      .type=ClientMessage,
      .message_type=DRIVER->atom__NET_WM_STATE,
      .send_event=1,
      .format=32,
      .window=DRIVER->win,
      .data={.l={
        full,
        DRIVER->atom__NET_WM_STATE_FULLSCREEN,
      }},
    }
  };
  XSendEvent(DRIVER->dpy,RootWindow(DRIVER->dpy,DRIVER->screen),0,SubstructureNotifyMask|SubstructureRedirectMask,&evt);
  XFlush(DRIVER->dpy);
  driver->fullscreen=full;
}
 
static int _glx_fullscreen(struct gp1_video *driver,int state) {
  if (state>0) {
    if (!driver->fullscreen) glx_set_fullscreen(driver,1);
  } else if (state==0) {
    if (driver->fullscreen) glx_set_fullscreen(driver,0);
  } else {
    glx_set_fullscreen(driver,driver->fullscreen?0:1);
  }
  return driver->fullscreen;
}

/* Screensaver.
 */
 
static void _glx_suppress_screensaver(struct gp1_video *driver) {
  if (!DRIVER->dpy) return;
  if (DRIVER->screensaver_suppressed) return;
  XForceScreenSaver(DRIVER->dpy,ScreenSaverReset);
  DRIVER->screensaver_suppressed=1;
}

/* Type definition.
 */
 
const struct gp1_video_type gp1_video_type_glx={
  .name="glx",
  .desc="Linux video with X11 and OpenGL.",
  .provides_system_keyboard=1,
  .objlen=sizeof(struct gp1_video_glx),
  .del=_glx_del,
  .init=_glx_init,
  .update=gp1_glx_update,
  .swap=_glx_swap,
  .set_fullscreen=_glx_fullscreen,
  .suppress_screensaver=_glx_suppress_screensaver,
};
