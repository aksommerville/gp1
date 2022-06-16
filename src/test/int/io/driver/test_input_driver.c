#include "test/gp1_test.h"
#include "io/gp1_io_input.h"
#include "io/gp1_io_clock.h"

/* Live test.
 */
 
static int live_input_cb_cap(struct gp1_input *input,int devid,int btnid,int usage,int lo,int value,int hi,void *userdata) {
  fprintf(stderr,"  0x%08x 0x%08x %d..%d..%d\n",btnid,usage,lo,value,hi);
  return 0;
}
 
static int live_input_cb_connect(struct gp1_input *input,int devid) {

  int vid=0,pid=0;
  const char *name=gp1_input_device_get_ids(&vid,&pid,input,devid);
  fprintf(stderr,"%s devid=%d vid=0x%04x pid=0x%04x name='%s'\n",__func__,devid,vid,pid,name);
  
  if (gp1_input_device_iterate(input,devid,live_input_cb_cap,0)<0) return -1;
  
  // Test manual disconnection. Normally disabled; it's more interesting to leave them running :)
  if (0) {
    fprintf(stderr,"Manually disconnecting device -- confirm you don't get more events.\n");
    if (gp1_input_device_disconnect(input,devid)<0) return -1;
  }
  
  return 0;
}
 
static int live_input_cb_disconnect(struct gp1_input *input,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
  return 0;
}
 
static int live_input_cb_button(struct gp1_input *input,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
  return 0;
}
 
XXX_GP1_ITEST(live_input) {
  
  const char *name="evdev";
  
  const struct gp1_input_type *type=gp1_input_type_by_name(name,-1);
  GP1_ASSERT(type,"input '%s' not found",name);
  
  struct gp1_input_delegate delegate={
    .connect=live_input_cb_connect,
    .disconnect=live_input_cb_disconnect,
    .button=live_input_cb_button,
  };
  struct gp1_input *input=gp1_input_new(type,&delegate);
  GP1_ASSERT(input,"Failed to instantiate input driver '%s'",name);
  
  fprintf(stderr,"Instantiated input driver '%s'. Will update for about 5 seconds...\n",name);
  int ttl=1000;
  while (ttl-->0) {
    GP1_ASSERT_CALL(gp1_input_update(input))
    gp1_sleep(5000);
  }
  
  gp1_input_del(input);
  return 0;
}

/* Generic types validation.
 */
 
GP1_ITEST(input_types) {
  int typec=0;
  for (;;typec++) {
    const struct gp1_input_type *type=gp1_input_type_by_index(typec);
    if (!type) break;
    
    GP1_ASSERT(type->name&&type->name[0],"p=%d",typec)
    GP1_ASSERT(gp1_input_type_by_name(type->name,-1)==type,"p=%d name=%s",typec,type->name)
    
    GP1_ASSERT_INTS_OP(type->objlen,>=,sizeof(struct gp1_input),"input type '%s'",type->name)
  }
  // typec==0 is actually ok, wouldn't be too shocking for a system with a window manager.
  return 0;
}

/* ID assignment.
 */
 
GP1_ITEST(test_gp1_input_unused_devid) {
  // Any REPC under 2**31-1 should work (tho you'd have to be more careful how you record them, at that level)
  #define REPC 32
  int usedv[REPC];
  int usedc=0;
  while (usedc<REPC) {
    int devid=gp1_input_unused_devid();
    GP1_ASSERT_INTS_OP(devid,>,0,"after %d repetitions",usedc)
    int i=usedc; while (i-->0) {
      GP1_ASSERT_INTS_OP(devid,!=,usedv[i],"after %d repetitions",usedc)
    }
    usedv[usedc++]=devid;
  }
  #undef REPC
  // The IDs assigned will be, in order, 1,2,3,...,REPC. But we're not asserting that.
  // gp1_input_unused_devid() will start failing after 2G repetitions, but we're not asserting that.
  return 0;
}
