/* gp1_video_batch.h
 * Helper for assembling video command batches.
 */
 
#ifndef GP1_VIDEO_BATCH_H
#define GP1_VIDEO_BATCH_H

struct gp1_video_batch {
  int32_t c,a;
  int32_t stopp;
  uint8_t *v;
};

#define GP1_LONGEST_VIDEO_COMMAND 13

/* (size) is somewhat arbitrary. Must be at least GP1_LONGEST_VIDEO_COMMAND. Aim high.
 * Ideally it's large enough to contain an entire frame of video.
 */
#define GP1_VIDEO_BATCH_DECLARE(name,size) \
  static uint8_t _gp1_video_batch_storage_##name[size]; \
  static struct gp1_video_batch name={.a=size,.stopp=(size)-GP1_LONGEST_VIDEO_COMMAND,.v=_gp1_video_batch_storage_##name};
  
/* Manually drop queued commands or send them.
 * In normal use you shouldn't need these. (you get an implicit flush at "eof").
 */
  
static inline void gp1_video_drop(struct gp1_video_batch *batch) {
  batch->c=0;
}

static inline void gp1_video_flush(struct gp1_video_batch *batch) {
  if (batch->c) {
    gp1_video_send(batch->v,batch->c);
    batch->c=0;
  }
}

/* All other functions correspond directly to one opcode.
 */
  
#define GP1_VIDEO_BATCH_REQUIRE(addc) \
  if (batch->c>batch->stopp) { \
    gp1_video_send(batch->v,batch->c); \
    batch->c=0; \
  }

// Caller must assert length first. 
#define GP1_VIDEO_APPEND8(n) batch->v[batch->c++]=n;
  
static inline void gp1_video_eof(struct gp1_video_batch *batch) {
  GP1_VIDEO_BATCH_REQUIRE(1)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_EOF)
  gp1_video_flush(batch);
}

static inline void gp1_video_declare_command(struct gp1_video_batch *batch,uint8_t opcode,uint16_t len) {
  GP1_VIDEO_BATCH_REQUIRE(4)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_DECLARE_COMMAND)
  GP1_VIDEO_APPEND8(opcode)
  GP1_VIDEO_APPEND8(len>>8)
  GP1_VIDEO_APPEND8(len)
}

#define GP1_COLOR_COMMAND(tag,opcode) \
  static inline void gp1_video_##tag(struct gp1_video_batch *batch,uint32_t rgba) { \
    GP1_VIDEO_BATCH_REQUIRE(5) \
    GP1_VIDEO_APPEND8(GP1_VIDEO_OP_##opcode) \
    GP1_VIDEO_APPEND8(rgba>>24) \
    GP1_VIDEO_APPEND8(rgba>>16) \
    GP1_VIDEO_APPEND8(rgba>>8) \
    GP1_VIDEO_APPEND8(rgba) \
  } \
  static inline void gp1_video_##tag##_split(struct gp1_video_batch *batch,uint8_t r,uint8_t g,uint8_t b,uint8_t a) { \
    GP1_VIDEO_BATCH_REQUIRE(5) \
    GP1_VIDEO_APPEND8(GP1_VIDEO_OP_##opcode) \
    GP1_VIDEO_APPEND8(r) \
    GP1_VIDEO_APPEND8(g) \
    GP1_VIDEO_APPEND8(b) \
    GP1_VIDEO_APPEND8(a) \
  }
  
GP1_COLOR_COMMAND(fgcolor,FGCOLOR)
GP1_COLOR_COMMAND(bgcolor,BGCOLOR)

static inline void gp1_video_xform(struct gp1_video_batch *batch,uint8_t xform) {
  GP1_VIDEO_BATCH_REQUIRE(2)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_XFORM)
  GP1_VIDEO_APPEND8(xform)
}

static inline void gp1_video_srcimage(struct gp1_video_batch *batch,uint32_t imageid) {
  GP1_VIDEO_BATCH_REQUIRE(5)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_SRCIMAGE)
  GP1_VIDEO_APPEND8(imageid>>24)
  GP1_VIDEO_APPEND8(imageid>>16)
  GP1_VIDEO_APPEND8(imageid>>8)
  GP1_VIDEO_APPEND8(imageid)
}

static inline void gp1_video_dstimage(struct gp1_video_batch *batch,uint32_t imageid) {
  GP1_VIDEO_BATCH_REQUIRE(5)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_DSTIMAGE)
  GP1_VIDEO_APPEND8(imageid>>24)
  GP1_VIDEO_APPEND8(imageid>>16)
  GP1_VIDEO_APPEND8(imageid>>8)
  GP1_VIDEO_APPEND8(imageid)
}

static inline void gp1_video_clear(struct gp1_video_batch *batch) {
  GP1_VIDEO_BATCH_REQUIRE(1)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_CLEAR)
}

static inline void gp1_video_copy(
  struct gp1_video_batch *batch,
  int16_t dstx,int16_t dsty
) {
  GP1_VIDEO_BATCH_REQUIRE(5)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_COPY)
  GP1_VIDEO_APPEND8(dstx>>8)
  GP1_VIDEO_APPEND8(dstx)
  GP1_VIDEO_APPEND8(dsty>>8)
  GP1_VIDEO_APPEND8(dsty)
}

static inline void gp1_video_copysub(
  struct gp1_video_batch *batch,
  int16_t dstx,int16_t dsty,
  int16_t srcx,int16_t srcy,
  int16_t w,int16_t h
) {
  GP1_VIDEO_BATCH_REQUIRE(13)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_COPYSUB)
  GP1_VIDEO_APPEND8(dstx>>8)
  GP1_VIDEO_APPEND8(dstx)
  GP1_VIDEO_APPEND8(dsty>>8)
  GP1_VIDEO_APPEND8(dsty)
  GP1_VIDEO_APPEND8(srcx>>8)
  GP1_VIDEO_APPEND8(srcx)
  GP1_VIDEO_APPEND8(srcy>>8)
  GP1_VIDEO_APPEND8(srcy)
  GP1_VIDEO_APPEND8(w>>8)
  GP1_VIDEO_APPEND8(w)
  GP1_VIDEO_APPEND8(h>>8)
  GP1_VIDEO_APPEND8(h)
}

static inline void gp1_video_tile(
  struct gp1_video_batch *batch,
  int16_t dstx,int16_t dsty,uint8_t tileid
) {
  GP1_VIDEO_BATCH_REQUIRE(6)
  GP1_VIDEO_APPEND8(GP1_VIDEO_OP_TILE)
  GP1_VIDEO_APPEND8(dstx>>8)
  GP1_VIDEO_APPEND8(dstx)
  GP1_VIDEO_APPEND8(dsty>>8)
  GP1_VIDEO_APPEND8(dsty)
  GP1_VIDEO_APPEND8(tileid)
}

#endif
