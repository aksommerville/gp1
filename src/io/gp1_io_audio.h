/* gp1_io_audio.h
 * Generic interface for PCM out.
 */
 
#ifndef GP1_IO_AUDIO_H
#define GP1_IO_AUDIO_H

struct gp1_audio;
struct gp1_audio_type;

#include <stdint.h>

struct gp1_audio_delegate {
  void *userdata;
  int (*cb_pcm_out)(int16_t *v,int c,struct gp1_audio *audio);
};

struct gp1_audio {
  const struct gp1_audio_type *type;
  struct gp1_audio_delegate delegate;
  int refc;
  int rate,chanc;
  int playing;
};

void gp1_audio_del(struct gp1_audio *audio);
int gp1_audio_ref(struct gp1_audio *audio);

/* (rate,chanc) are not guaranteed, check the returned instance after construction.
 */
struct gp1_audio *gp1_audio_new(
  const struct gp1_audio_type *type,
  const struct gp1_audio_delegate *delegate,
  int rate,int chanc
);

/* A newly-constructed driver is paused until you explicitly play().
 * Once playing, your callback may fire at any time, probably from a different thread.
 */
int gp1_audio_play(struct gp1_audio *audio,int play);

int gp1_audio_update(struct gp1_audio *audio);

// Your callback is guaranteed not running while you hold the lock.
int gp1_audio_lock(struct gp1_audio *audio);
void gp1_audio_unlock(struct gp1_audio *audio);

struct gp1_audio_type {
  const char *name;
  const char *desc;
  int objlen;
  void (*del)(struct gp1_audio *audio);
  int (*init)(struct gp1_audio *audio);
  int (*play)(struct gp1_audio *audio,int play);
  int (*update)(struct gp1_audio *audio);
  int (*lock)(struct gp1_audio *audio);
  void (*unlock)(struct gp1_audio *audio);
};

const struct gp1_audio_type *gp1_audio_type_by_index(int p);
const struct gp1_audio_type *gp1_audio_type_by_name(const char *name,int namec);

#endif
