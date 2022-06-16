#ifndef GP1_ALSA_INTERNAL_H
#define GP1_ALSA_INTERNAL_H

#include "io/gp1_io_audio.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define ALSA_BUFFER_SIZE 1024

#define GP1_ALSA_RATE_MIN 200
#define GP1_ALSA_RATE_MAX 200000
#define GP1_ALSA_CHANC_MIN 1
#define GP1_ALSA_CHANC_MAX 2

struct gp1_audio_alsa {
  struct gp1_audio hdr;

  snd_pcm_t *alsa;
  snd_pcm_hw_params_t *hwparams;

  int hwbuffersize;
  int bufc; // frames
  int bufc_samples;
  char *buf;

  pthread_t iothd;
  pthread_mutex_t iomtx;
  int ioabort;
  int cberror;
};

#define DRIVER ((struct gp1_audio_alsa*)driver)

#endif
