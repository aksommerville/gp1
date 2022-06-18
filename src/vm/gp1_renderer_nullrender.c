#include "gp1_vm_render.h"
#include "gp1/gp1.h"
#include <string.h>
#include <stdio.h>

struct gp1_renderer_nullrender {
  struct gp1_renderer hdr;
  // If (parse_input==0), we might miss DECLARE_COMMAND ops, which would be a problem if you turn parsing back on later.
  // So, it should be ON or OFF for the entire duration, don't change midflight.
  int parse_input; // Nonzero to read the requests and validate framing, etc.
  int log_input; // Nonzero to log every command to stderr -- beware, this is an enormous amount of logging
  int lenv[256];
};

#define RENDERER ((struct gp1_renderer_nullrender*)renderer)

/* Init.
 */
 
static int _gp1_nullrender_init(struct gp1_renderer *renderer) {
  RENDERER->parse_input=1;
  RENDERER->log_input=0;
  memset(RENDERER->lenv,-1,sizeof(RENDERER->lenv));
  return 0;
}

/* Load image.
 */
 
static int _gp1_nullrender_load_image(
  struct gp1_renderer *renderer,
  int imageid,int w,int h,int stride,int format,
  const void *src,int srcc
) {
  fprintf(stderr,"%s imageid=%d size=(%d,%d) format=%d total=%d\n",__func__,imageid,w,h,format,srcc);
  return 0;
}

/* Render.
 */
 
#define LOG(fmt,...) { if (RENDERER->log_input) fprintf(stderr,fmt"\n",##__VA_ARGS__); }
 
static int _gp1_nullrender_render(
  struct gp1_renderer *renderer,
  const void *src,int srcc
) {
  if (!RENDERER->parse_input) return 0;
  //...or validate commands...
  LOG("%s srcc=%d...",__func__,srcc)
  const uint8_t *SRC=src;
  int srcp=0;
  while (srcp<srcc) {
    uint8_t opcode=SRC[srcp++];
    #define REQUIRE(c) if (srcp>srcc-(c)) { \
      fprintf(stderr,"%s:ERROR: Input overflow at %d/%d, expecting %d\n",__func__,srcp,srcc,c); \
      return -1; \
    }
    switch (opcode) {
      case GP1_VIDEO_OP_EOF: LOG("EOF") return 0;
      case GP1_VIDEO_OP_DECLARE_COMMAND: {
          REQUIRE(3)
          uint8_t xop=SRC[srcp];
          uint16_t len=(SRC[srcp+1]<<8)|SRC[srcp+2];
          LOG("DECLARE_COMMAND %02x = %d",xop,len)
          RENDERER->lenv[xop]=len;
          srcp+=3;
        } break;
      case GP1_VIDEO_OP_FGCOLOR: {
          REQUIRE(4)
          LOG("FGCOLOR %02x%02x%02x%02x",SRC[srcp],SRC[srcp+1],SRC[srcp+2],SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_BGCOLOR: {
          REQUIRE(4)
          LOG("BGCOLOR %02x%02x%02x%02x",SRC[srcp],SRC[srcp+1],SRC[srcp+2],SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_HIGHLIGHT: {
          REQUIRE(4)
          LOG("HIGHLIGHT %02x%02x%02x%02x",SRC[srcp],SRC[srcp+1],SRC[srcp+2],SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_TINT: {
          REQUIRE(4)
          LOG("TINT %02x%02x%02x%02x",SRC[srcp],SRC[srcp+1],SRC[srcp+2],SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_XFORM: {
          REQUIRE(1)
          LOG("XFORM %02x",SRC[srcp])
          srcp+=1;
        } break;
      case GP1_VIDEO_OP_SCALE: {
          REQUIRE(4)
          LOG("SCALE %02x%02x.%02x.%02x",SRC[srcp],SRC[srcp+1],SRC[srcp+2],SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_ROTATE: {
          REQUIRE(1)
          LOG("ROTATE %02x",SRC[srcp])
          srcp+=1;
        } break;
      case GP1_VIDEO_OP_SRCIMAGE: {
          REQUIRE(4)
          LOG("SRCIMAGE %d",(SRC[srcp]<<24)|(SRC[srcp+1]<<16)|(SRC[srcp+2]<<8)|SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_DSTIMAGE: {
          REQUIRE(4)
          LOG("DSTIMAGE %d",(SRC[srcp]<<24)|(SRC[srcp+1]<<16)|(SRC[srcp+2]<<8)|SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_CLEAR: {
          LOG("CLEAR")
        } break;
      case GP1_VIDEO_OP_COPY: {
          REQUIRE(4)
          LOG("COPY (%d,%d)",(SRC[srcp]<<8)|SRC[srcp+1],(SRC[srcp+2]<<8)|SRC[srcp+3])
          srcp+=4;
        } break;
      case GP1_VIDEO_OP_COPYSUB: {
          REQUIRE(12)
          LOG(
            "COPYSUB (%d,%d),(%d,%d),(%d,%d)",
            (SRC[srcp]<<8)|SRC[srcp+1],(SRC[srcp+2]<<8)|SRC[srcp+3],
            (SRC[srcp+4]<<8)|SRC[srcp+5],(SRC[srcp+6]<<8)|SRC[srcp+7],
            (SRC[srcp+8]<<8)|SRC[srcp+9],(SRC[srcp+10]<<8)|SRC[srcp+11]
          )
          srcp+=12;
        } break;
      case GP1_VIDEO_OP_TILE: {
          REQUIRE(5)
          LOG("TILE (%d,%d) %02x",(SRC[srcp]<<8)|SRC[srcp+1],(SRC[srcp+2]<<8)|SRC[srcp+3],SRC[srcp+4])
          srcp+=5;
        } break;
      default: {
          if (RENDERER->lenv[opcode]<0) {
            fprintf(stderr,"Unknown video opcode 0x%02x at %d/%d\n",opcode,srcp-1,srcc);
            return -1;
          } else {
            srcp+=RENDERER->lenv[opcode];
          }
        }
    }
    #undef REQUIRE
  }
  return 0;
}

#undef LOG

/* Type definition.
 */
 
const struct gp1_renderer_type gp1_renderer_type_nullrender={
  .name="nullrender",
  .desc="Discards all input, for headless automation only.",
  .objlen=sizeof(struct gp1_renderer_nullrender),
  .init=_gp1_nullrender_init,
  .load_image=_gp1_nullrender_load_image,
  .render=_gp1_nullrender_render,
};
