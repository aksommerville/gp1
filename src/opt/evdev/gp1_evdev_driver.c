#include "gp1_evdev_internal.h"
  
/* Cleanup.
 */
 
void gp1_evdev_device_cleanup(struct gp1_evdev_device *device) {
  if (device->fd>=0) close(device->fd);
  if (device->name) free(device->name);
}
 
static void _gp1_evdev_del(struct gp1_input *input) {
  if (INPUT->devroot) free(INPUT->devroot);
  if (INPUT->infd>=0) close(INPUT->infd);
  if (INPUT->devicev) {
    while (INPUT->devicec-->0) gp1_evdev_device_cleanup(INPUT->devicev+INPUT->devicec);
    free(INPUT->devicev);
  }
  if (INPUT->pollfdv) free(INPUT->pollfdv);
}

/* Init.
 */
 
static int _gp1_evdev_init(struct gp1_input *input) {

  INPUT->infd=-1;
  
  // For now and the forseeable future, we'll complete setup during init.
  // It's a separate function in case we someday want to expose this to callers.
  if (gp1_evdev_setup(input,"/dev/input/",11)<0) return -1;
  
  return 0;
}

/* Reopen inotify.
 */
 
static int gp1_evdev_reopen_inotify(struct gp1_input *input) {
  if (INPUT->infd>=0) return 0;
  if (!INPUT->devroot) return -1;
  if ((INPUT->infd=inotify_init())<0) return -1;
  if (inotify_add_watch(INPUT->infd,INPUT->devroot,IN_CREATE|IN_ATTRIB|IN_MOVED_TO)<0) return -1;
  return 0;
}

/* Setup.
 */
 
int gp1_evdev_setup(struct gp1_input *input,const char *devroot,int devrootc) {
  if (!input||(input->type!=&gp1_input_type_evdev)) return -1;
  if (INPUT->devroot) return -1;
  
  if (!devroot) return -1;
  if (devrootc<0) { devrootc=0; while (devroot[devrootc]) devrootc++; }
  if ((devrootc<1)||(devroot[0]!='/')||(devroot[devrootc-1]!='/')) return -1;
  
  if (!(INPUT->devroot=malloc(devrootc+1))) return -1;
  memcpy(INPUT->devroot,devroot,devrootc);
  INPUT->devroot[devrootc]=0;
  INPUT->devrootc=devrootc;
  
  if (gp1_evdev_reopen_inotify(input)<0) return -1;
  INPUT->scan=1;
  
  return 0;
}

/* Populate pollfd list.
 * Note that this does not touch (INPUT->pollfdv); one expects that to be used as the output vector.
 */
 
int gp1_evdev_get_pollfds(struct pollfd *dst,int dsta,struct gp1_input *input) {
  int dstc=0;
  
  if (INPUT->infd>=0) {
    if (dstc<dsta) {
      struct pollfd *pollfd=dst+dstc;
      pollfd->fd=INPUT->infd;
      pollfd->events=POLLIN|POLLERR|POLLHUP;
      pollfd->revents=0;
    }
    dstc++;
  }
  
  const struct gp1_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->fd<0) continue;
    if (dstc<dsta) {
      struct pollfd *pollfd=dst+dstc;
      pollfd->fd=device->fd;
      pollfd->events=POLLIN|POLLERR|POLLHUP;
      pollfd->revents=0;
    }
    dstc++;
  }
  
  return dstc;
}

/* Drop any devices with a closed file.
 * This does not trigger the disconnect callback; that should be done at the moment the file closes.
 */
 
static void gp1_evdev_drop_defunct_devices(struct gp1_input *input) {
  int i=INPUT->devicec;
  struct gp1_evdev_device *device=INPUT->devicev+i;
  while (i-->0) {
    device--;
    if (device->fd>=0) continue;
    gp1_evdev_device_cleanup(device);
    INPUT->devicec--;
    memmove(device,device+1,sizeof(struct gp1_evdev_device)*(INPUT->devicec-i));
  }
}

/* Update.
 */
 
static int _gp1_evdev_update(struct gp1_input *input) {

  gp1_evdev_drop_defunct_devices(input);
  
  if (INPUT->scan) {
    INPUT->scan=0;
    if (gp1_evdev_scan(input)<0) return -1;
  }
  
  while (1) {
    if ((INPUT->pollfdc=gp1_evdev_get_pollfds(INPUT->pollfdv,INPUT->pollfda,input))<0) return -1;
    if (INPUT->pollfdc<=INPUT->pollfda) break;
    int na=(INPUT->pollfdc+8)&~7;
    void *nv=realloc(INPUT->pollfdv,sizeof(struct pollfd)*na);
    if (!nv) return -1;
    INPUT->pollfdv=nv;
    INPUT->pollfda=na;
  }
  if (INPUT->pollfdc<1) return 0;
  
  if (poll(INPUT->pollfdv,INPUT->pollfdc,0)<=0) return 0;
  
  struct pollfd *pollfd=INPUT->pollfdv;
  int i=INPUT->pollfdc;
  for (;i-->0;pollfd++) {
    if (!pollfd->revents) continue;
    if (gp1_evdev_poll_file(input,pollfd->fd)<0) return -1;
  }
  
  return 0;
}

/* Disconnect device.
 */
 
static int _gp1_evdev_device_disconnect(struct gp1_input *input,int devid) {
  struct gp1_evdev_device *device=gp1_evdev_device_by_devid(input,devid);
  if (!device) return -1;
  if (device->fd>=0) {
    close(device->fd);
    device->fd=-1;
  }
  return 0;
}

/* Get device IDs.
 */
 
static const char *_gp1_evdev_device_get_ids(int *vid,int *pid,struct gp1_input *input,int devid) {
  struct gp1_evdev_device *device=gp1_evdev_device_by_devid(input,devid);
  if (!device) return 0;
  gp1_evdev_device_require_ids(device);
  *vid=device->vid;
  *pid=device->pid;
  return device->name;
}

/* Search devices.
 */
 
struct gp1_evdev_device *gp1_evdev_device_by_kid(const struct gp1_input *input,int kid) {
  struct gp1_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->kid==kid) return device;
  }
  return 0;
}

struct gp1_evdev_device *gp1_evdev_device_by_fd(const struct gp1_input *input,int fd) {
  struct gp1_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->fd==fd) return device;
  }
  return 0;
}

struct gp1_evdev_device *gp1_evdev_device_by_devid(const struct gp1_input *input,int devid) {
  struct gp1_evdev_device *device=INPUT->devicev;
  int i=INPUT->devicec;
  for (;i-->0;device++) {
    if (device->devid==devid) return device;
  }
  return 0;
}

/* Type definition.
 */
 
const struct gp1_input_type gp1_input_type_evdev={
  .name="evdev",
  .desc="Linux event devices (mostly for joysticks).",
  .objlen=sizeof(struct gp1_input_evdev),
  .del=_gp1_evdev_del,
  .init=_gp1_evdev_init,
  .update=_gp1_evdev_update,
  .device_disconnect=_gp1_evdev_device_disconnect,
  .device_get_ids=_gp1_evdev_device_get_ids,
  .device_iterate=_gp1_evdev_device_iterate,
};
