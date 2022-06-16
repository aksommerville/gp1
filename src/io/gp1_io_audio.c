#include "gp1_io_audio.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* Global type registry.
 ******************************************************************************/
 
// These are mostly optional and might not actually exist.
extern const struct gp1_audio_type gp1_audio_type_alsa;
extern const struct gp1_audio_type gp1_audio_type_pulse;
extern const struct gp1_audio_type gp1_audio_type_macaudio;
extern const struct gp1_audio_type gp1_audio_type_msaudio;
extern const struct gp1_audio_type gp1_audio_type_nullaudio;
 
static const struct gp1_audio_type *gp1_audio_typev[]={
#if GP1_USE_alsa
  &gp1_audio_type_alsa,
#endif
#if GP1_USE_pulse
  &gp1_audio_type_pulse,
#endif
#if GP1_USE_macaudio
  &gp1_audio_type_macaudio,
#endif
#if GP1_USE_msaudio
  &gp1_audio_type_msaudio,
#endif
  &gp1_audio_type_nullaudio,
};

const struct gp1_audio_type *gp1_audio_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(gp1_audio_typev)/sizeof(void*);
  if (p>=c) return 0;
  return gp1_audio_typev[p];
}

const struct gp1_audio_type *gp1_audio_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  const struct gp1_audio_type **p=gp1_audio_typev;
  int i=sizeof(gp1_audio_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (memcmp(name,(*p)->name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Audio driver wrapper.
 ********************************************************************************/

void gp1_audio_del(struct gp1_audio *audio) {
  if (!audio) return;
  if (audio->refc-->1) return;
  
  if (audio->type->del) audio->type->del(audio);
  
  free(audio);
}

int gp1_audio_ref(struct gp1_audio *audio) {
  if (!audio) return -1;
  if (audio->refc<1) return -1;
  if (audio->refc==INT_MAX) return -1;
  audio->refc++;
  return 0;
}

struct gp1_audio *gp1_audio_new(
  const struct gp1_audio_type *type,
  const struct gp1_audio_delegate *delegate,
  int rate,int chanc
) {
  if (!type) return 0;
  
  struct gp1_audio *audio=calloc(1,type->objlen);
  if (!audio) return 0;
  
  audio->refc=1;
  audio->type=type;
  if (delegate) audio->delegate=*delegate;
  audio->rate=rate;
  audio->chanc=chanc;
  
  if (type->init&&(type->init(audio)<0)) {
    gp1_audio_del(audio);
    return 0;
  }
  
  return audio;
}

int gp1_audio_play(struct gp1_audio *audio,int play) {
  if (!audio) return 0;
  if (play) {
    if (audio->playing) return 0;
  } else {
    if (!audio->playing) return 0;
  }
  if (!audio->type->play) return -1;
  return audio->type->play(audio,play);
}

int gp1_audio_update(struct gp1_audio *audio) {
  if (!audio) return -1;
  if (!audio->type->update) return 0;
  return audio->type->update(audio);
}

int gp1_audio_lock(struct gp1_audio *audio) {
  if (!audio) return -1;
  if (!audio->type->lock) return -1;
  return audio->type->lock(audio);
}

void gp1_audio_unlock(struct gp1_audio *audio) {
  if (!audio) return;
  if (!audio->type->unlock) return;
  audio->type->unlock(audio);
}
