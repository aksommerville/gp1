#include "gp1_gl2_internal.h"

/* Cleanup.
 */
 
static void _gp1_gl2_del(struct gp1_renderer *renderer) {
  fprintf(stderr,"%s\n",__func__);
}

/* Init.
 */
 
static int _gp1_gl2_init(struct gp1_renderer *renderer) {
  fprintf(stderr,"%s\n",__func__);
  return 0;
}

/* Load image.
 */
 
static int _gp1_gl2_load_image(
  struct gp1_renderer *renderer,
  int imageid,int w,int h,int stride,int format,
  const void *src,int srcc
) {
  fprintf(stderr,"%s #%d (%d,%d) stride=%d format=%d src=%p srcc=%d\n",__func__,imageid,w,h,stride,format,src,srcc);
  return 0;
}

/* Render.
 */
 
static int _gp1_gl2_render(struct gp1_renderer *renderer,const void *src,int srcc) {
  fprintf(stderr,"%s srcc=%d\n",__func__,srcc);
  return 0;
}

/* Type definition.
 */
 
const struct gp1_renderer_type gp1_renderer_type_gl2={
  .name="gl2",
  .desc="OpenGL version 2, widely supported accelerated graphics.",
  .objlen=sizeof(struct gp1_renderer_gl2),
  .del=_gp1_gl2_del,
  .init=_gp1_gl2_init,
  .load_image=_gp1_gl2_load_image,
  .render=_gp1_gl2_render,
};
