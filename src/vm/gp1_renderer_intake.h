/* gp1_renderer_intake.h
 * Utility for renderers that provides a uniform implementation of the 'render' hook.
 *
 * In your renderer instance, add:
 *   GP1_RENDERER_INTAKE_INSTANCE
 *
 * In your 'init' hook, add:
 *   GP1_RENDERER_INTAKE_INIT
 *
 * Before your type definition, at global scope:
 *   GP1_RENDERER_INTAKE_RENDER(my_renderer_name)
 *
 * And finally in the type definition, no surprises:
 *   .render=_gp1_my_renderer_name_render,
 *
 * Then try to compile it, and implement everything that comes back undefined. (search below for details)
 */
 
#ifndef GP1_RENDERER_INTAKE_H
#define GP1_RENDERER_INTAKE_H

struct gp1_render_state {
  uint32_t fgcolor;
  uint32_t bgcolor;
  uint8_t xform;
  uint32_t srcimageid;
  uint32_t dstimageid;
};

struct gp1_render_tile {
  int16_t dstx;
  int16_t dsty;
  uint8_t tileid;
  /* Per API, xform is a separate uniform attribute.
   * I think renderers are going to make it a vertex attribute, so we're combining here.
   * (at least, because that's how gl2 prefers it, and that's the only renderer I've written so far).
   */
  uint8_t xform;
};

#define GP1_RENDERER_INTAKE_TILE_LIMIT 256

#define GP1_RENDERER_INTAKE_INSTANCE \
  struct gp1_render_state state; \
  int xoplenv[256]; \
  struct gp1_render_tile tilev[GP1_RENDERER_INTAKE_TILE_LIMIT]; \
  
#define GP1_RENDERER_INTAKE_INIT \
  memset(RENDERER->xoplenv,-1,sizeof(RENDERER->xoplenv));

#define _GP1_RENDERER_INTAKE_REQUIRE(c) { \
  if (srcp>srcc-(c)) { \
    _gp1_renderer_illegal(renderer,opcode,srcp,srcc); \
    return -1; \
  } \
}

#define _GP1_RENDERER_STATE32(name) { \
  _GP1_RENDERER_INTAKE_REQUIRE(4) \
  RENDERER->state.name=(SRC[srcp]<<24)|(SRC[srcp+1]<<16)|(SRC[srcp+2]<<8)|SRC[srcp+3]; \
  srcp+=4; \
}

#define _GP1_RENDERER_STATE8(name) { \
  _GP1_RENDERER_INTAKE_REQUIRE(1) \
  RENDERER->state.name=SRC[srcp]; \
  srcp+=1; \
}

#define GP1_RENDERER_INTAKE_RENDER(tag) \
static int _gp1_##tag##_render(struct gp1_renderer *renderer,const void *src,int srcc) { \
\
  RENDERER->state.fgcolor=0xffffffff; \
  RENDERER->state.bgcolor=0x00000000; \
  RENDERER->state.xform=0; \
  RENDERER->state.srcimageid=0; \
  RENDERER->state.dstimageid=0; \
\
  const uint8_t *SRC=src; \
  int srcp=0; \
  while (srcp<srcc) { \
    uint8_t opcode=SRC[srcp++]; \
    switch (opcode) { \
\
      case GP1_VIDEO_OP_EOF: return 0; \
\
      case GP1_VIDEO_OP_DECLARE_COMMAND: { \
          _GP1_RENDERER_INTAKE_REQUIRE(3) \
          uint8_t xop=SRC[srcp++]; \
          uint16_t len=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          RENDERER->xoplenv[opcode]=len; \
        } break; \
\
      case GP1_VIDEO_OP_FGCOLOR: _GP1_RENDERER_STATE32(fgcolor) break; \
      case GP1_VIDEO_OP_BGCOLOR: _GP1_RENDERER_STATE32(bgcolor) break; \
      case GP1_VIDEO_OP_XFORM: _GP1_RENDERER_STATE8(xform) break; \
      case GP1_VIDEO_OP_SRCIMAGE: _GP1_RENDERER_STATE32(srcimageid) break; \
      case GP1_VIDEO_OP_DSTIMAGE: _GP1_RENDERER_STATE32(dstimageid) break; \
\
      case GP1_VIDEO_OP_CLEAR: _gp1_##tag##_clear(renderer); break; \
\
      case GP1_VIDEO_OP_COPY: { \
          _GP1_RENDERER_INTAKE_REQUIRE(4); \
          int16_t x=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t y=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          _gp1_##tag##_copy(renderer,x,y); \
        } break; \
\
      case GP1_VIDEO_OP_COPYSUB: { \
          _GP1_RENDERER_INTAKE_REQUIRE(12); \
          int16_t dstx=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t dsty=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t srcx=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t srcy=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t w=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          int16_t h=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
          _gp1_##tag##_copysub(renderer,dstx,dsty,srcx,srcy,w,h); \
        } break; \
\
      case GP1_VIDEO_OP_TILE: { \
          int tilec=0; \
          while (1) { \
            _GP1_RENDERER_INTAKE_REQUIRE(5) \
            int16_t x=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
            int16_t y=(SRC[srcp]<<8)|SRC[srcp+1]; srcp+=2; \
            uint8_t tileid=SRC[srcp]; srcp+=1; \
            RENDERER->tilev[tilec++]=(struct gp1_render_tile){.dstx=x,.dsty=y,.tileid=tileid,.xform=RENDERER->state.xform}; \
            if (tilec>=GP1_RENDERER_INTAKE_TILE_LIMIT) break; \
            while ((srcp<srcc)&&(SRC[srcp]==GP1_VIDEO_OP_XFORM)) { \
              srcp++; \
              _GP1_RENDERER_INTAKE_REQUIRE(1) \
              RENDERER->state.xform=SRC[srcp++]; \
            } \
            if (srcp>=srcc) break; \
            if (SRC[srcp]!=GP1_VIDEO_OP_TILE) break; \
            srcp++; \
          } \
          _gp1_##tag##_tiles(renderer,RENDERER->tilev,tilec); \
        } break; \
\
      default: { \
          if (RENDERER->xoplenv[opcode]>=0) { \
            _GP1_RENDERER_INTAKE_REQUIRE(RENDERER->xoplenv[opcode]) \
            srcp+=RENDERER->xoplenv[opcode]; \
          } else { \
            _gp1_renderer_illegal(renderer,opcode,srcp,srcc); \
            return -1; \
          } \
        } \
    } \
  } \
  return 0; \
}

#endif
