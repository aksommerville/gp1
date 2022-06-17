#include "gp1/gp1.h"

static uint16_t pvinput=0;
static int32_t wsid=0;
static int ws_connect_pending=0;

int32_t gp1_init() {
  gp1_store(123,"hello",5);
  
  char tmp[32]={1,2,3};
  int tmpc=gp1_get_string(tmp,sizeof(tmp),1);
  if ((tmpc<0)||(tmpc>32)) tmpc=0;
  gp1_store(1,tmp,tmpc);
  
  gp1_http_request("POTS","http://inter.nets/abc?xyz=123","{}",2,&pvinput);
  
  return 0;
}

void gp1_http_response(
  int32_t status,
  const void *body,int bodyc,
  void *userdata
) {
  //TODO
}

void gp1_ws_connect_complete(
  void *userdata,
  int32_t wsid_incoming
) {
  if (ws_connect_pending) {
    wsid=wsid_incoming;
    ws_connect_pending=0;
  } else {
    gp1_ws_disconnect(wsid_incoming);
  }
}

void gp1_ws_receive(
  void *userdata,
  int32_t wsid,
  const void *src,int srcc
) {
  //TODO
}

void gp1_ws_disconnected(
  void *userdata,
  int32_t wsid_incoming
) {
  wsid=0;
}

static void render_frame() {
  char buf[1024];
  int bufc=0;
  //TODO populate (buf) with rendering commands
  buf[bufc++]=11;
  buf[bufc++]=22;
  buf[bufc++]=33;
  gp1_video_send(buf,bufc);
}

static void play_sound() {
  uint8_t buf[]={
    //TODO audio commands
  };
  gp1_audio_send(buf,sizeof(buf));
}

static void store_something() {
  gp1_store(1,"hello",5);
}

static void load_something() {
  char tmp[32];
  int tmpc=gp1_load(tmp,sizeof(tmp),1);
}

static void load_string() {
  char tmp[32];
  int tmpc=gp1_get_string(tmp,sizeof(tmp),1);
}

static void request_http() {
  gp1_http_request("GET","http://localhost:12345/abc/def","Hello this is an HTTP request.",30,0);
}

static void send_websocket() {
  if (ws_connect_pending) return;
  if (wsid) {
    gp1_ws_send(wsid,"This is sent from wasm app.",27);
  } else {
    if (gp1_ws_connect("ws://localhost:12345/ws",0)>=0) {
      ws_connect_pending=1;
    }
  }
}

static void drop_websocket() {
  ws_connect_pending=0;
  if (!wsid) return;
  gp1_ws_disconnect(wsid);
  wsid=0;
}

int32_t gp1_update() {

  uint16_t input=gp1_get_input_state(GP1_PLAYER_AGGREGATE);
  if (input!=pvinput) {
    #define BTNDOWN(tag) ((input&GP1_BTNID_##tag)&&!(pvinput&GP1_BTNID_##tag))
    if (BTNDOWN(SOUTH)) play_sound();
    if (BTNDOWN(WEST)) store_something();
    if (BTNDOWN(EAST)) load_something();
    if (BTNDOWN(NORTH)) load_string();
    if (BTNDOWN(L1)) request_http();
    if (BTNDOWN(R1)) send_websocket();
    if (BTNDOWN(R2)) drop_websocket();
    #undef BTNDOWN
    pvinput=input;
  }
  
  render_frame();
  
  return 0;
}
