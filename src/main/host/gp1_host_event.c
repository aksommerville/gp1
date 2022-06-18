#include "gp1_host_internal.h"

/* Render.
 */
 
int _gp1_host_render(struct gp1_vm *vm,const void *src,int srcc) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  return gp1_renderer_render(host->renderer,src,srcc);
}

/* Store game data.
 */
 
int _gp1_host_store(struct gp1_vm *vm,uint32_t k,const void *v,int c) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  fprintf(stderr,"%s k=0x%08x c=%d:",__func__,k,c);
  int printc=(c>50)?50:c;
  int i=0; for (;i<printc;i++) fprintf(stderr," %02x",((uint8_t*)v)[i]);
  fprintf(stderr,"\n");
  return 0;
}

/* Load game data.
 */
 
int _gp1_host_load(struct gp1_vm *vm,void *v,int a,uint32_t k) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  fprintf(stderr,"%s k=0x%08x a=%d\n",__func__,k,a);
  return 0;
}

/* Begin HTTP request.
 */
 
int _gp1_host_http_request(struct gp1_vm *vm,struct gp1_http_request *request) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  fprintf(stderr,"%s\n",__func__);
  return 0;
}

/* Connect WebSocket.
 */
 
int _gp1_host_ws_connect(struct gp1_vm *vm,int wsid,const char *url,int urlc) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  fprintf(stderr,"%s wsid=%d url='%.*s'\n",__func__,wsid,urlc,url);
  return 0;
}

/* Send WebSocket packet.
 */
 
int _gp1_host_ws_send(struct gp1_vm *vm,int wsid,const void *src,int srcc) {
  struct gp1_host *host=gp1_vm_get_userdata(vm);
  fprintf(stderr,"%s wsid=%d c=%d\n",__func__,wsid,srcc);
  return 0;
}

/* Window closed.
 */

int _gp1_host_video_close(struct gp1_video *video) {
  struct gp1_host *host=video->delegate.userdata;
  fprintf(stderr,"%s\n",__func__);
  host->quit_requested=1;
  return 0;
}

/* Window gained or lost focus.
 */
 
int _gp1_host_video_focus(struct gp1_video *video,int focus) {
  struct gp1_host *host=video->delegate.userdata;
  fprintf(stderr,"%s %d\n",__func__,focus);
  return 0;
}

/* Window resized.
 */
 
int _gp1_host_video_resize(struct gp1_video *video,int w,int h) {
  struct gp1_host *host=video->delegate.userdata;
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
  return 0;
}

/* Raw keyboard input from window manager.
 */
 
int _gp1_host_video_key(struct gp1_video *video,int keycode,int value) {
  struct gp1_host *host=video->delegate.userdata;
  fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  
  //XXX TEMP hard-code a few key bindings helpful during development
  if (value) switch (keycode) {
    case 0x00070029: host->quit_requested=1; break; // Escape
  }
  
  return 0;
}

/* Unicode keyboard input from window manager.
 */
 
int _gp1_host_video_text(struct gp1_video *video,int codepoint) {
  struct gp1_host *host=video->delegate.userdata;
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
  return 0;
}

/* Mouse motion.
 */
 
int _gp1_host_video_mmotion(struct gp1_video *video,int x,int y) {
  struct gp1_host *host=video->delegate.userdata;
  //fprintf(stderr,"%s %d,%d\n",__func__,x,y);
  return 0;
}

/* Mouse button.
 */
 
int _gp1_host_video_mbutton(struct gp1_video *video,int btnid,int value) {
  struct gp1_host *host=video->delegate.userdata;
  //fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
  return 0;
}

/* Mouse wheel.
 */
 
int _gp1_host_video_mwheel(struct gp1_video *video,int dx,int dy) {
  struct gp1_host *host=video->delegate.userdata;
  //fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
  return 0;
}

/* Connected input device.
 */

int _gp1_host_input_connect(struct gp1_input *input,int devid) {
  struct gp1_host *host=input->delegate.userdata;
  fprintf(stderr,"%s %d\n",__func__,devid);
  return 0;
}

/* Disconnected input device.
 */
 
int _gp1_host_input_disconnect(struct gp1_input *input,int devid) {
  struct gp1_host *host=input->delegate.userdata;
  fprintf(stderr,"%s %d\n",__func__,devid);
  return 0;
}

/* Event on general input device.
 */
 
int _gp1_host_input_button(struct gp1_input *input,int devid,int btnid,int value) {
  struct gp1_host *host=input->delegate.userdata;
  fprintf(stderr,"%s %d.0x%08x=%d\n",__func__,devid,btnid,value);
  return 0;
}

/* Audio driver ready for PCM.
 */

int _gp1_host_audio_pcm_out(int16_t *v,int c,struct gp1_audio *audio) {
  struct gp1_host *host=audio->delegate.userdata;
  return gp1_vm_synthesize(v,c,host->vm);
}
