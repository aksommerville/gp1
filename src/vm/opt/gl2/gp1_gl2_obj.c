#include "gp1_gl2_internal.h"

/* Cleanup.
 */
 
static void _gp1_gl2_del(struct gp1_renderer *renderer) {
  if (RENDERER->imagev) {
    while (RENDERER->imagec-->0) gp1_gl2_image_cleanup(RENDERER->imagev+RENDERER->imagec);
    free(RENDERER->imagev);
  }
  
  gp1_gl2_program_del(RENDERER->pgm_decal);
  gp1_gl2_program_del(RENDERER->pgm_tile);
}

/* Init.
 */
 
static int _gp1_gl2_init(struct gp1_renderer *renderer) {
  GP1_RENDERER_INTAKE_INIT
  
  RENDERER->srcimageid=-1;
  RENDERER->dstimageid=-1;
  
  if (
    !(RENDERER->pgm_decal=gp1_gl2_program_decal_new(renderer))||
    !(RENDERER->pgm_tile=gp1_gl2_program_tile_new(renderer))||
  0) return -1;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  
  return 0;
}

/* ROM changed: Drop image cache.
 */
 
static int _gp1_gl2_rom_changed(struct gp1_renderer *renderer) {

  // Clear all images.
  RENDERER->srcimageid=-1;
  RENDERER->srcimage=0;
  RENDERER->dstimageid=-1;
  RENDERER->dstimage=0;
  while (RENDERER->imagec>0) {
    RENDERER->imagec--;
    gp1_gl2_image_cleanup(RENDERER->imagev+RENDERER->imagec);
  }
  RENDERER->imagecontigc=0;
  
  // If a ROM is present, rebuild image zero.
  if (renderer->rom) {
    struct gp1_gl2_image *image=gp1_gl2_image_get(renderer,0);
    if (!image) {
      fprintf(stderr,"gl2: Failed to generate image zero for %dx%d framebuffer.\n",renderer->rom->fbw,renderer->rom->fbh);
      return -1;
    }
  }
  
  return 0;
}

/* Report illegal instruction.
 */
 
static void _gp1_renderer_illegal(struct gp1_renderer *renderer,uint8_t opcode,int srcp,int srcc) {
  fprintf(stderr,"gl2: Illegal video instruction 0x%02x at %d/%d\n",opcode,srcp,srcc);
}

/* Clear framebuffer.
 */
 
static void _gp1_gl2_clear(struct gp1_renderer *renderer) {
  gp1_gl2_require_images(renderer);
  glClearColor(
    (RENDERER->state.bgcolor>>24)/255.0f,
    ((RENDERER->state.bgcolor>>16)&0xff)/255.0f,
    ((RENDERER->state.bgcolor>>8)&0xff)/255.0f,
    (RENDERER->state.bgcolor&0xff)/255.0f
  );
  glClear(GL_COLOR_BUFFER_BIT);
}

/* Copy entire image.
 */
 
static void _gp1_gl2_copy(struct gp1_renderer *renderer,int16_t dstx,int16_t dsty) {
  gp1_gl2_require_images(renderer);
  if (!RENDERER->srcimage||!RENDERER->srcimage->texid) return;
  if (!RENDERER->dstimage||!RENDERER->dstimage->texid) return;
  gp1_gl2_program_decal_render(
    RENDERER->pgm_decal,
    dstx,dsty,-1,-1,
    0,0,-1,-1
  );
}

/* Copy portion of image.
 */
 
static void _gp1_gl2_copysub(struct gp1_renderer *renderer,int16_t dstx,int16_t dsty,int16_t srcx,int16_t srcy,int16_t w,int16_t h) {
  gp1_gl2_require_images(renderer);
  if (!RENDERER->srcimage||!RENDERER->srcimage->texid) return;
  if (!RENDERER->dstimage||!RENDERER->dstimage->texid) return;
  gp1_gl2_program_decal_render(
    RENDERER->pgm_decal,
    dstx,dsty,w,h,
    srcx,srcy,w,h
  );
}

/* Copy tiles.
 */
 
static void _gp1_gl2_tiles(struct gp1_renderer *renderer,const struct gp1_render_tile *vtxv,int vtxc) {
  gp1_gl2_require_images(renderer);
  gp1_gl2_program_tiles_render(RENDERER->pgm_tile,vtxv,vtxc);
}

/* Update (dstx,dsty,dstw,dsth) if needed.
 */
 
static void gp1_gl2_require_output_bounds(struct gp1_renderer *renderer) {
  if ((RENDERER->pvmainw==renderer->mainw)&&(RENDERER->pvmainh==renderer->mainh)) return;
  struct gp1_gl2_image *fb=gp1_gl2_image_get(renderer,0);
  if (!fb) return;
  if ((renderer->mainw<1)||(renderer->mainh<1)||(fb->w<1)||(fb->h<1)) return;
  
  //TODO There's plenty of fuzzy logic we could apply here, eg clamp to integer scale at small size, fuzz to edges when close, etc
  // For now, keeping it simple.
  
  int wforh=(fb->w*renderer->mainh)/fb->h;
  if (wforh<=renderer->mainw) {
    RENDERER->dstw=wforh;
    RENDERER->dsth=renderer->mainh;
  } else {
    RENDERER->dstw=renderer->mainw;
    RENDERER->dsth=(fb->h*renderer->mainw)/fb->w;
  }
  
  RENDERER->dstx=(renderer->mainw>>1)-(RENDERER->dstw>>1);
  RENDERER->dsty=(renderer->mainh>>1)-(RENDERER->dsth>>1);
  
  RENDERER->pvmainw=renderer->mainw;
  RENDERER->pvmainh=renderer->mainh;
}

/* End frame.
 */
 
static int _gp1_gl2_end_frame(struct gp1_renderer *renderer) {
  //TODO detect whether image zero changed. if not, we can no-op
  gp1_gl2_image_bind_main(renderer);
  gp1_gl2_require_output_bounds(renderer);
  
  // Letterbox or pillarbox if we're not covering the output.
  if ((RENDERER->dstw<renderer->mainw)||(RENDERER->dsth<renderer->mainh)) {
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  
  gp1_gl2_program_decal_render(
    RENDERER->pgm_decal,
    RENDERER->dstx,RENDERER->dsty,RENDERER->dstw,RENDERER->dsth,
    0,0,-1,-1
  );
  
  return 0;
}

/* Type definition.
 */
 
GP1_RENDERER_INTAKE_RENDER(gl2)
 
const struct gp1_renderer_type gp1_renderer_type_gl2={
  .name="gl2",
  .desc="OpenGL version 2, widely supported accelerated graphics.",
  .objlen=sizeof(struct gp1_renderer_gl2),
  .del=_gp1_gl2_del,
  .init=_gp1_gl2_init,
  .rom_changed=_gp1_gl2_rom_changed,
  .render=_gp1_gl2_render,
  .end_frame=_gp1_gl2_end_frame,
};
