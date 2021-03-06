#include "gp1_vm_render.h"
#include "gp1_rom.h"
#include "gp1/gp1.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

/* Global type registry.
 */
 
extern const struct gp1_renderer_type gp1_renderer_type_gl2;
extern const struct gp1_renderer_type gp1_renderer_type_nullrender;
 
static const struct gp1_renderer_type *gp1_renderer_typev[]={
#if GP1_USE_gl2
  &gp1_renderer_type_gl2,
#endif
//TODO softrender
//TODO metal?
//TODO direct3d?
//TODO vulkan?
//  &gp1_renderer_type_nullrender,//XXX temporarily disabled so i can fail sanely. TODO add a "not as a fallback" flag
};

const struct gp1_renderer_type *gp1_renderer_type_by_index(int p) {
  if (p<0) return 0;
  int c=sizeof(gp1_renderer_typev)/sizeof(void*);
  if (p>=c) return 0;
  return gp1_renderer_typev[p];
}

const struct gp1_renderer_type *gp1_renderer_type_by_name(const char *name,int namec) {
  if (!name) return 0;
  if (namec<0) { namec=0; while (name[namec]) namec++; }
  const struct gp1_renderer_type **p=gp1_renderer_typev;
  int i=sizeof(gp1_renderer_typev)/sizeof(void*);
  for (;i-->0;p++) {
    if (memcmp((*p)->name,name,namec)) continue;
    if ((*p)->name[namec]) continue;
    return *p;
  }
  return 0;
}

/* Instance wrapper.
 */

void gp1_renderer_del(struct gp1_renderer *renderer) {
  if (!renderer) return;
  if (renderer->refc-->1) return;
  if (renderer->type->del) renderer->type->del(renderer);
  gp1_rom_del(renderer->rom);
  free(renderer);
}

int gp1_renderer_ref(struct gp1_renderer *renderer) {
  if (!renderer) return -1;
  if (renderer->refc<1) return -1;
  if (renderer->refc==INT_MAX) return -1;
  renderer->refc++;
  return 0;
}

struct gp1_renderer *gp1_renderer_new(const struct gp1_renderer_type *type) {
  if (!type) return 0;
  struct gp1_renderer *renderer=calloc(1,type->objlen);
  if (!renderer) return 0;
  renderer->type=type;
  renderer->refc=1;
  if (type->init&&(type->init(renderer)<0)) {
    gp1_renderer_del(renderer);
    return 0;
  }
  return renderer;
}

int gp1_renderer_set_rom(struct gp1_renderer *renderer,struct gp1_rom *rom) {
  if (renderer->rom==rom) return 0;
  if (rom&&(gp1_rom_ref(rom)<0)) return -1;
  gp1_rom_del(renderer->rom);
  renderer->rom=rom;
  if (renderer->type->rom_changed) {
    if (renderer->type->rom_changed(renderer)<0) return -1;
  }
  return 0;
}

int gp1_renderer_render(struct gp1_renderer *renderer,const void *src,int srcc) {
  if (!src||(srcc<1)) return 0;
  if (!renderer||!renderer->type->render) return -1;
  return renderer->type->render(renderer,src,srcc);
}

int gp1_renderer_end_frame(struct gp1_renderer *renderer) {
  if (!renderer->type->end_frame) return 0;
  return renderer->type->end_frame(renderer);
}
