#include "gp1_evdev_internal.h"
#include "io/gp1_io_fs.h"

/* Read device.
 * Fail only on hard errors; if it fails to read or whatever, disconnect the device and report success.
 */
 
static int gp1_evdev_poll_device(struct gp1_input *input,struct gp1_evdev_device *device) {
  struct input_event eventv[32];
  int eventc=read(device->fd,eventv,sizeof(eventv));
  if (eventc<=0) {
    int err=0;
    close(device->fd);
    device->fd=-1;
    if (input->delegate.disconnect) {
      err=input->delegate.disconnect(input,device->devid);
    }
    return err;
  }
  if (!input->delegate.button) return 0;
  eventc/=sizeof(struct input_event);
  const struct input_event *event=eventv;
  int i=eventc;
  for (;i-->0;event++) {
  
    // Throw away SYN and MSC events.
    // MSC ones are a shame; the kernel reports USB-HID usage codes this way.
    // But we want them all at connect; they do us no good once we've got this far.
    if (event->type==EV_SYN) continue;
    if (event->type==EV_MSC) continue;
  
    if (input->delegate.button(input,device->devid,(event->type<<24)|event->code,event->value)<0) return -1;
  }
  return 0;
}

/* Grow device list.
 */
 
static struct gp1_evdev_device *gp1_evdev_add_device(struct gp1_input *input) {
  if (INPUT->devicec>=INPUT->devicea) {
    int na=INPUT->devicea+8;
    if (na>INT_MAX/sizeof(struct gp1_evdev_device)) return 0;
    void *nv=realloc(INPUT->devicev,sizeof(struct gp1_evdev_device)*na);
    if (!nv) return 0;
    INPUT->devicev=nv;
    INPUT->devicea=na;
  }
  struct gp1_evdev_device *device=INPUT->devicev+INPUT->devicec++;
  memset(device,0,sizeof(struct gp1_evdev_device));
  device->fd=-1;
  device->kid=-1;
  return device;
}

/* Examine a detected file and connect if valid.
 * No harm if it's not an evdev, fails to open, or already open.
 */
 
static int gp1_evdev_try_file(struct gp1_input *input,const char *base,int basec) {

  // Read kernel ID from basename. Abort if not evdev-like, or already open.
  if (!base) return 0;
  if (basec<0) { basec=0; while (base[basec]) basec++; }
  if (basec<6) return 0;
  if (memcmp(base,"event",5)) return 0;
  int kid=0,basep=5;
  for (;basep<basec;basep++) {
    int digit=base[basep]-'0';
    if ((digit<0)||(digit>9)) return 0;
    kid*=10;
    kid+=digit;
  }
  if (gp1_evdev_device_by_kid(input,kid)) return 0;
  
  // Compose the full path and open it.
  char path[1024];
  int pathc=snprintf(path,sizeof(path),"%.*s%.*s",INPUT->devrootc,INPUT->devroot,basec,base);
  if ((pathc<1)||(pathc>=sizeof(path))) return 0;
  int fd=open(path,O_RDONLY);
  if (fd<0) return 0;
  
  // EVIOCGVERSION to confirm for real that it is evdev.
  int eversion=0;
  if (ioctl(fd,EVIOCGVERSION,&eversion)<0) {
    close(fd);
    return 0;
  }
  
  // EVIOCGRAB, optional.
  int grab=1;
  if (ioctl(fd,EVIOCGRAB,&grab)<0) {
    // proceed anyway
  }
  
  int devid=gp1_input_unused_devid();
  if (devid<0) {
    close(fd);
    return 0;
  }
  
  struct gp1_evdev_device *device=gp1_evdev_add_device(input);
  if (!device) {
    close(fd);
    return -1;
  }
  
  device->fd=fd;
  device->kid=kid;
  device->devid=devid;
  
  // Notify client.
  // Careful, it is perfectly legal for the client to manually disconnect the device during this callback.
  if (input->delegate.connect) {
    if (input->delegate.connect(input,devid)<0) return -1;
  }
  
  return 0;
}

/* Read inotify.
 */
 
static int gp1_evdev_poll_inotify(struct gp1_input *input) {
  char buf[256];
  int bufc=read(INPUT->infd,buf,sizeof(buf));
  if (bufc<=0) {
    fprintf(stderr,"evdev: inotify closed. New device connections will not be detected.\n");
    close(INPUT->infd);
    INPUT->infd=-1;
    return 0;
  }
  int bufp=0,stopp=bufc-sizeof(struct inotify_event);
  while (bufp<=stopp) {
    struct inotify_event *event=(struct inotify_event*)(buf+bufp);
    bufp+=sizeof(struct inotify_event);
    if (event->len<0) break;
    if (bufp>bufc-event->len) break;
    bufp+=event->len;
    const char *base=event->name;
    int basec=0;
    while ((basec<event->len)&&base[basec]) basec++;
    if (gp1_evdev_try_file(input,base,basec)<0) return -1;
  }
  return 0;
}

