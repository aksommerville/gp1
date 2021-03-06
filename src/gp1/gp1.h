/* gp1.h
 * Interface for client games.
 * This describes the API that GP1 exposes via WebAssembly.
 */
 
#ifndef GP1_H
#define GP1_H

#include <stdint.h>

#define GP1_VERSION 0x01000000

/* Client hooks.
 * Each game must implement these.
 * Declared here for documentation.
 ***************************************************************/

/* Called once, when the game starts up.
 */
int32_t gp1_init();

/* Called for each video frame.
 * Return <0 to terminate with an error.
 */
int32_t gp1_update();

// Network hooks are required only if you declare network activity in the header.

/* Receive the response for some prior gp1_http_request().
 */
void gp1_http_response(
  int32_t status,
  const void *body,int bodyc,
  void *userdata
);

/* Report completion of gp1_ws_connect().
 * If (wsid>0), it succeeded.
 */
void gp1_ws_connect_complete(
  void *userdata,
  int32_t wsid
);

/* Receive a packet from a connected WebSocket.
 */
void gp1_ws_receive(
  void *userdata,
  int32_t wsid,
  const void *src,int srcc
);

/* Report the end of a WebSocket connection.
 * This is called regardless of who disconnected it.
 */
void gp1_ws_disconnected(
  void *userdata,
  int32_t wsid
);

/* Input.
 * You must poll for input.
 * States will not change during one update.
 ****************************************************************/
 
#define GP1_PLAYER_AGGREGATE    0
#define GP1_PLAYER_FIRST        1
#define GP1_PLAYER_LAST         8
#define GP1_PLAYER_COUNT        9 /* including the fake "aggregate" player */

#define GP1_BTNID_LEFT      0x0001
#define GP1_BTNID_RIGHT     0x0002
#define GP1_BTNID_UP        0x0004
#define GP1_BTNID_DOWN      0x0008
#define GP1_BTNID_SOUTH     0x0010
#define GP1_BTNID_WEST      0x0020
#define GP1_BTNID_EAST      0x0040
#define GP1_BTNID_NORTH     0x0080
#define GP1_BTNID_L1        0x0100
#define GP1_BTNID_R1        0x0200
#define GP1_BTNID_L2        0x0400
#define GP1_BTNID_R2        0x0800
#define GP1_BTNID_AUX1      0x1000
#define GP1_BTNID_AUX2      0x2000
#define GP1_BTNID_AUX3      0x4000
#define GP1_BTNID_CD        0x8000

#define GP1_FOR_EACH_BTNID \
  _(LEFT) \
  _(RIGHT) \
  _(UP) \
  _(DOWN) \
  _(SOUTH) \
  _(WEST) \
  _(EAST) \
  _(NORTH) \
  _(L1) \
  _(R1) \
  _(L2) \
  _(R2) \
  _(AUX1) \
  _(AUX2) \
  _(AUX3) \
  _(CD)
 
uint16_t gp1_get_input_state(int32_t playerid);

//TODO Query input capabilities

/* Video.
 ******************************************************************/
 
#define GP1_IMAGE_FORMAT_A1       0x00 /* alpha or luma, we don't take a firm opinion. */
#define GP1_IMAGE_FORMAT_A8       0x01 /* '' */
#define GP1_IMAGE_FORMAT_RGB565   0x02
#define GP1_IMAGE_FORMAT_ARGB1555 0x03
#define GP1_IMAGE_FORMAT_RGB888   0x04
#define GP1_IMAGE_FORMAT_RGBA8888 0x05

#define GP1_FOR_EACH_IMAGE_FORMAT \
  _(A1) \
  _(A8) \
  _(RGB565) \
  _(ARGB1555) \
  _(RGB888) \
  _(RGBA8888)
  
#define GP1_VIDEO_OP_EOF             0x00 /* () */
#define GP1_VIDEO_OP_DECLARE_COMMAND 0x01 /* (1 op,2 len) */
#define GP1_VIDEO_OP_FGCOLOR         0x10 /* (4 rgba) */
#define GP1_VIDEO_OP_BGCOLOR         0x11 /* (4 rgba) */
#define GP1_VIDEO_OP_XFORM           0x12 /* (1 xform) */
#define GP1_VIDEO_OP_SRCIMAGE        0x13 /* (4 imageid) */
#define GP1_VIDEO_OP_DSTIMAGE        0x14 /* (4 imageid) */
#define GP1_VIDEO_OP_CLEAR           0x20 /* () */
#define GP1_VIDEO_OP_COPY            0x21 /* (2 dstx,2 dsty) */
#define GP1_VIDEO_OP_COPYSUB         0x22 /* (2 dstx,2 dsty,2 srcx,2 srcy,2 w,2 h) */
#define GP1_VIDEO_OP_TILE            0x23 /* (2 dstx,2 dsty,1 tileid) */

#define GP1_XFORM_NONE 0
#define GP1_XFORM_XREV 1
#define GP1_XFORM_YREV 2
#define GP1_XFORM_SWAP 4
 
/* Send a batch of encoded video commands.
 * Each batch runs against a clean state, and the same framebuffer content left over from the last batch.
 * See gp1_video_batch.h for a friendlier interface.
 */
void gp1_video_send(const void *src,int32_t srcc);

/* Audio.
 *****************************************************************/

/* Send a batch of encoded audio commands.
 * Audio happens in real time. You don't know how much will elapse between two sends, but each send is atomic.
 * NB These are commands, not raw PCM. That would be unwieldly.
 * Helpers will be provided to assemble batches. TODO
 */
void gp1_audio_send(const void *src,int32_t srcc);

/* Files.
 *****************************************************************/

/* Read or write a permanent file on the host.
 * You must declare these files in the ROM header.
 * You may only read/write entire files at once.
 */
int32_t gp1_store(uint32_t k,const void *v,int32_t c);
int32_t gp1_load(void *v,int32_t a,uint32_t k);

/* Fetch a "teXT" resource.
 * The ROM can contain multiple languages; runtime selects one appropriate to the user.
 */
int32_t gp1_get_string(char *dst,int32_t dsta,uint32_t id);

/* "daTA" resources, same idea as "teXT" but no language.
 */
int32_t gp1_get_data(void *dst,int32_t dsta,uint32_t id);

/* Network.
 ********************************************************************/

/* Begin an HTTP request.
 * If this returns >=0, the VM will eventually call gp1_http_response() with the same (userdata) and the response body.
 * Clients are not allowed to set or read headers.
 */
int32_t gp1_http_request(
  const char *method,
  const char *url,
  const void *body,int32_t bodyc,
  void *userdata
);

/* Request a WebSocket connection.
 * Returns <0 for errors or 0 if starting.
 * VM will call gp1_ws_connect_complete() with this (userdata) when it succeeds or fails.
 * That may be during this call or after.
 * Note that the return value here is *not* (wsid). You don't get that until the connection succeeds.
 */
int32_t gp1_ws_connect(
  const char *url,
  void *userdata
);

/* Send data to an established WebSocket connection.
 */
int32_t gp1_ws_send(
  int32_t wsid,
  const void *src,int srcc
);

void gp1_ws_disconnect(int32_t wsid);

//TODO other things to expose to clients.... clock?

/* libc functions explicitly provided by gp1.
 *********************************************************************/
 
#if 0 // Use your compiler's libc headers; this section is documentation only.

// stdlib.h
int rand();
void *malloc(size_t len);
void *calloc(size_t c,size_t size);
void *realloc(void *p,size_t len);
void free(void *p);

#endif
 
#endif
