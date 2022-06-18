/* gp1_vm_render.h
 * Renderers built-in to the GP1 VM.
 * A host might skip this entirely and implement its own renderer.
 * But we'll try to cover all the typical cases.
 */
 
#ifndef GP1_VM_RENDER_H
#define GP1_VM_RENDER_H

struct gp1_renderer;
struct gp1_renderer_type;

// Artificial sanity limit on image sizes, either axis.
#define GP1_IMAGE_SIZE_LIMIT 8192

struct gp1_renderer {
  const struct gp1_renderer_type *type;
  int refc;
};

void gp1_renderer_del(struct gp1_renderer *renderer);
int gp1_renderer_ref(struct gp1_renderer *renderer);

struct gp1_renderer *gp1_renderer_new(const struct gp1_renderer_type *type);

/* Stride must be the minimum, padded to one byte.
 * (srcc) is knowable from (w,h,format) but you must provide it anyway as an assertion.
 */
int gp1_renderer_load_image(
  struct gp1_renderer *renderer,
  int imageid,int w,int h,int format,
  const void *src,int srcc
);

int gp1_renderer_render(struct gp1_renderer *renderer,const void *src,int srcc);

struct gp1_renderer_type {
  const char *name;
  const char *desc;
  int objlen;
  void (*del)(struct gp1_renderer *renderer);
  int (*init)(struct gp1_renderer *renderer);
  
  // Wrapper validates that (srcc) agrees with (w,h,format).
  // (and implicitly, that format is valid)
  int (*load_image)(struct gp1_renderer *renderer,int imageid,int w,int h,int stride,int format,const void *src,int srcc);
  
  int (*render)(struct gp1_renderer *renderer,const void *src,int srcc);
};

const struct gp1_renderer_type *gp1_renderer_type_by_index(int p);
const struct gp1_renderer_type *gp1_renderer_type_by_name(const char *src,int srcc);

#endif
