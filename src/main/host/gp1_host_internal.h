#ifndef GP1_HOST_INTERNAL_H
#define GP1_HOST_INTERNAL_H

#include "gp1_host.h"
#include "main/gp1_cli.h"
#include "vm/gp1_vm.h"
#include "vm/gp1_rom.h"
#include "io/gp1_io_video.h"
#include "io/gp1_io_audio.h"
#include "io/gp1_io_input.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define GP1_UPDATE_RATE 60

// vm delegate
int _gp1_host_render(struct gp1_vm *vm,const void *src,int srcc);
int _gp1_host_store(struct gp1_vm *vm,uint32_t k,const void *v,int c);
int _gp1_host_load(struct gp1_vm *vm,void *v,int a,uint32_t k);
int _gp1_host_http_request(struct gp1_vm *vm,struct gp1_http_request *request);
int _gp1_host_ws_connect(struct gp1_vm *vm,int wsid,const char *url,int urlc);
int _gp1_host_ws_send(struct gp1_vm *vm,int wsid,const void *src,int srcc);

// video delegate
int _gp1_host_video_close(struct gp1_video *video);
int _gp1_host_video_focus(struct gp1_video *video,int focus);
int _gp1_host_video_resize(struct gp1_video *video,int w,int h);
int _gp1_host_video_key(struct gp1_video *video,int keycode,int value);
int _gp1_host_video_text(struct gp1_video *video,int codepoint);
int _gp1_host_video_mmotion(struct gp1_video *video,int x,int y);
int _gp1_host_video_mbutton(struct gp1_video *video,int btnid,int value);
int _gp1_host_video_mwheel(struct gp1_video *video,int dx,int dy);

// input delegate
int _gp1_host_input_connect(struct gp1_input *input,int devid);
int _gp1_host_input_disconnect(struct gp1_input *input,int devid);
int _gp1_host_input_button(struct gp1_input *input,int devid,int btnid,int value);

// audio delegate
int _gp1_host_audio_pcm_out(int16_t *v,int c,struct gp1_audio *audio);

#endif
