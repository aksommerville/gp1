#include "gp1_vm_render.h"
#include "gp1/gp1.h"
#include "gp1_renderer_intake.h"
#include <string.h>
#include <stdio.h>

struct gp1_renderer_nullrender {
  struct gp1_renderer hdr;
  GP1_RENDERER_INTAKE_INSTANCE
};

#define RENDERER ((struct gp1_renderer_nullrender*)renderer)

/* Init.
 */
 
static int _gp1_nullrender_init(struct gp1_renderer *renderer) {
  GP1_RENDERER_INTAKE_INIT
  return 0;
}

/* Report illegal instruction.
 */

static void _gp1_renderer_illegal(struct gp1_renderer *renderer,uint8_t opcode,int srcp,int srcc) {
  fprintf(stderr,"nullrender: Illegal opcode 0x%02x at %d/%d\n",opcode,srcp,srcc);
}

/* Render ops, ie do nothing.
 */
 
static void _gp1_nullrender_clear(struct gp1_renderer *renderer) {
}

static void _gp1_nullrender_copy(struct gp1_renderer *renderer,int16_t dstx,int16_t dsty) {
}

static void _gp1_nullrender_copysub(struct gp1_renderer *render,int16_t dstx,int16_t dsty,int16_t srcx,int16_t srcy,int16_t w,int16_t h) {
}

static void _gp1_nullrender_tiles(struct gp1_renderer *render,const struct gp1_render_tile *vtxv,int vtxc) {
}

/* Type definition.
 */

GP1_RENDERER_INTAKE_RENDER(nullrender)
 
const struct gp1_renderer_type gp1_renderer_type_nullrender={
  .name="nullrender",
  .desc="Discards all input, for headless automation only.",
  .objlen=sizeof(struct gp1_renderer_nullrender),
  .init=_gp1_nullrender_init,
  .render=_gp1_nullrender_render,
};
