#include "gp1_glx_internal.h"

/* Key press, release, or repeat.
 */
 
static int glx_evt_key(struct gp1_video *driver,XKeyEvent *evt,int value) {

  /* Pass the raw keystroke. */
  if (driver->delegate.cb_key) {
    KeySym keysym=XkbKeycodeToKeysym(DRIVER->dpy,evt->keycode,0,0);
    if (keysym) {
      int keycode=gp1_glx_usb_usage_from_keysym((int)keysym);
      if (keycode) {
        if (driver->delegate.cb_key(driver,keycode,value)<0) return -1;
      }
    }
  }
  
  /* Pass text if press or repeat, and text can be acquired. */
  if (driver->delegate.cb_text) {
    int shift=(evt->state&ShiftMask)?1:0;
    KeySym tkeysym=XkbKeycodeToKeysym(DRIVER->dpy,evt->keycode,0,shift);
    if (shift&&!tkeysym) { // If pressing shift makes this key "not a key anymore", fuck that and pretend shift is off
      tkeysym=XkbKeycodeToKeysym(DRIVER->dpy,evt->keycode,0,0);
    }
    if (tkeysym) {
      int codepoint=gp1_glx_codepoint_from_keysym(tkeysym);
      if (codepoint && (evt->type == KeyPress || evt->type == KeyRepeat)) {
        if (driver->delegate.cb_text(driver,codepoint)<0) return -1;
      }
    }
  }
  
  return 0;
}

/* Mouse events.
 */
 
static int glx_evt_mbtn(struct gp1_video *driver,XButtonEvent *evt,int value) {
  switch (evt->button) {
    case 1: if (driver->delegate.cb_mbutton) return driver->delegate.cb_mbutton(driver,1,value); break;
    case 2: if (driver->delegate.cb_mbutton) return driver->delegate.cb_mbutton(driver,3,value); break;
    case 3: if (driver->delegate.cb_mbutton) return driver->delegate.cb_mbutton(driver,2,value); break;
    case 4: if (value&&driver->delegate.cb_mwheel) return driver->delegate.cb_mwheel(driver,0,-1); break;
    case 5: if (value&&driver->delegate.cb_mwheel) return driver->delegate.cb_mwheel(driver,0,1); break;
    case 6: if (value&&driver->delegate.cb_mwheel) return driver->delegate.cb_mwheel(driver,-1,0); break;
    case 7: if (value&&driver->delegate.cb_mwheel) return driver->delegate.cb_mwheel(driver,1,0); break;
  }
  return 0;
}

static int glx_evt_mmotion(struct gp1_video *driver,XMotionEvent *evt) {
  if (driver->delegate.cb_mmotion) {
    if (driver->delegate.cb_mmotion(driver,evt->x,evt->y)<0) return -1;
  }
  return 0;
}

/* Client message.
 */
 
static int glx_evt_client(struct gp1_video *driver,XClientMessageEvent *evt) {
  if (evt->message_type==DRIVER->atom_WM_PROTOCOLS) {
    if (evt->format==32) {
      if (evt->data.l[0]==DRIVER->atom_WM_DELETE_WINDOW) {
        if (driver->delegate.cb_close) {
          return driver->delegate.cb_close(driver);
        }
      }
    }
  }
  return 0;
}

/* Configuration event (eg resize).
 */
 
static int glx_evt_configure(struct gp1_video *driver,XConfigureEvent *evt) {
  int nw=evt->width,nh=evt->height;
  if ((nw!=driver->w)||(nh!=driver->h)) {
    driver->w=nw;
    driver->h=nh;
    if (driver->delegate.cb_resize) {
      if (driver->delegate.cb_resize(driver,nw,nh)<0) {
        return -1;
      }
    }
  }
  return 0;
}

/* Focus.
 */
 
static int glx_evt_focus(struct gp1_video *driver,XFocusInEvent *evt,int value) {
  if (value==DRIVER->focus) return 0;
  DRIVER->focus=value;
  if (driver->delegate.cb_focus) {
    return driver->delegate.cb_focus(driver,value);
  }
  return 0;
}

/* Dispatch single event.
 */
 
static int glx_receive_event(struct gp1_video *driver,XEvent *evt) {
  if (!evt) return -1;
  switch (evt->type) {
  
    case KeyPress: return glx_evt_key(driver,&evt->xkey,1);
    case KeyRelease: return glx_evt_key(driver,&evt->xkey,0);
    case KeyRepeat: return glx_evt_key(driver,&evt->xkey,2);
    
    case ButtonPress: return glx_evt_mbtn(driver,&evt->xbutton,1);
    case ButtonRelease: return glx_evt_mbtn(driver,&evt->xbutton,0);
    case MotionNotify: return glx_evt_mmotion(driver,&evt->xmotion);
    
    case ClientMessage: return glx_evt_client(driver,&evt->xclient);
    
    case ConfigureNotify: return glx_evt_configure(driver,&evt->xconfigure);
    
    case FocusIn: return glx_evt_focus(driver,&evt->xfocus,1);
    case FocusOut: return glx_evt_focus(driver,&evt->xfocus,0);
    
  }
  return 0;
}

/* Update.
 */
 
int gp1_glx_update(struct gp1_video *driver) {
  if (!DRIVER->dpy) return 0;
  glFlush();
  int evtc=XEventsQueued(DRIVER->dpy,QueuedAfterFlush);
  while (evtc-->0) {
    XEvent evt={0};
    XNextEvent(DRIVER->dpy,&evt);
    
    /* If we detect an auto-repeated key, drop one of the events, and turn the other into KeyRepeat.
     * This is a hack to force single events for key repeat.
     */
    if ((evtc>0)&&(evt.type==KeyRelease)) {
      XEvent next={0};
      XNextEvent(DRIVER->dpy,&next);
      evtc--;
      if ((next.type==KeyPress)&&(evt.xkey.keycode==next.xkey.keycode)&&(evt.xkey.time>=next.xkey.time-GP1_GLX_KEY_REPEAT_INTERVAL)) {
        evt.type=KeyRepeat;
        if (glx_receive_event(driver,&evt)<0) return -1;
      } else {
        if (glx_receive_event(driver,&evt)<0) return -1;
        if (glx_receive_event(driver,&next)<0) return -1;
      }
    } else {
      if (glx_receive_event(driver,&evt)<0) return -1;
    }
  }
  return 0;
}
