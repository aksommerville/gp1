/* gp1_vm.h
 * The GP1 runtime, with no I/O.
 * This API is for emulators. You supply the host-side I/O, we take care of game logic.
 */
 
#ifndef GP1_VM_H
#define GP1_VM_H

#include <stdint.h>

struct gp1_vm;
struct gp1_rom;
struct gp1_http_request;//TODO
struct gp1_http_response;

struct gp1_vm_delegate {
  void *userdata;
  
  /* Render a stream of commands.
   * There's any number of strategies you might employ, and the VM doesn't favor one.
   * See gp1_vm_render.h for built-in renderers.
   */
  int (*render)(struct gp1_vm *vm,const void *src,int srcc);
  
  /* Request from the game to store or load persistent data.
   * VM ensures that you will only receive requests consistent with the ROM file's headers.
   * load() must return the amount read, (0..a), or <0 for errors.
   * These must complete synchronously.
   */
  int (*store)(struct gp1_vm *vm,uint32_t k,const void *v,int c);
  int (*load)(struct gp1_vm *vm,void *v,int a,uint32_t k);
  
  /* Begin an HTTP request.
   * VM ensures that the request agrees with ROM file's headers.
   * This is asynchronous. You must later report the outcome via gp1_vm_http_response(), or return <0 here.
   */
  int (*http_request)(
    struct gp1_vm *vm,
    struct gp1_http_request *request
  );
  
  /* Open a WebSocket connection to the given (url), with opaque identifier (wsid).
   * This does not need to complete synchronously.
   * You are free to report the connection status during this call, or any time later.
   * Failing this call is equivalent to reporting a connection failure.
   */
  int (*ws_connect)(struct gp1_vm *vm,int wsid,const char *url,int urlc);
  
  /* Send data to a WebSocket.
   * VM ensures that this socket is open.
   */
  int (*ws_send)(struct gp1_vm *vm,int wsid,const void *src,int srcc);
};

void gp1_vm_del(struct gp1_vm *vm);
int gp1_vm_ref(struct gp1_vm *vm);

struct gp1_vm *gp1_vm_new(const struct gp1_vm_delegate *delegate);

void *gp1_vm_get_userdata(const struct gp1_vm *vm);

/* Attach a ROM file.
 * No game code executes during this call.
 */
int gp1_vm_load_rom(struct gp1_vm *vm,struct gp1_rom *rom);

/* Wipe transient state and execute the game's initializer.
 * You should do this once per game, immediately after loading.
 * It is safe to initialize again later, that effectively reboots the system.
 */
int gp1_vm_init_game(struct gp1_vm *vm);

/* Execute one pass of the game's main loop, and generate one frame of video.
 * Expect your render() callback to fire once (maybe more) during this call, but that's not guaranteed.
 */
int gp1_vm_update(struct gp1_vm *vm);

/* Set state of one input button.
 * (playerid) in 1..8 (GP1_PLAYER_FIRST..GP1_PLAYER_LAST).
 * (btnid) is a single bit among the low 16 (GP1_BTNID_*, in gp1/gp1.h).
 */
int gp1_vm_input_event(struct gp1_vm *vm,int playerid,int btnid,int value);

/* Initialize the synthesizer with whatever rate and channel count your driver expects, we're flexible.
 * It is safe to call gp1_synthesize() while other gp1 calls are running -- that is not true of gp1 functions in general.
 * It's not a big deal if your video and audio timing go out of sync.
 */
int gp1_vm_init_synth(struct gp1_vm *vm,int rate,int chanc);
int gp1_vm_synthesize(int16_t *v,int c,struct gp1_vm *vm);

/* Deliver an asynchronous response to an HTTP request you previous received through your delegate.
 * You must retain the request object, and deliver it again here.
 * A response is expected for all requests, unless you fail it synchronously.
 */
int gp1_vm_http_response(
  struct gp1_vm *vm,
  const struct gp1_http_request *request,
  const struct gp1_http_response *response
);

/* Report the connection status of a WebSocket, requested via your delegate.
 */
int gp1_vm_ws_connect_fail(struct gp1_vm *vm,int wsid);
int gp1_vm_ws_connect_ok(struct gp1_vm *vm,int wsid);

/* Report messages or disconnection of a WebSocket.
 * You must report the payloads only, not the framing.
 * (in theory, you could use some other protocol).
 */
int gp1_vm_ws_receive(struct gp1_vm *vm,int wsid,const void *src,int srcc);
int gp1_vm_ws_disconnect(struct gp1_vm *vm,int wsid);

#endif
