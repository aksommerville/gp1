#ifndef GP1_PULSE_INTERNAL_H
#define GP1_PULSE_INTERNAL_H

#include "io/gp1_io_audio.h"
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#define GP1_PULSE_RATE_MIN 200
#define GP1_PULSE_RATE_MAX 200000
#define GP1_PULSE_CHANC_MIN 1
#define GP1_PULSE_CHANC_MAX 2

struct gp1_audio_pulse {
  struct gp1_audio hdr;
  
  pa_simple *pa;
  
  pthread_t iothd;
  pthread_mutex_t iomtx;
  int ioerror;
  int iocancel; // pa_simple doesn't like regular thread cancellation
  
  int16_t *buf;
  int bufa;
};

#define DRIVER ((struct gp1_audio_pulse*)driver)

#endif
