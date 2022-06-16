/* gp1_io_input.h
 * Generic interface for input drivers.
 * "btnid" in this interface has nothing to do with the VM's symbols, it's opaque and device-specific.
 */
 
#ifndef GP1_IO_INPUT_H
#define GP1_IO_INPUT_H

struct gp1_input;
struct gp1_input_type;

struct gp1_input_delegate {
  void *userdata;
  
  /* Called when a new device is detected.
   * Guaranteed to be the first time you'll see this (devid).
   * Drivers please note: It is legal and normal for clients to disconnect the device during this callback.
   */
  int (*connect)(struct gp1_input *input,int devid);
  
  /* Called when a device previously connected becomes disconnected.
   * Note that this is not called at driver shutdown; any devices left on are quietly dropped then.
   * Ditto for manual disconnects, those aren't reported.
   * Guaranteed to be the last time you'll see this (devid).
   */
  int (*disconnect)(struct gp1_input *input,int devid);
  
  /* Change of state on a connected device.
   * (btnid) is in a namespace specific to the device or driver; use gp1_input_device_iterate() to make sense of it.
   */
  int (*button)(struct gp1_input *input,int devid,int btnid,int value);
};

struct gp1_input {
  const struct gp1_input_type *type;
  struct gp1_input_delegate delegate;
  int refc;
};

void gp1_input_del(struct gp1_input *input);
int gp1_input_ref(struct gp1_input *input);

struct gp1_input *gp1_input_new(
  const struct gp1_input_type *type,
  const struct gp1_input_delegate *delegate
);

int gp1_input_update(struct gp1_input *input);

int gp1_input_device_disconnect(struct gp1_input *input,int devid);
const char *gp1_input_device_get_ids(int *vid,int *pid,struct gp1_input *input,int devid);
int gp1_input_device_iterate(
  struct gp1_input *input,int devid,
  int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
  void *userdata
);

struct gp1_input_type {
  const char *name;
  const char *desc;
  int objlen;
  void (*del)(struct gp1_input *input);
  int (*init)(struct gp1_input *input);
  int (*update)(struct gp1_input *input);
  int (*device_disconnect)(struct gp1_input *input,int devid);
  const char *(*device_get_ids)(int *vid,int *pid,struct gp1_input *input,int devid);
  int (*device_iterate)(
    struct gp1_input *input,int devid,
    int (*cb)(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata),
    void *userdata
  );
};

const struct gp1_input_type *gp1_input_type_by_index(int p);
const struct gp1_input_type *gp1_input_type_by_name(const char *name,int namec);

/* Drivers should call this to create a new devid, will ensure they never collide.
 * We don't track them or anything. After 2**31-1 have been made, no more devices can be connected.
 */
int gp1_input_unused_devid();

#endif