/* Update readable file.
 */
 
int gp1_evdev_poll_file(struct gp1_input *input,int fd) {
  if (fd==INPUT->infd) return gp1_evdev_poll_inotify(input);
  struct gp1_evdev_device *device=gp1_evdev_device_by_fd(input,fd);
  if (device) return gp1_evdev_poll_device(input,device);
  return 0;
}

/* Scan device directory.
 */
 
static int gp1_evdev_scan_cb(const char *path,const char *base,char type,void *userdata) {
  struct gp1_input *input=userdata;
  if (gp1_evdev_try_file(input,base,-1)<0) return -1;
  return 0;
}
 
int gp1_evdev_scan(struct gp1_input *input) {
  return gp1_dir_read(INPUT->devroot,gp1_evdev_scan_cb,input);
}

/* Fetch device IDs if we haven't yet.
 */
 
int gp1_evdev_device_require_ids(struct gp1_evdev_device *device) {
  if (device->vid) return 0;
  if (device->fd<0) {
    device->vid=device->pid=-1;
    return 0;
  }
  struct input_id id={0};
  ioctl(device->fd,EVIOCGID,&id);
  device->vid=id.vendor;
  device->pid=id.product;
  char tmp[256];
  int tmpc=ioctl(device->fd,EVIOCGNAME(sizeof(tmp)),tmp);
  if ((tmpc>0)&&(tmpc<=sizeof(tmp))) {
    int leadc=0;
    while ((leadc<tmpc)&&((unsigned char)tmp[leadc]<=0x20)) leadc++;
    if (leadc<tmpc) {
      int tailc=0;
      while ((leadc+tailc<tmpc)&&((unsigned char)tmp[tmpc-tailc-1]<=0x20)) tailc++;
      char *nv=malloc(tmpc-tailc-leadc+1);
      if (nv) {
        memcpy(nv,tmp+leadc,tmpc-tailc-leadc);
        nv[tmpc-tailc-leadc]=0;
        if (device->name) free(device->name);
        device->name=nv;
        device->namec=tmpc-tailc-leadc;
        int i=device->namec;
        while (i-->0) if ((device->name[i]<0x20)||(device->name[i]>0x7e)) device->name[i]='?';
      }
    }
  }
  return 0;
}

/* Iterate capabilities.
 */
 
static int gp1_evdev_device_iterate_1(
  struct gp1_input *input,
  struct gp1_evdev_device *device,
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata,
  int type,const uint8_t *v,int c
) {
  int code=type<<24,err;
  for (;c-->0;v++,code+=8) {
    if (!*v) continue;
    uint8_t mask=1,minor=0;
    for (;minor<8;minor++,mask<<=1) {
      if (!((*v)&mask)) continue;
      
      int lo=0,value=0,hi=2;
      if (type==EV_ABS) {
        struct input_absinfo absinfo={0};
        ioctl(device->fd,EVIOCGABS(code|minor),&absinfo);
        lo=absinfo.minimum;
        hi=absinfo.maximum;
        value=absinfo.value;
      }
      
      if (err=cb(input,device->devid,code|minor,gp1_evdev_usage_for_btnid(code|minor),lo,value,hi,userdata)) return err;
    }
  }
  return 0;
}
 
int _gp1_evdev_device_iterate(
  struct gp1_input *input,int devid,
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata
) {
  struct gp1_evdev_device *device=gp1_evdev_device_by_devid(input,devid);
  if (!device) return -1;
  
  uint8_t v[(KEY_CNT+7)>>3]; // KEY is by far the largest namespace
  int c,err;
  if ((c=ioctl(device->fd,EVIOCGBIT(EV_KEY,sizeof(v)),v))>0) {
    if (err=gp1_evdev_device_iterate_1(input,device,cb,userdata,EV_KEY,v,c)) return err;
  }
  if ((c=ioctl(device->fd,EVIOCGBIT(EV_ABS,sizeof(v)),v))>0) {
    if (err=gp1_evdev_device_iterate_1(input,device,cb,userdata,EV_ABS,v,c)) return err;
  }
  // EV_REL,EV_SW, do we care?
  
  return 0;
}
