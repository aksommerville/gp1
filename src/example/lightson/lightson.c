#include "gp1/gp1.h"
#include "gp1/gp1_video_batch.h"
#include <stdlib.h>

//TODO confirm implemented: malloc,calloc,realloc,free

// Must agree with lightson.meta:
#define FBW 640
#define FBH 360

// Must agree with 002-tiles.png:
#define TILESIZE 16
#define BIGTILESIZE 32 /* 003-big-tiles.png */

GP1_VIDEO_BATCH_DECLARE(video,8192*16)
static uint16_t pvinput=0;
static int32_t wsid=0;
static int ws_connect_pending=0;
static uint32_t framec=0;

#define SPRITEC 100
static struct sprite {
  int16_t x,y; // top left corner (NB not center, as the renderer wants)
  int16_t dx,dy;
  uint8_t tileid;
  uint8_t xform;
} spritev[SPRITEC];
static struct sprite bigspritev[SPRITEC];

int32_t gp1_init() {
  gp1_store(123,"hello",5);
  
  char tmp[32]={1,2,3};
  int tmpc=gp1_get_string(tmp,sizeof(tmp),1);
  if ((tmpc<0)||(tmpc>32)) tmpc=0;
  gp1_store(1,tmp,tmpc);
  
  gp1_http_request("POTS","http://inter.nets/abc?xyz=123","{}",2,&pvinput);
  
  struct sprite *sprite=spritev;
  struct sprite *bigsprite=bigspritev;
  int i=SPRITEC;
  for (;i-->0;sprite++,bigsprite++) {
    sprite->x=rand()%(FBW-TILESIZE);
    sprite->y=rand()%(FBH-TILESIZE);
    sprite->dx=rand()%5-2;
    sprite->dy=rand()%5-2;
    switch (rand()%5) {
      case 0: sprite->tileid=0x00; break;
      case 1: sprite->tileid=0x01; break;
      case 2: sprite->tileid=0x02; break;
      case 3: sprite->tileid=0x03; break;
      case 4: sprite->tileid=0xff; break;
    }
    
    bigsprite->x=rand()%(FBW-BIGTILESIZE);
    bigsprite->y=rand()%(FBH-BIGTILESIZE);
    bigsprite->dx=rand()%5-2;
    bigsprite->dy=rand()%5-2;
    switch (rand()%3) {
      case 0: bigsprite->tileid=0x00; break;
      case 1: bigsprite->tileid=0x01; break;
      case 2: bigsprite->tileid=0x02; break;
    }
  }
  
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

// Renders three rows of icons that should be identical
static void render_xform_exposition(int16_t x,int16_t y) {

  // We have an 8-bit image explaining this; draw it to the right.
  // With oscillating alpha, for no particular reason.
  {
    int a=(framec&0x1ff);
    if (a>=0x100) a=0x1ff-a;
    gp1_video_srcimage(&video,8);
    gp1_video_fgcolor(&video,0xffffff00|a);
    gp1_video_copy(&video,x+(TILESIZE+1)*8,y);
  }

  x+=1+(TILESIZE>>1);
  y+=1+(TILESIZE>>1);
  int16_t stride=TILESIZE+1;

  // Top row are pre-transformed tiles for reference.
  gp1_video_srcimage(&video,2);
  gp1_video_tile(&video,x+stride*0,y,0x10);
  gp1_video_tile(&video,x+stride*1,y,0x11);
  gp1_video_tile(&video,x+stride*2,y,0x12);
  gp1_video_tile(&video,x+stride*3,y,0x13);
  gp1_video_tile(&video,x+stride*4,y,0x14);
  gp1_video_tile(&video,x+stride*5,y,0x15);
  gp1_video_tile(&video,x+stride*6,y,0x16);
  gp1_video_tile(&video,x+stride*7,y,0x17);
  y+=stride;
  
  // Second row are the first tile, with a transform on each.
  gp1_video_xform(&video,0); // NONE
  gp1_video_tile(&video,x+stride*0,y,0x10);
  gp1_video_xform(&video,1); // XREV
  gp1_video_tile(&video,x+stride*1,y,0x10);
  gp1_video_xform(&video,2); // YREV
  gp1_video_tile(&video,x+stride*2,y,0x10);
  gp1_video_xform(&video,3); // XREV|YREV
  gp1_video_tile(&video,x+stride*3,y,0x10);
  gp1_video_xform(&video,4); // SWAP
  gp1_video_tile(&video,x+stride*4,y,0x10);
  gp1_video_xform(&video,5); // SWAP|XREV
  gp1_video_tile(&video,x+stride*5,y,0x10);
  gp1_video_xform(&video,6); // SWAP|YREV
  gp1_video_tile(&video,x+stride*6,y,0x10);
  gp1_video_xform(&video,7); // SWAP|XREV|YREV
  gp1_video_tile(&video,x+stride*7,y,0x10);
  gp1_video_xform(&video,0);
  y+=stride;
  
  // Third row, do it again but with "copy" instead.
  // Note that copy wants the top-left corner, where tile wants the center.
  x-=TILESIZE>>1;
  y-=TILESIZE>>1;
  gp1_video_srcimage(&video,7);
  gp1_video_xform(&video,0); // NONE
  gp1_video_copy(&video,x+stride*0,y);
  gp1_video_xform(&video,1); // XREV
  gp1_video_copy(&video,x+stride*1,y);
  gp1_video_xform(&video,2); // YREV
  gp1_video_copy(&video,x+stride*2,y);
  gp1_video_xform(&video,3); // XREV|YREV
  gp1_video_copy(&video,x+stride*3,y);
  gp1_video_xform(&video,4); // SWAP
  gp1_video_copy(&video,x+stride*4,y);
  gp1_video_xform(&video,5); // SWAP|XREV
  gp1_video_copy(&video,x+stride*5,y);
  gp1_video_xform(&video,6); // SWAP|YREV
  gp1_video_copy(&video,x+stride*6,y);
  gp1_video_xform(&video,7); // SWAP|XREV|YREV
  gp1_video_copy(&video,x+stride*7,y);
  gp1_video_xform(&video,0);
}

static int bounce1bitp=0;
static int bounce1bittime=0;

static void render_1bit_tiles(int16_t x,int16_t y) {
  const char message[]="The quick brown fox jumps over the lazy dog.";
  int messagec=sizeof(message)-1;
  x+=3;
  y+=3;
  if (bounce1bittime--<0) {
    bounce1bittime=7;
    bounce1bitp++;
    if (bounce1bitp>=4) bounce1bitp=0;
  }
  
  gp1_video_srcimage(&video,9);
  int r=(framec*2)&0x1ff; if (r>0xff) r=0x1ff-r;
  int g=(framec*3)&0x1ff; if (g>0xff) g=0x1ff-g;
  int b=(framec*5)&0x1ff; if (b>0xff) b=0x1ff-b;
  gp1_video_bgcolor(&video,0x00000000);
  gp1_video_fgcolor(&video,(r<<24)|(g<<16)|(b<<8)|0xff);
  
  const char *p=message;
  int i=messagec;
  for (;i-->0;p++,x+=4) {
    gp1_video_tile(&video,x,y-(((i%4)==bounce1bitp)?1:0),*p);
  }
}

static void render_frame() {
  
  gp1_video_begin(&video);
  gp1_video_bgcolor(&video,0x604020ff); // brown
  gp1_video_clear(&video);
  
  render_xform_exposition(0,0);
  render_1bit_tiles(0,55);
  
  gp1_video_end(&video);
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

static void update_sprite(struct sprite *sprite) {
  
  sprite->x+=sprite->dx;
  if ((sprite->x<0)&&(sprite->dx<0)) sprite->dx=-sprite->dx;
  else if ((sprite->x>FBW-TILESIZE)&&(sprite->dx>0)) sprite->dx=-sprite->dx;
    
  sprite->y+=sprite->dy;
  if ((sprite->y<0)&&(sprite->dy<0)) sprite->dy=-sprite->dy;
  else if ((sprite->y>FBH-TILESIZE)&&(sprite->dy>0)) sprite->dy=-sprite->dy;
}

static void update_sprites() {
  struct sprite *sprite=spritev;
  int i=SPRITEC;
  for (;i-->0;sprite++) {
    update_sprite(sprite);
  }
  for (sprite=bigspritev,i=SPRITEC;i-->0;sprite++) {
    update_sprite(sprite);
  }
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
  
  update_sprites();
  
  render_frame();
  
  framec++;
  return 0;
}
