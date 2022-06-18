#ifndef GP1_GL2_INTERNAL_H
#define GP1_GL2_INTERNAL_H

#include "gp1/gp1.h"
#include "vm/gp1_vm_render.h"
#include <stdio.h>

struct gp1_renderer_gl2 {
  struct gp1_renderer hdr;
};

#define RENDERER ((struct gp1_renderer_gl2*)renderer)

#endif
