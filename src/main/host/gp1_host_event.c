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

/* Digested input state.
 */
 
int _gp1_host_inmgr_state(struct gp1_inmgr *inmgr,int playerid,int btnid,int value) {
  struct gp1_host *host=gp1_inmgr_get_userdata(inmgr);
  //fprintf(stderr,"%s %d.%04x=%d\n",__func__,playerid,btnid,value);
  return gp1_vm_input_event(host->vm,playerid,btnid,value);
}

/* Stateless action.
 */
 
int _gp1_host_inmgr_action(struct gp1_inmgr *inmgr,int action) {
  struct gp1_host *host=gp1_inmgr_get_userdata(inmgr);
  fprintf(stderr,"%s 0x%08x\n",__func__,action);
  switch (action) {
    case GP1_ACTION_QUIT: host->quit_requested=1; break;
    case GP1_ACTION_RESET: break;//TODO
    case GP1_ACTION_SUSPEND: host->suspend=1; break;
    case GP1_ACTION_RESUME: host->suspend=0; break;
    case GP1_ACTION_SCREENCAP: break;//TODO
    case GP1_ACTION_LOAD_STATE: break;//TODO
    case GP1_ACTION_SAVE_STATE: break;//TODO
    case GP1_ACTION_MENU: break;//TODO
    case GP1_ACTION_STEP_FRAME: host->step=1; break;
    case GP1_ACTION_FULLSCREEN: return gp1_video_set_fullscreen(host->video,-1);
    default: fprintf(stderr,"%s:WARNING: Unimplemented action %d\n",host->config->exename,action);
  }
  return 0;
}

/* Window closed.
 */

int _gp1_host_video_close(struct gp1_video *video) {
  struct gp1_host *host=video->delegate.userdata;
  host->quit_requested=1;
  return 0;
}

/* Window gained or lost focus.
 */
 
int _gp1_host_video_focus(struct gp1_video *video,int focus) {
  struct gp1_host *host=video->delegate.userdata;
  return 0;
}

/* Window resized.
 */
 
int _gp1_host_video_resize(struct gp1_video *video,int w,int h) {
  struct gp1_host *host=video->delegate.userdata;
  if (host->renderer) {
    host->renderer->mainw=w;
    host->renderer->mainh=h;
  }
  return 0;
}

/* Raw keyboard input from window manager.
 */
 
int _gp1_host_video_key(struct gp1_video *video,int keycode,int value) {
  struct gp1_host *host=video->delegate.userdata;
  //TODO Suspend keyboard events while text entry in progress (GUI).
  int err=gp1_inmgr_key(host->inmgr,keycode,value);
  if (err) return err;
  return 0;
}

/* Digested keyboard and mouse events from window manager.
 * TODO Deliver to GUI.
 */
 
int _gp1_host_video_text(struct gp1_video *video,int codepoint) {
  struct gp1_host *host=video->delegate.userdata;
  return 0;
}
 
int _gp1_host_video_mmotion(struct gp1_video *video,int x,int y) {
  struct gp1_host *host=video->delegate.userdata;
  return 0;
}
 
int _gp1_host_video_mbutton(struct gp1_video *video,int btnid,int value) {
  struct gp1_host *host=video->delegate.userdata;
  return 0;
}
 
int _gp1_host_video_mwheel(struct gp1_video *video,int dx,int dy) {
  struct gp1_host *host=video->delegate.userdata;
  return 0;
}

/* General input events (let inmgr do all the work).
 */

int _gp1_host_input_connect(struct gp1_input *input,int devid) {
  struct gp1_host *host=input->delegate.userdata;
  return gp1_inmgr_connect(host->inmgr,input,devid);
}
 
int _gp1_host_input_disconnect(struct gp1_input *input,int devid) {
  struct gp1_host *host=input->delegate.userdata;
  return gp1_inmgr_disconnect(host->inmgr,input,devid);
}
 
int _gp1_host_input_button(struct gp1_input *input,int devid,int btnid,int value) {
  struct gp1_host *host=input->delegate.userdata;
  return gp1_inmgr_button(host->inmgr,input,devid,btnid,value);
}

/* Audio driver ready for PCM.
 */

int _gp1_host_audio_pcm_out(int16_t *v,int c,struct gp1_audio *audio) {
  struct gp1_host *host=audio->delegate.userdata;
  return gp1_vm_synthesize(v,c,host->vm);
}
