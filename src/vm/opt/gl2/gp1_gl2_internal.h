#ifndef GP1_GL2_INTERNAL_H
#define GP1_GL2_INTERNAL_H

#include "gp1/gp1.h"
#include "vm/gp1_vm_render.h"
#include "vm/gp1_renderer_intake.h"
#include "vm/gp1_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//TODO I'm sure this is different on MacOS...
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

struct gp1_gl2_program; // forward

#define GP1_GL2_COLOR_MODE_NORMAL 0 /* use color from texture */
#define GP1_GL2_COLOR_MODE_1BIT   1 /* use texture to select fgcolor or bgcolor */
#define GP1_GL2_COLOR_MODE_8BIT   2 /* use fgcolor only, with texture as alpha */

struct gp1_renderer_gl2 {
  struct gp1_renderer hdr;
  
  int dstx,dsty,dstw,dsth; // in video pixels, where to draw the framebuffer
  int pvmainw,pvmainh; // matches (mainw,mainh) to detect whether to recalculate
  
  struct gp1_gl2_image {
    int imageid; // per gp1
    GLuint texid; // per gl; zero if unset
    GLuint fbid; // zero if read-only
    int w,h;
    int color_mode;
  } *imagev;
  int imagec,imagea;
  int imagecontigc;
  int srcimageid,dstimageid;
  struct gp1_gl2_image *srcimage,*dstimage; // WEAK
  
  struct gp1_gl2_program *pgm_decal;
  struct gp1_gl2_program *pgm_tile;
  
  GP1_RENDERER_INTAKE_INSTANCE
};

#define RENDERER ((struct gp1_renderer_gl2*)renderer)

void gp1_gl2_image_cleanup(struct gp1_gl2_image *image);
int gp1_gl2_image_search(const struct gp1_renderer *renderer,int imageid);
struct gp1_gl2_image *gp1_gl2_image_insert(struct gp1_renderer *renderer,int p,int imageid);
struct gp1_gl2_image *gp1_gl2_image_get(struct gp1_renderer *renderer,int imageid); // loads if necessary
void gp1_gl2_require_images(struct gp1_renderer *renderer); // Force (srcimage,dstimage) to agree with (state)
int gp1_gl2_image_require_input(struct gp1_renderer *renderer,struct gp1_gl2_image *image);
int gp1_gl2_image_bind_framebuffer(struct gp1_renderer *renderer,struct gp1_gl2_image *image);
int gp1_gl2_image_bind_main(struct gp1_renderer *renderer);

struct gp1_gl2_program {
  struct gp1_renderer *renderer; // WEAK
  const char *name;
  void (*del)(struct gp1_gl2_program *program);
};

void gp1_gl2_program_del(struct gp1_gl2_program *program);
struct gp1_gl2_program *gp1_gl2_program_new(int size);

int gp1_gl2_program_compile_shader(struct gp1_gl2_program *program,const char *src,int srcc,int type); // => shaderid
int gp1_gl2_program_link(struct gp1_gl2_program *program,int vshader,int fshader); // => programid

struct gp1_gl2_program *gp1_gl2_program_decal_new(struct gp1_renderer *renderer);

// (srcw,srch<0) to draw the whole image. (dstw,dsth<0) to infer from src.
void gp1_gl2_program_decal_render(
  struct gp1_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
);

struct gp1_gl2_program *gp1_gl2_program_tile_new(struct gp1_renderer *renderer);

void gp1_gl2_program_tiles_render(
  struct gp1_gl2_program *program,
  const struct gp1_render_tile *vtxv,int vtxc
);

#endif
