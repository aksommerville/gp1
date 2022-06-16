#include "gp1_drm_internal.h"

static int _drm_swap(struct gp1_video *driver);

/* Cleanup.
 */
 
static void drm_fb_cleanup(struct gp1_video *driver,struct gp1_drm_fb *fb) {
  if (fb->fbid) {
    drmModeRmFB(DRIVER->fd,fb->fbid);
  }
}
 
static void _drm_del(struct gp1_video *driver) {

  // If waiting for a page flip, we must let it finish first.
  if ((DRIVER->fd>=0)&&DRIVER->flip_pending) {
    struct pollfd pollfd={.fd=DRIVER->fd,.events=POLLIN|POLLERR|POLLHUP};
    if (poll(&pollfd,1,500)>0) {
      char dummy[64];
      read(DRIVER->fd,dummy,sizeof(dummy));
    }
  }
  
  if (DRIVER->eglcontext) eglDestroyContext(DRIVER->egldisplay,DRIVER->eglcontext);
  if (DRIVER->eglsurface) eglDestroySurface(DRIVER->egldisplay,DRIVER->eglsurface);
  if (DRIVER->egldisplay) eglTerminate(DRIVER->egldisplay);
  
  drm_fb_cleanup(driver,DRIVER->fbv+0);
  drm_fb_cleanup(driver,DRIVER->fbv+1);
  
  if (DRIVER->crtc_restore&&(DRIVER->fd>=0)) {
    drmModeCrtcPtr crtc=DRIVER->crtc_restore;
    drmModeSetCrtc(
      DRIVER->fd,crtc->crtc_id,crtc->buffer_id,
      crtc->x,crtc->y,&DRIVER->connid,1,&crtc->mode
    );
    drmModeFreeCrtc(DRIVER->crtc_restore);
  }
  
  if (DRIVER->fd>=0) close(DRIVER->fd);
}

/* Init.
 */
 
static int _drm_init(
  struct gp1_video *driver,
  const char *title,int titlec,
  const void *icon_rgba,int iconw,int iconh
) {

  DRIVER->fd=-1;
  DRIVER->crtcunset=1;

  if (!drmAvailable()) {
    fprintf(stderr,"DRM not available.\n");
    return -1;
  }
  
  if (
    (gp1_drm_open_file(driver)<0)||
    (gp1_drm_configure(driver)<0)||
    (gp1_drm_init_gx(driver)<0)||
  0) return -1;

  return 0;
}

/* Poll file.
 */
 
static void drm_cb_vblank(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,void *userdata
) {}
 
static void drm_cb_page1(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,void *userdata
) {
  struct gp1_video *driver=userdata;
  DRIVER->flip_pending=0;
}
 
static void drm_cb_page2(
  int fd,unsigned int seq,unsigned int times,unsigned int timeus,unsigned int ctrcid,void *userdata
) {
  drm_cb_page1(fd,seq,times,timeus,userdata);
}
 
static void drm_cb_seq(
  int fd,uint64_t seq,uint64_t timeus,uint64_t userdata
) {}
 
static int drm_poll_file(struct gp1_video *driver,int to_ms) {
  //TODO Can we borrow the owner's poller?
  struct pollfd pollfd={.fd=DRIVER->fd,.events=POLLIN};
  if (poll(&pollfd,1,to_ms)<=0) return 0;
  drmEventContext ctx={
    .version=DRM_EVENT_CONTEXT_VERSION,
    .vblank_handler=drm_cb_vblank,
    .page_flip_handler=drm_cb_page1,
    .page_flip_handler2=drm_cb_page2,
    .sequence_handler=drm_cb_seq,
  };
  int err=drmHandleEvent(DRIVER->fd,&ctx);
  if (err<0) return -1;
  return 0;
}

/* Swap.
 */
 
static int drm_swap_egl(uint32_t *fbid,struct gp1_video *driver) { 
  eglSwapBuffers(DRIVER->egldisplay,DRIVER->eglsurface);
  
  struct gbm_bo *bo=gbm_surface_lock_front_buffer(DRIVER->gbmsurface);
  if (!bo) return -1;
  
  int handle=gbm_bo_get_handle(bo).u32;
  struct gp1_drm_fb *fb;
  if (!DRIVER->fbv[0].handle) {
    fb=DRIVER->fbv;
  } else if (handle==DRIVER->fbv[0].handle) {
    fb=DRIVER->fbv;
  } else {
    fb=DRIVER->fbv+1;
  }
  
  if (!fb->fbid) {
    int width=gbm_bo_get_width(bo);
    int height=gbm_bo_get_height(bo);
    int stride=gbm_bo_get_stride(bo);
    fb->handle=handle;
    if (drmModeAddFB(DRIVER->fd,width,height,24,32,stride,fb->handle,&fb->fbid)<0) return -1;
    
    if (DRIVER->crtcunset) {
      if (drmModeSetCrtc(
        DRIVER->fd,DRIVER->crtcid,fb->fbid,0,0,
        &DRIVER->connid,1,&DRIVER->mode
      )<0) {
        fprintf(stderr,"drmModeSetCrtc: %m\n");
        return -1;
      }
      DRIVER->crtcunset=0;
    }
  }
  
  *fbid=fb->fbid;
  if (DRIVER->bo_pending) {
    gbm_surface_release_buffer(DRIVER->gbmsurface,DRIVER->bo_pending);
  }
  DRIVER->bo_pending=bo;
  
  return 0;
}

static int _drm_swap(struct gp1_video *driver) {

  // There must be no more than one page flip in flight at a time.
  // If one is pending -- likely -- give it a chance to finish.
  if (DRIVER->flip_pending) {
    if (drm_poll_file(driver,10)<0) return -1;
    if (DRIVER->flip_pending) {
      // Page flip didn't complete after a 10 ms delay? Drop the frame, no worries.
      return 0;
    }
  }
  DRIVER->flip_pending=1;
  
  uint32_t fbid=0;
  if (drm_swap_egl(&fbid,driver)<0) {
    DRIVER->flip_pending=0;
    return -1;
  }
  
  if (drmModePageFlip(DRIVER->fd,DRIVER->crtcid,fbid,DRM_MODE_PAGE_FLIP_EVENT,driver)<0) {
    fprintf(stderr,"drmModePageFlip: %m\n");
    DRIVER->flip_pending=0;
    return -1;
  }

  return 0;
}

/* Type definition.
 */
 
const struct gp1_video_type gp1_video_type_drm={
  .name="drm",
  .desc="Linux video with DRM and OpenGL. For headless systems only.",
  .provides_system_keyboard=0,
  .objlen=sizeof(struct gp1_video_drm),
  .del=_drm_del,
  .init=_drm_init,
  .swap=_drm_swap,
};
