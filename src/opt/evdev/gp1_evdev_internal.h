#ifndef GP1_EVDEV_INTERNAL_H
#define GP1_EVDEV_INTERNAL_H

#include "io/gp1_io_input.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/inotify.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

extern const struct gp1_input_type gp1_input_type_evdev;

struct gp1_input_evdev {
  struct gp1_input hdr;
  char *devroot; // "/dev/input/"
  int devrootc;
  int infd;
  int scan; // nonzero to scan before the next update
  struct gp1_evdev_device {
    int fd; // open character device
    int devid; // assigned by gp1 io
    int kid; // kernel's id, from the file name
    int vid,pid; // presumably from the usb device; vid zero if not fetched yet, -1 if failed to fetch
    char *name; // ''
    int namec;
  } *devicev;
  int devicec,devicea;
  struct pollfd *pollfdv;
  int pollfdc,pollfda;
};

#define INPUT ((struct gp1_input_evdev*)input)

void gp1_evdev_device_cleanup(struct gp1_evdev_device *device);

// (devroot) must begin and end with slash. Legal only once.
int gp1_evdev_setup(struct gp1_input *input,const char *devroot,int devrootc);

int gp1_evdev_scan(struct gp1_input *input);

// Keeping these nice and separate, in case we change the interface to expose them publicly.
// (it would be beneficial to get all the pollable things in one place, but i think not realistic).
int gp1_evdev_get_pollfds(struct pollfd *dst,int dsta,struct gp1_input *input);
int gp1_evdev_poll_file(struct gp1_input *input,int fd);

struct gp1_evdev_device *gp1_evdev_device_by_kid(const struct gp1_input *input,int kid);
struct gp1_evdev_device *gp1_evdev_device_by_fd(const struct gp1_input *input,int fd);
struct gp1_evdev_device *gp1_evdev_device_by_devid(const struct gp1_input *input,int devid);

int gp1_evdev_device_require_ids(struct gp1_evdev_device *device);

int _gp1_evdev_device_iterate(
  struct gp1_input *input,int devid,
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata
);

int gp1_evdev_usage_for_btnid(int btnid);

#endif
