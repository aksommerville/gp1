#include "gp1_gl2_internal.h"

/* Cleanup one image.
 */
 
void gp1_gl2_image_cleanup(struct gp1_gl2_image *image) {
  if (image->texid) glDeleteTextures(1,&image->texid);
  if (image->fbid) glDeleteFramebuffers(1,&image->fbid);
}

/* Search.
 */
 
int gp1_gl2_image_search(const struct gp1_renderer *renderer,int imageid) {
  if (imageid<0) return -1;
  if (imageid<RENDERER->imagecontigc) return imageid;
  int lo=RENDERER->imagecontigc,hi=RENDERER->imagec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (imageid<RENDERER->imagev[ck].imageid) hi=ck;
    else if (imageid>RENDERER->imagev[ck].imageid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert.
 */
 
struct gp1_gl2_image *gp1_gl2_image_insert(struct gp1_renderer *renderer,int p,int imageid) {

  if ((p<0)||(p>RENDERER->imagec)) return 0;
  if (imageid<0) return 0;
  if (p&&(imageid<=RENDERER->imagev[p-1].imageid)) return 0;
  if ((p<RENDERER->imagec)&&(imageid>=RENDERER->imagev[p].imageid)) return 0;
  
  if (RENDERER->imagec>=RENDERER->imagea) {
    int na=RENDERER->imagea+16;
    if (na>INT_MAX/sizeof(struct gp1_gl2_image)) return 0;
    void *nv=realloc(RENDERER->imagev,sizeof(struct gp1_gl2_image)*na);
    if (!nv) return 0;
    RENDERER->imagev=nv;
    RENDERER->imagea=na;
  }
  
  struct gp1_gl2_image *image=RENDERER->imagev+p;
  memmove(image+1,image,sizeof(struct gp1_gl2_image)*(RENDERER->imagec-p));
  memset(image,0,sizeof(struct gp1_gl2_image));
  image->imageid=imageid;
  RENDERER->imagec++;
  
  while (
    (RENDERER->imagecontigc<RENDERER->imagec)&&
    (RENDERER->imagev[RENDERER->imagecontigc].imageid==RENDERER->imagecontigc)
  ) RENDERER->imagecontigc++;
  
  RENDERER->srcimageid=-1;
  RENDERER->dstimageid=-1;
  RENDERER->srcimage=0;
  RENDERER->dstimage=0;
  
  return image;
}

/* Load raw image into gl2 texture, wrapped by our image object.
 */
 
static void gp1_gl2_set_image(
  struct gp1_renderer *renderer,struct gp1_gl2_image *image,
  int w,int h,int fmt,
  const void *src,int srcc
) {
  fprintf(stderr,"%s %dx%d fmt=%d srcc=%d\n",__func__,w,h,fmt,srcc);
  
  image->color_mode=GP1_GL2_COLOR_MODE_NORMAL;

  // Convert to GL-friendly format if needed.
  switch (fmt) {
    case GP1_IMAGE_FORMAT_A1: {
        void *nv=gp1_convert_image(GP1_IMAGE_FORMAT_A8,src,w,h,fmt);
        if (nv) {
          gp1_gl2_set_image(renderer,image,w,h,GP1_IMAGE_FORMAT_A8,nv,0);
          image->color_mode=GP1_GL2_COLOR_MODE_1BIT;
          free(nv);
        }
      } return;
    case GP1_IMAGE_FORMAT_RGB565: {
        void *nv=gp1_convert_image(GP1_IMAGE_FORMAT_RGB888,src,w,h,fmt);
        if (nv) {
          gp1_gl2_set_image(renderer,image,w,h,GP1_IMAGE_FORMAT_RGB888,nv,0);
          free(nv);
        }
      } return;
    case GP1_IMAGE_FORMAT_ARGB1555: {
        void *nv=gp1_convert_image(GP1_IMAGE_FORMAT_RGBA8888,src,w,h,fmt);
        if (nv) {
          gp1_gl2_set_image(renderer,image,w,h,GP1_IMAGE_FORMAT_RGBA8888,nv,0);
          free(nv);
        }
      } return;
  }
  
  image->w=w;
  image->h=h;

  if (!image->texid) {
    glGenTextures(1,&image->texid);
    if (!image->texid) {
      glGenTextures(1,&image->texid);
      if (!image->texid) return;
    }
  }
  
  glBindTexture(GL_TEXTURE_2D,image->texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

  //TODO Confirm RGB888 works for sizes that are not multiples of 4 -- i think it won't
  switch (fmt) {
    case GP1_IMAGE_FORMAT_A8: {
        image->color_mode=GP1_GL2_COLOR_MODE_8BIT;
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,src);
      } break;
    case GP1_IMAGE_FORMAT_RGB888: {
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,src);
      } break;
    case GP1_IMAGE_FORMAT_RGBA8888: {
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,src);
      } break;
  }
}

