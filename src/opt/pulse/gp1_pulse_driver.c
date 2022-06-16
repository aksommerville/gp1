#include "gp1_pulse_internal.h"

/* I/O thread.
 */
 
static void *pulse_iothd(void *arg) {
  struct gp1_audio *driver=arg;
  while (1) {
    if (DRIVER->iocancel) return 0;
    
    // Fill buffer while holding the lock.
    if (pthread_mutex_lock(&DRIVER->iomtx)) {
      DRIVER->ioerror=-1;
      return 0;
    }
    if (driver->playing) {
      driver->delegate.cb_pcm_out(DRIVER->buf,DRIVER->bufa,driver);
    } else {
      memset(DRIVER->buf,0,DRIVER->bufa*2);
    }
    if (pthread_mutex_unlock(&DRIVER->iomtx)) {
      DRIVER->ioerror=-1;
      return 0;
    }
    if (DRIVER->iocancel) return 0;
    
    // Deliver to PulseAudio.
    int err=0,result;
    result=pa_simple_write(DRIVER->pa,DRIVER->buf,sizeof(int16_t)*DRIVER->bufa,&err);
    if (DRIVER->iocancel) return 0;
    if (result<0) {
      DRIVER->ioerror=-1;
      return 0;
    }
  }
}

/* Terminate I/O thread.
 */
 
static void pulse_abort_io(struct gp1_audio *driver) {
  if (!DRIVER->iothd) return;
  DRIVER->iocancel=1;
  pthread_join(DRIVER->iothd,0);
  DRIVER->iothd=0;
}

/* Delete.
 */
 
static void _pulse_del(struct gp1_audio *driver) {
  pulse_abort_io(driver);
  pthread_mutex_destroy(&DRIVER->iomtx);
  if (DRIVER->pa) {
    pa_simple_free(DRIVER->pa);
  }
}

/* Init PulseAudio client.
 */
 
static int pulse_init_pa(struct gp1_audio *driver) {
  int err;

  pa_sample_spec sample_spec={
    #if BYTE_ORDER==BIG_ENDIAN
      .format=PA_SAMPLE_S16BE,
    #else
      .format=PA_SAMPLE_S16LE,
    #endif
    .rate=driver->rate,
    .channels=driver->chanc,
  };
  int bufframec=driver->rate/20; //TODO more sophisticated buffer length decision
  if (bufframec<20) bufframec=20;
  pa_buffer_attr buffer_attr={
    .maxlength=driver->chanc*sizeof(int16_t)*bufframec,
    .tlength=driver->chanc*sizeof(int16_t)*bufframec,
    .prebuf=0xffffffff,
    .minreq=0xffffffff,
  };
  
  if (!(DRIVER->pa=pa_simple_new(
    0, // server name
    "GP1", // our name TODO I'd rather not hard-code this in the driver, can we provide some other way?
    PA_STREAM_PLAYBACK,
    0, // sink name (?)
    "GP1", // stream (as opposed to client) name
    &sample_spec,
    0, // channel map
    &buffer_attr,
    &err
  ))) {
    return -1;
  }
  
  driver->rate=sample_spec.rate;
  driver->chanc=sample_spec.channels;
  
  return 0;
}

/* With the final rate and channel count settled, calculate a good buffer size and allocate it.
 */
 
static int pulse_init_buffer(struct gp1_audio *driver) {

  const double buflen_target_s= 0.010; // about 100 Hz
  const int buflen_min=           128; // but in no case smaller than N samples
  const int buflen_max=         16384; // ...nor larger
  
  // Initial guess and clamp to the hard boundaries.
  DRIVER->bufa=buflen_target_s*driver->rate*driver->chanc;
  if (DRIVER->bufa<buflen_min) {
    DRIVER->bufa=buflen_min;
  } else if (DRIVER->bufa>buflen_max) {
    DRIVER->bufa=buflen_max;
  }
  // Reduce to next multiple of channel count.
  DRIVER->bufa-=DRIVER->bufa%driver->chanc;
  
  if (!(DRIVER->buf=malloc(sizeof(int16_t)*DRIVER->bufa))) {
    return -1;
  }
  
  return 0;
}

/* Init thread.
 */
 
static int pulse_init_thread(struct gp1_audio *driver) {
  int err;
  if (err=pthread_mutex_init(&DRIVER->iomtx,0)) return -1;
  if (err=pthread_create(&DRIVER->iothd,0,pulse_iothd,driver)) return -1;
  return 0;
}

/* Init.
 */
 
static int _pulse_init(struct gp1_audio *driver) {
  if ((driver->rate<GP1_PULSE_RATE_MIN)||(driver->rate>GP1_PULSE_RATE_MAX)) driver->rate=44100;
  if ((driver->chanc<1)||(driver->chanc>2)) driver->chanc=2;
  if (
    (pulse_init_pa(driver)<0)||
    (pulse_init_buffer(driver)<0)||
    (pulse_init_thread(driver)<0)||
  0) return -1;
  return 0;
}

/* Play.
 */
 
static int _pulse_play(struct gp1_audio *driver,int play) {
  driver->playing=play;
  return 0;
}

/* Lock.
 */
 
static int _pulse_lock(struct gp1_audio *driver) {
  if (pthread_mutex_lock(&DRIVER->iomtx)<0) return -1;
  return 0;
}

static void _pulse_unlock(struct gp1_audio *driver) {
  pthread_mutex_unlock(&DRIVER->iomtx);
}

/* Type.
 */
 
const struct gp1_audio_type gp1_audio_type_pulse={
  .name="pulse",
  .desc="Linux audio via PulseAudio. Preferred for desktop environments.",
  .objlen=sizeof(struct gp1_audio_pulse),
  .del=_pulse_del,
  .init=_pulse_init,
  .play=_pulse_play,
  .lock=_pulse_lock,
  .unlock=_pulse_unlock,
};
