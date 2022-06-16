#include "gp1_io_audio.h"
#include "gp1_io_clock.h"
#include <stdio.h>

#define GP1_NULLAUDIO_BUFFER_LIMIT_FRAMES 128
#define GP1_NULLAUDIO_CHANC_LIMIT 8
#define GP1_NULLAUDIO_BUFFER_LIMIT_SAMPLES (GP1_NULLAUDIO_BUFFER_LIMIT_FRAMES*GP1_NULLAUDIO_CHANC_LIMIT)
#define GP1_NULLAUDIO_RATE_LIMIT 1000000

/* Instance definition.
 */
 
struct gp1_audio_nullaudio {
  struct gp1_audio hdr;
  int64_t last_update_us;
  int resetc;
  int64_t framec;
  int16_t buffer[GP1_NULLAUDIO_BUFFER_LIMIT_SAMPLES];
};

#define AUDIO ((struct gp1_audio_nullaudio*)audio)

/* Cleanup.
 */
 
static void _gp1_nullaudio_del(struct gp1_audio *audio) {
}

/* Init.
 */
 
static int _gp1_nullaudio_init(struct gp1_audio *audio) {

  // Loose sanity limits on (rate,chanc), in truth we don't care much.
  if ((audio->rate<1)||(audio->rate>GP1_NULLAUDIO_RATE_LIMIT)) return -1;
  if ((audio->chanc<1)||(audio->chanc>GP1_NULLAUDIO_CHANC_LIMIT)) return -1;
  
  return 0;
}

/* Play.
 */
 
static int _gp1_nullaudio_play(struct gp1_audio *audio,int play) {
  audio->playing=play;
  return 0;
}

/* Generate and discard PCM.
 */
 
static int gp1_nullaudio_generate(struct gp1_audio *audio,int framec) {
  int samplec=framec*audio->chanc;
  if (audio->playing&&audio->delegate.cb_pcm_out) {
    if (audio->delegate.cb_pcm_out(AUDIO->buffer,samplec,audio)<0) return -1;
  } else {
    //memset(AUDIO->buffer,0,sizeof(int16_t)*samplec);
  }
  return 0;
}
 
static int gp1_nullaudio_pump_provider(struct gp1_audio *audio,int framec) {
  AUDIO->framec+=framec;
  while (framec>=GP1_NULLAUDIO_BUFFER_LIMIT_FRAMES) {
    if (gp1_nullaudio_generate(audio,GP1_NULLAUDIO_BUFFER_LIMIT_FRAMES)<0) return -1;
    framec-=GP1_NULLAUDIO_BUFFER_LIMIT_FRAMES;
  }
  if (framec>0) {
    if (gp1_nullaudio_generate(audio,framec)<0) return -1;
  }
  return 0;
}

/* Update.
 */
 
static int _gp1_nullaudio_update(struct gp1_audio *audio) {

  // Timing is deferred to the first update, so we don't clog up with all the game's init time.
  if (!AUDIO->last_update_us) {
    AUDIO->last_update_us=gp1_now_us();
    return 0;
  }
  
  // Measure elapsed time since last update.
  // If it's negative or unreasonably high, reset.
  int64_t now=gp1_now_us();
  int64_t elapsedus=now-AUDIO->last_update_us;
  if ((elapsedus<0)||(elapsedus>1000000)) {
    AUDIO->last_update_us=now;
    AUDIO->resetc++;
    return 0;
  }
  int64_t framec=(elapsedus*1000000)/audio->rate;
  if (framec<1) return 0;
  
  // Generate PCM and throw it away.
  if (gp1_nullaudio_pump_provider(audio,framec)<0) return -1;
  
  // Advance timer based on what we generated, rather than assigning from (now). Dodges rounding errors.
  AUDIO->last_update_us+=(framec*1000000)/audio->rate;

  return 0;
}

/* Type definition.
 */
 
const struct gp1_audio_type gp1_audio_type_nullaudio={
  .name="nullaudio",
  .desc="Dummy driver that tracks time correctly but discards output.",
  .objlen=sizeof(struct gp1_audio_nullaudio),
  .del=_gp1_nullaudio_del,
  .init=_gp1_nullaudio_init,
  .play=_gp1_nullaudio_play,
  .update=_gp1_nullaudio_update,
};
