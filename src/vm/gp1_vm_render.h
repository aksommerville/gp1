/* gp1_vm_render.h
 * Renderers built-in to the GP1 VM.
 * A host might skip this entirely and implement its own renderer.
 * But we'll try to cover all the typical cases.
 */
 
#ifndef GP1_VM_RENDER_H
#define GP1_VM_RENDER_H

struct gp1_renderer;
struct gp1_renderer_type;
struct gp1_rom;

// Artificial sanity limit on image sizes, either axis.
#define GP1_IMAGE_SIZE_LIMIT 8192

struct gp1_renderer {
  const struct gp1_renderer_type *type;
  int refc;
  struct gp1_rom *rom;
  int mainw,mainh; // Owner should set directly
};

void gp1_renderer_del(struct gp1_renderer *renderer);
int gp1_renderer_ref(struct gp1_renderer *renderer);

struct gp1_renderer *gp1_renderer_new(const struct gp1_renderer_type *type);

int gp1_renderer_set_rom(struct gp1_renderer *renderer,struct gp1_rom *rom);

int gp1_renderer_render(struct gp1_renderer *renderer,const void *src,int srcc);
int gp1_renderer_end_frame(struct gp1_renderer *renderer);

struct gp1_renderer_type {
  const char *name;
  const char *desc;
  int objlen;
  void (*del)(struct gp1_renderer *renderer);
  int (*init)(struct gp1_renderer *renderer);
  
  // Clear image cache, etc.
  int (*rom_changed)(struct gp1_renderer *renderer);
  
  int (*render)(struct gp1_renderer *renderer,const void *src,int srcc);
  int (*end_frame)(struct gp1_renderer *renderer);
};

const struct gp1_renderer_type *gp1_renderer_type_by_index(int p);
const struct gp1_renderer_type *gp1_renderer_type_by_name(const char *src,int srcc);

int gp1_image_calculate_stride(int format,int w);

/* Input must have minimum stride, output will too.
 */
void *gp1_convert_image(int dstfmt,const void *src,int w,int h,int srcfmt);

#endif
