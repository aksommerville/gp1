#include "gp1_gl2_internal.h"
#include <math.h>

/* Instance definition.
 */
 
struct gp1_gl2_loc { // uniform locations
  GLuint screensize;
  GLuint texsize;
  GLuint sampler;
  GLuint inverty;
  GLuint fgcolor;
  GLuint bgcolor;
};
 
struct gp1_gl2_program_decal {
  struct gp1_gl2_program hdr;
  int programid_normal;
  int programid_1bit;
  int programid_8bit;
  struct gp1_gl2_loc loc_normal;
  struct gp1_gl2_loc loc_1bit;
  struct gp1_gl2_loc loc_8bit;
};

#define PROGRAM ((struct gp1_gl2_program_decal*)program)

/* Shaders.
 */
 
struct gp1_decal_vertex {
  int16_t x;
  int16_t y;
  int16_t tx;
  int16_t ty;
};
 
static const char gp1_decal_vsrc[]=
  "uniform vec2 screensize;\n"
  "uniform vec2 texsize;\n"
  "uniform float inverty;\n"
  "attribute vec2 aposition;\n"
  "attribute vec2 atexcoord;\n"
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "vec2 adjposition=(aposition*2.0)/screensize-1.0;\n"
    "adjposition.y*=inverty;\n"
    "gl_Position=vec4(adjposition.x,adjposition.y,0.0,1.0);\n"
    "vtexcoord=atexcoord/texsize;\n"
  "}\n"
"";

static const char gp1_decal_fsrc[]=
  "uniform sampler2D sampler;\n"
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "gl_FragColor=texture2D(sampler,vtexcoord);\n"
  "}\n"
"";

static const char gp1_decal_1bit_fsrc[]=
  "uniform sampler2D sampler;\n"
  "uniform vec4 bgcolor;\n"
  "uniform vec4 fgcolor;\n"
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "vec4 srccolor=texture2D(sampler,vtexcoord);\n"
    "gl_FragColor=vec4(mix(bgcolor,fgcolor,srccolor.r).rgb,bgcolor.a*srccolor.a);\n"
  "}\n"
"";

static const char gp1_decal_8bit_fsrc[]=
  "uniform sampler2D sampler;\n"
  "uniform vec4 bgcolor;\n"
  "uniform vec4 fgcolor;\n"
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "vec4 srccolor=texture2D(sampler,vtexcoord);\n"
    "gl_FragColor=vec4(fgcolor.rgb,fgcolor.a*srccolor.r);\n"
  "}\n"
"";

/* Cleanup.
 */
 
static void _gp1_gl2_program_decal_del(struct gp1_gl2_program *program) {
  if (PROGRAM->programid_normal>0) glDeleteProgram(PROGRAM->programid_8bit);
  if (PROGRAM->programid_1bit>0) glDeleteProgram(PROGRAM->programid_1bit);
  if (PROGRAM->programid_8bit>0) glDeleteProgram(PROGRAM->programid_8bit);
}

/* Compile sources, etc.
 */
 
static int gp1_gl2_program_decal_setup(struct gp1_gl2_program *program,const char *fsrc,int fsrcc,struct gp1_gl2_loc *loc) {

  int vshader=gp1_gl2_program_compile_shader(program,gp1_decal_vsrc,sizeof(gp1_decal_vsrc),GL_VERTEX_SHADER);
  if (vshader<0) {
    return -1;
  }
  int fshader=gp1_gl2_program_compile_shader(program,fsrc,fsrcc,GL_FRAGMENT_SHADER);
  if (fshader<0) {
    glDeleteShader(vshader);
    return -1;
  }
  
  int programid=gp1_gl2_program_link(program,vshader,fshader);
  glDeleteShader(vshader);
  glDeleteShader(fshader);
  if (programid<0) {
    return -1;
  }
  
  loc->screensize=glGetUniformLocation(programid,"screensize");
  loc->texsize=glGetUniformLocation(programid,"texsize");
  loc->sampler=glGetUniformLocation(programid,"sampler");
  loc->inverty=glGetUniformLocation(programid,"inverty");
  loc->bgcolor=glGetUniformLocation(programid,"bgcolor");
  loc->fgcolor=glGetUniformLocation(programid,"fgcolor");
  
  glBindAttribLocation(programid,0,"aposition");
  glBindAttribLocation(programid,1,"atexcoord");
  
  return programid;
}

/* New.
 */
 
struct gp1_gl2_program *gp1_gl2_program_decal_new(struct gp1_renderer *renderer) {
  struct gp1_gl2_program *program=gp1_gl2_program_new(sizeof(struct gp1_gl2_program_decal));
  if (!program) return 0;
  
  program->renderer=renderer;
  program->name="decal";
  program->del=_gp1_gl2_program_decal_del;
  