/* Get image.
 */
 
struct gp1_gl2_image *gp1_gl2_image_get(struct gp1_renderer *renderer,int imageid) {

  // Best case scenario: We happen to have it queued up in one of the memo slots.
  if (imageid==RENDERER->srcimageid) return RENDERER->srcimage;
  if (imageid==RENDERER->dstimageid) return RENDERER->dstimage;
  
  // Don't let invalid IDs pass.
  if (imageid<0) return 0;

  // Also pretty good: It's present in our image list.
  int p=gp1_gl2_image_search(renderer,imageid);
  if (p>=0) return RENDERER->imagev+p;
  
  // Find it in the ROM and add to our image list (even if absent from ROM).
  struct gp1_gl2_image *image=gp1_gl2_image_insert(renderer,-p-1,imageid);
  if (!image) return 0;
  if (renderer->rom) {
    if (imageid) {
      void *src=0;
      int srcc;
      uint16_t w=0,h=0;
      uint8_t fmt=0;
      if ((srcc=gp1_rom_image_decode(&src,&w,&h,&fmt,renderer->rom,imageid))>=0) {
        gp1_gl2_set_image(renderer,image,w,h,fmt,src,srcc);
        free(src);
      }
    } else {
      gp1_gl2_set_image(renderer,image,renderer->rom->fbw,renderer->rom->fbh,GP1_IMAGE_FORMAT_RGBA8888,0,0);
      gp1_gl2_image_require_input(renderer,image);
    }
  }
  return image;
}

/* Load (srcimage,dstimage) if state disagrees with memo.
 */

void gp1_gl2_require_images(struct gp1_renderer *renderer) {
  if (RENDERER->srcimageid!=RENDERER->state.srcimageid) {
    RENDERER->srcimage=gp1_gl2_image_get(renderer,RENDERER->state.srcimageid);
    RENDERER->srcimageid=RENDERER->state.srcimageid;
  }
  if (RENDERER->dstimageid!=RENDERER->state.dstimageid) {
    RENDERER->dstimage=gp1_gl2_image_get(renderer,RENDERER->state.dstimageid);
    RENDERER->dstimageid=RENDERER->state.dstimageid;
    gp1_gl2_image_bind_framebuffer(renderer,RENDERER->dstimage);
  }
}

/* Require a framebuffer attached to one image.
 */
 
int gp1_gl2_image_require_input(struct gp1_renderer *renderer,struct gp1_gl2_image *image) {
  if (!image) return -1;
  if (image->fbid) return 0;
  fprintf(stderr,"%s generating framebuffer for image %d\n",__func__,image->imageid);
  
  glGenFramebuffers(1,&image->fbid);
  if (!image->fbid) return -1;
  
  glBindFramebuffer(GL_FRAMEBUFFER,image->fbid);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,image->texid,0);
  
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  return 0;
}

/* Require framebuffer and bind it.
 */
 
int gp1_gl2_image_bind_framebuffer(struct gp1_renderer *renderer,struct gp1_gl2_image *image) {
  if (gp1_gl2_image_require_input(renderer,image)<0) return -1;
  glBindFramebuffer(GL_FRAMEBUFFER,image->fbid);
  glViewport(0,0,image->w,image->h);
  return 0;
}

int gp1_gl2_image_bind_main(struct gp1_renderer *renderer) {
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glViewport(0,0,renderer->mainw,renderer->mainh);
  RENDERER->dstimageid=-1;
  RENDERER->dstimage=0;
  if (RENDERER->srcimageid!=0) {
    RENDERER->srcimage=gp1_gl2_image_get(renderer,0);
    RENDERER->srcimageid=0;
  }
  return 0;
}
