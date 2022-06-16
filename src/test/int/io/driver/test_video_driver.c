#include "test/gp1_test.h"
#include "io/gp1_io_video.h"
#include "io/gp1_io_clock.h"
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <GL/gl.h>

/* Live test.
 */
 
static int live_video_quit=0;
 
static int cb_close(struct gp1_video *video) {
  fprintf(stderr,"%s\n",__func__);
  live_video_quit=1;
  return 0;
}

static int cb_focus(struct gp1_video *video,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
  return 0;
}

static int cb_resize(struct gp1_video *video,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
  return 0;
}
  
static int cb_key(struct gp1_video *video,int keycode,int value) {
  fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  return 0;
}
  
static int cb_text(struct gp1_video *video,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
  switch (codepoint) {
    case 0x1b: live_video_quit=1; break;
  }
  return 0;
}
  
static int cb_mmotion(struct gp1_video *video,int x,int y) {
  fprintf(stderr,"%s %d,%d\n",__func__,x,y);
  return 0;
}

static int cb_mbutton(struct gp1_video *video,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
  return 0;
}

static int cb_mwheel(struct gp1_video *video,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
  return 0;
}
 
XXX_GP1_ITEST(live_video) {

  const char *name="glx";
  int w=640;
  int h=360;
  int fullscreen=0;
  int rate=60;
  
  const uint8_t icon[8*8*4]={
  #define _ 0,0,0,0,
  #define K 0,0,0,255,
  #define W 255,255,255,255,
  #define R 255,0,0,255,
  #define G 0,255,0,255,
  #define B 0,0,255,255,
    _ _ _ K K K K K
    _ _ K W W W W K
    _ K W W W W W K
    K R R R R R W K
    K G G G G G W K
    K B B B B B W K
    K W W W W W W K
    K K K K K K K K
  #undef _
  #undef K
  #undef W
  #undef R
  #undef G
  #undef B
  };
  
  const struct gp1_video_type *type=gp1_video_type_by_name(name,-1);
  GP1_ASSERT(type,"video '%s' not found",name)
  
  live_video_quit=0;
  
  struct gp1_video_delegate delegate={
    .cb_close=cb_close,
    .cb_focus=cb_focus,
    .cb_resize=cb_resize,
    .cb_key=cb_key,
    .cb_text=cb_text,
    .cb_mmotion=cb_mmotion,
    .cb_mbutton=cb_mbutton,
    .cb_mwheel=cb_mwheel,
  };
  struct gp1_video *video=gp1_video_new(type,&delegate,w,h,rate,fullscreen,"GP1 Live Video Test",-1,icon,8,8);
  GP1_ASSERT(video,"gp1_video_new %s %d,%d rate=%d fullscreen=%d",name,w,h,rate,fullscreen)
  
  float lumap=0.0f;
  float lumadp=(M_PI*2.0f)/(rate*3.0f); // final constant is the period in seconds
  double time_start=gp1_now_s();
  int framec=0;
  
  int ttl=500;
  while (1) {
    if (live_video_quit) {
      fprintf(stderr,"Quitting due to user request.\n");
      break;
    }
    if (--ttl<=0) {
      fprintf(stderr,"Quitting due to TTL.\n");
      break;
    }
    GP1_ASSERT_CALL(gp1_video_update(video))
    
    lumap+=lumadp;
    if (lumap>M_PI) lumap-=M_PI*2.0f;
    float luma=(sinf(lumap)*0.5f)+0.5f;
    glClearColor(0.40f*luma,0.60f*luma,0.90f*luma,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    GP1_ASSERT_CALL(gp1_video_swap(video))
    framec++;
    
    // Poll stdin rather than sleeping -- this also gives us a means to manually quit for non-WM drivers.
    struct pollfd pollfd={.fd=STDIN_FILENO,.events=POLLIN|POLLERR|POLLHUP};
    if (poll(&pollfd,1,16)>0) {
      fprintf(stderr,"Quitting due to stdin.\n");
      char buf[64];
      read(STDIN_FILENO,buf,sizeof(buf));
      break;
    }
  }
  
  double time_end=gp1_now_s();
  double elapsed=time_end-time_start;
  fprintf(stderr,"%d frames in %.03fs, average rate %.03f Hz\n",framec,elapsed,framec/elapsed);

  gp1_video_del(video);
  return 0;
}

/* Validate drivers generically.
 */
 
GP1_ITEST(video_drivers) {
  int typec=0;
  for (;;typec++) {
    const struct gp1_video_type *type=gp1_video_type_by_index(typec);
    if (!type) break;
    
    GP1_ASSERT(type->name&&type->name[0],"p=%d, name required",typec)
    int namec=0;
    for (;type->name[namec];namec++) {
      if ((type->name[namec]<0x20)||(type->name[namec]>0x7e)) {
        GP1_FAIL("p=%d, illegal byte 0x%02x in name",typec,(unsigned char)type->name[namec])
      }
    }
    GP1_ASSERT_INTS_OP(namec,<,64,"p=%d, name too long",typec)
    
    GP1_ASSERT(type==gp1_video_type_by_name(type->name,-1),"p=%d name=%s",typec,type->name)
    
    GP1_ASSERT_INTS_OP(type->objlen,>=,sizeof(struct gp1_video),"video '%s'",type->name)
  }
  return 0;
}
