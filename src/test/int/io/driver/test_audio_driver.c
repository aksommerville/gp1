#include "test/gp1_test.h"
#include "io/gp1_io_audio.h"
#include "io/gp1_io_clock.h"
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

/* Live test: Instantiate an audio driver and sing into it.
 */
 
static int live_audio_samplec=0;
static int live_audio_phase=0;
static int live_audio_halfperiod=1; // will be overwritten
static int16_t live_audio_level=5000;
 
static int live_audio_cb_pcm_out(int16_t *v,int c,struct gp1_audio *audio) {
  live_audio_samplec+=c;
  switch (audio->chanc) {
    case 1: {
        for (;c-->0;v++) {
          if (++live_audio_phase>=live_audio_halfperiod) {
            live_audio_phase-=live_audio_halfperiod;
            live_audio_level=-live_audio_level;
          }
          *v=live_audio_level;
        }
      } break;
    case 2: {
        int framec=c>>1;
        for (;framec-->0;v+=2) {
          if (++live_audio_phase>=live_audio_halfperiod) {
            live_audio_phase-=live_audio_halfperiod;
            live_audio_level=-live_audio_level;
          }
          v[0]=v[1]=live_audio_level;
        }
      } break;
    default: memset(v,0,c<<1); break;
  }
  return 0;
}
 
XXX_GP1_ITEST(live_audio) {

  const char *name="alsa";
  int rate=44100;
  int chanc=2;
  int tone_rate=440;
  
  const struct gp1_audio_type *type=gp1_audio_type_by_name(name,-1);
  GP1_ASSERT(type,"audio type '%s' not found",name)
  
  struct gp1_audio_delegate delegate={
    .cb_pcm_out=live_audio_cb_pcm_out,
  };
  live_audio_samplec=0;
  
  struct gp1_audio *audio=gp1_audio_new(type,&delegate,rate,chanc);
  GP1_ASSERT(audio,"init audio '%s' failed rate=%d chanc=%d",name,rate,chanc)
  if ((audio->rate==rate)&&(audio->chanc==chanc)) {
    fprintf(stderr,"%s: Got rate=%d chanc=%d as requested.\n",name,rate,chanc);
  } else {
    fprintf(stderr,"%s: Requested (%d,%d), got (%d,%d).\n",name,rate,chanc,audio->rate,audio->chanc);
    rate=audio->rate;
    chanc=audio->chanc;
  }
  if (audio->chanc>2) fprintf(stderr,"WARNING: chanc>2, we are going to emit silence\n");
  live_audio_halfperiod=audio->rate/(tone_rate*2);
  
  fprintf(stderr,"Sleeping one second to ensure nothing generates before play...\n");
  gp1_sleep(1000000);
  GP1_ASSERT_INTS(live_audio_samplec,0)
  int64_t stop_time=gp1_now_us()+10000000;
  
  GP1_ASSERT_CALL(gp1_audio_play(audio,1))
  
  fprintf(stderr,"Playing some audio. ENTER to quit, or wait about ten seconds.\n");
  while (1) {
    if (gp1_now_us()>=stop_time) {
      fprintf(stderr,"Ten seconds elapsed, quitting.\n");
      break;
    }
    if (gp1_audio_update(audio)<0) {
      gp1_audio_del(audio);
      GP1_FAIL("!!! Updating audio '%s' failed !!!",name)
    }
    struct pollfd pollfd={.fd=STDIN_FILENO,.events=POLLIN|POLLERR|POLLHUP};
    if (poll(&pollfd,1,20)>0) {
      fprintf(stderr,"Received input on stdin, quitting.\n");
      char buf[256];
      read(STDIN_FILENO,buf,sizeof(buf));
      break;
    }
  }
  GP1_ASSERT_INTS_OP(live_audio_samplec,>,0)
  
  gp1_audio_del(audio);

  return 0;
}

/* Examine each driver type and validate some generic assumptions.
 */
 
GP1_ITEST(audio_drivers) {
  int typec=0;
  for (;;typec++) {
    const struct gp1_audio_type *type=gp1_audio_type_by_index(typec);
    if (!type) break;
    
    GP1_ASSERT(type->name,"p=%d, name must not be null",typec)
    int namec=0;
    for (;type->name[namec];namec++) {
      if ((type->name[namec]<0x20)||(type->name[namec]>0x7e)) {
        GP1_FAIL("p=%d, illegal byte 0x%02x in name",typec,(unsigned char)type->name[namec])
      }
    }
    GP1_ASSERT_INTS_OP(namec,>,0,"p=%d",typec)
    GP1_ASSERT_INTS_OP(namec,<,64,"p=%d",typec)
    
    GP1_ASSERT(gp1_audio_type_by_name(type->name,-1)==type,"audio '%s'",type->name)
    
    GP1_ASSERT(type->desc,"audio '%s', desc must not be null",type->name)
    int descc=0;
    for (;type->desc[descc];descc++) {
      if ((type->desc[descc]<0x20)||(type->desc[descc]>0x7e)) {
        GP1_FAIL("audio '%s', illegal byte 0x%02x in desc",type->name,(unsigned char)type->desc[descc])
      }
    }
    
    GP1_ASSERT_INTS_OP(type->objlen,>=,sizeof(struct gp1_audio),"audio '%s'",type->name)
    
    GP1_ASSERT(type->init,"audio '%s'",type->name)
    GP1_ASSERT(type->play,"audio '%s'",type->name)
    
    if (type->update) {
    } else if (type->lock&&type->unlock) {
    } else {
      GP1_FAIL("audio '%s', must implement (update) or (lock,unlock)",type->name)
    }
  }
  GP1_ASSERT_INTS_OP(typec,>,0,"No audio drivers compiled!")
  return 0;
}