  if (
    ((PROGRAM->programid_normal=gp1_gl2_program_decal_setup(program,gp1_decal_fsrc,sizeof(gp1_decal_fsrc),&PROGRAM->loc_normal))<0)||
    ((PROGRAM->programid_1bit=gp1_gl2_program_decal_setup(program,gp1_decal_1bit_fsrc,sizeof(gp1_decal_1bit_fsrc),&PROGRAM->loc_1bit))<0)||
    ((PROGRAM->programid_8bit=gp1_gl2_program_decal_setup(program,gp1_decal_8bit_fsrc,sizeof(gp1_decal_8bit_fsrc),&PROGRAM->loc_8bit))<0)||
  0) {
    gp1_gl2_program_del(program);
    return 0;
  }
  
  return program;
}

/* Render.
 */
 
void gp1_gl2_program_decal_render(
  struct gp1_gl2_program *program,
  int dstx,int dsty,int dstw,int dsth,
  int srcx,int srcy,int srcw,int srch
) {
  struct gp1_renderer *renderer=program->renderer;
  if (!RENDERER->srcimage) return;
  // dstimage null is ok; we can render to the main.
  
  if (srcw<0) srcw=RENDERER->srcimage->w-srcx;
  if (srch<0) srch=RENDERER->srcimage->h-srcy;
  if (dstw<0) dstw=srcw;
  if (dsth<0) dsth=srch;
  
  struct gp1_gl2_loc *loc;
  switch (RENDERER->srcimage->color_mode) {
    case GP1_GL2_COLOR_MODE_NORMAL: {
        glUseProgram(PROGRAM->programid_normal);
        loc=&PROGRAM->loc_normal;
      } break;
    case GP1_GL2_COLOR_MODE_1BIT: {
        glUseProgram(PROGRAM->programid_1bit);
        loc=&PROGRAM->loc_1bit;
        uint8_t r,g,b,a;
        r=RENDERER->state.bgcolor>>24;
        g=RENDERER->state.bgcolor>>16;
        b=RENDERER->state.bgcolor>>8;
        a=RENDERER->state.bgcolor;
        glUniform4f(loc->bgcolor,r/255.0f,g/255.0f,b/255.0f,a/255.0f);
        r=RENDERER->state.fgcolor>>24;
        g=RENDERER->state.fgcolor>>16;
        b=RENDERER->state.fgcolor>>8;
        a=RENDERER->state.fgcolor;
        glUniform4f(loc->fgcolor,r/255.0f,g/255.0f,b/255.0f,a/255.0f);
      } break;
    case GP1_GL2_COLOR_MODE_8BIT: {
        glUseProgram(PROGRAM->programid_8bit);
        loc=&PROGRAM->loc_8bit;
        uint8_t r,g,b,a;
        r=RENDERER->state.fgcolor>>24;
        g=RENDERER->state.fgcolor>>16;
        b=RENDERER->state.fgcolor>>8;
        a=RENDERER->state.fgcolor;
        glUniform4f(loc->fgcolor,r/255.0f,g/255.0f,b/255.0f,a/255.0f);
      } break;
    default: return;
  }
  
  // Reverse X and Y axis if requested. (swap comes later)
  switch (RENDERER->state.xform&(GP1_XFORM_XREV|GP1_XFORM_YREV)) {
    case GP1_XFORM_NONE: break;
    case GP1_XFORM_XREV: srcx+=srcw; srcw=-srcw; break;
    case GP1_XFORM_YREV: srcy+=srch; srch=-srch; break;
    case GP1_XFORM_XREV|GP1_XFORM_YREV: {
        srcx+=srcw; srcw=-srcw;
        srcy+=srch; srch=-srch;
      } break;
  }
  
  if (RENDERER->dstimage) {
    glUniform1f(loc->inverty,1.0f);
    glUniform2f(loc->screensize,RENDERER->dstimage->w,RENDERER->dstimage->h);
  } else {
    glUniform1f(loc->inverty,-1.0f);
    glUniform2f(loc->screensize,renderer->mainw,renderer->mainh);
  }
  glUniform2f(loc->texsize,RENDERER->srcimage->w,RENDERER->srcimage->h);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,RENDERER->srcimage->texid);
  glUniform1i(loc->sampler,GL_TEXTURE0);
  
  struct gp1_decal_vertex vtxv[4]={
    {dstx     ,dsty     ,srcx     ,srcy     },
    {dstx     ,dsty+dsth,srcx     ,srcy+srch},
    {dstx+dstw,dsty     ,srcx+srcw,srcy     },
    {dstx+dstw,dsty+dsth,srcx+srcw,srcy+srch},
  };
  if (RENDERER->state.xform&GP1_XFORM_SWAP) {
    vtxv[1].tx=srcx+srcw;
    vtxv[1].ty=srcy;
    vtxv[2].tx=srcx;
    vtxv[2].ty=srcy+srch;
  }
  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(vtxv[0]),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_SHORT,0,sizeof(vtxv[0]),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,sizeof(vtxv)/sizeof(vtxv[0]));
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}
