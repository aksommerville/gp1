#include "gp1_alsa_internal.h"

/* Delete.
 */
 
static void _alsa_del(struct gp1_audio *driver) {
  DRIVER->ioabort=1;
  if (DRIVER->iothd&&!DRIVER->cberror) {
    pthread_cancel(DRIVER->iothd);
    pthread_join(DRIVER->iothd,0);
  }
  pthread_mutex_destroy(&DRIVER->iomtx);
  if (DRIVER->hwparams) snd_pcm_hw_params_free(DRIVER->hwparams);
  if (DRIVER->alsa) snd_pcm_close(DRIVER->alsa);
  if (DRIVER->buf) free(DRIVER->buf);
}

/* I/O thread.
 */

static void *_alsa_iothd(void *dummy) {
  struct gp1_audio *driver=dummy;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);
  while (1) {
    pthread_testcancel();

    if (pthread_mutex_lock(&DRIVER->iomtx)) {
      DRIVER->cberror=1;
      return 0;
    }
    if (driver->playing) {
      driver->delegate.cb_pcm_out((int16_t*)DRIVER->buf,DRIVER->bufc_samples,driver);
    } else {
      memset(DRIVER->buf,0,DRIVER->bufc_samples*2);
    }
    pthread_mutex_unlock(&DRIVER->iomtx);
    if (DRIVER->ioabort) return 0;

    char *samplev=DRIVER->buf;
    int samplep=0,samplec=DRIVER->bufc;
    while (samplep<samplec) {
      pthread_testcancel();
      int err=snd_pcm_writei(DRIVER->alsa,samplev+samplep*2,samplec-samplep);
      if (DRIVER->ioabort) return 0;
      if (err<=0) {
        if ((err=snd_pcm_recover(DRIVER->alsa,err,0))<0) {
          DRIVER->cberror=1;
          return 0;
        }
        break;
      }
      samplep+=err;
    }
  }
  return 0;
}

/* Init.
 */
 
static int _alsa_init(struct gp1_audio *driver) {
  if ((driver->rate<GP1_ALSA_RATE_MIN)||(driver->rate>GP1_ALSA_RATE_MAX)) driver->rate=44100;
  if ((driver->chanc<GP1_ALSA_CHANC_MIN)||(driver->chanc>GP1_ALSA_CHANC_MAX)) driver->chanc=2;
  
  const char *device="default"; // TODO configurable
  
  /**
  fprintf(stderr,
    "%s:%d: Ready to initialize ALSA. device=%s rate=%d chanc=%d format=%d\n",
    __FILE__,__LINE__,
    driver->delegate.device,
    driver->delegate.rate,
    driver->delegate.chanc,
    driver->delegate.format
  );
  /**/
  
  if (
    (snd_pcm_open(&DRIVER->alsa,device,SND_PCM_STREAM_PLAYBACK,0)<0)||
    (snd_pcm_hw_params_malloc(&DRIVER->hwparams)<0)||
    (snd_pcm_hw_params_any(DRIVER->alsa,DRIVER->hwparams)<0)||
    (snd_pcm_hw_params_set_access(DRIVER->alsa,DRIVER->hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)<0)||
    (snd_pcm_hw_params_set_format(DRIVER->alsa,DRIVER->hwparams,SND_PCM_FORMAT_S16)<0)||
    (snd_pcm_hw_params_set_rate_near(DRIVER->alsa,DRIVER->hwparams,&driver->rate,0)<0)||
    (snd_pcm_hw_params_set_channels_near(DRIVER->alsa,DRIVER->hwparams,&driver->chanc)<0)||
    (snd_pcm_hw_params_set_buffer_size(DRIVER->alsa,DRIVER->hwparams,ALSA_BUFFER_SIZE)<0)||
    (snd_pcm_hw_params(DRIVER->alsa,DRIVER->hwparams)<0)
  ) return -1;
  
  //fprintf(stderr,"...rate=%d chanc=%d\n",driver->delegate.rate,driver->delegate.chanc);
  
  if (snd_pcm_nonblock(DRIVER->alsa,0)<0) return -1;
  if (snd_pcm_prepare(DRIVER->alsa)<0) return -1;

  DRIVER->bufc=ALSA_BUFFER_SIZE;
  DRIVER->bufc_samples=DRIVER->bufc*driver->chanc;
  if (!(DRIVER->buf=malloc(DRIVER->bufc_samples*2))) return -1;

  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr,PTHREAD_MUTEX_RECURSIVE);
  if (pthread_mutex_init(&DRIVER->iomtx,&mattr)) return -1;
  pthread_mutexattr_destroy(&mattr);
  if (pthread_create(&DRIVER->iothd,0,_alsa_iothd,driver)) return -1;
  return 0;
}

/* Play.
 */
 
static int _alsa_play(struct gp1_audio *driver,int play) {
  driver->playing=play;
  return 0;
}

/* Lock.
 */
 
static int _alsa_update(struct gp1_audio *driver) {
  if (DRIVER->cberror) {
    DRIVER->cberror=0;
    return -1;
  }
  return 0;
}
 
static int _alsa_lock(struct gp1_audio *driver) {
  if (pthread_mutex_lock(&DRIVER->iomtx)) return -1;
  return 0;
}

static void _alsa_unlock(struct gp1_audio *driver) {
  pthread_mutex_unlock(&DRIVER->iomtx);
}

/* Type.
 */
 
const struct gp1_audio_type gp1_audio_type_alsa={
  .name="alsa",
  .desc="Linux audio via ALSA. Preferred for headless systems.",
  .objlen=sizeof(struct gp1_audio_alsa),
  .del=_alsa_del,
  .init=_alsa_init,
  .play=_alsa_play,
  .update=_alsa_update,
  .lock=_alsa_lock,
  .unlock=_alsa_unlock,
};
