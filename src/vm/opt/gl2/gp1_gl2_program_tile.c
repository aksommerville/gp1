#include "gp1_gl2_internal.h"

/* Instance definition.
 */
 
struct gp1_gl2_tile_loc {
  GLuint screensize;
  GLuint sampler;
  GLuint bgcolor;
  GLuint fgcolor;
};
 
struct gp1_gl2_program_tile {
  struct gp1_gl2_program hdr;
  int programid_normal;
  int programid_1bit;
  int programid_8bit;
  struct gp1_gl2_tile_loc loc_normal;
  struct gp1_gl2_tile_loc loc_1bit;
  struct gp1_gl2_tile_loc loc_8bit;
};

#define PROGRAM ((struct gp1_gl2_program_tile*)program)

/* Shaders.
 */
 
static const char gp1_tile_vsrc[]=
  "uniform vec2 screensize;\n"
  "attribute vec2 aposition;\n"
  "attribute float atileid;\n"
  "attribute float axform;\n"
  "varying vec2 vtexcoord;\n"
  "varying mat2x3 xform;\n"
  "void main() {\n"
    "vec2 adjposition=(aposition*2.0)/screensize-1.0;\n"
    "gl_Position=vec4(adjposition.x,adjposition.y,0.0,1.0);\n"
    "float texx=mod(atileid,16.0)/16.0;\n"
    "float texy=floor(atileid/16.0)/16.0;\n"
    "vtexcoord=vec2(texx,texy);\n"
    
    "     if (axform<0.5) xform=mat2x3(  1.0, 0.0,0.0,  0.0,-1.0,1.0 );\n" // NONE
    "else if (axform<1.5) xform=mat2x3( -1.0, 0.0,1.0,  0.0,-1.0,1.0 );\n" // XREV
    "else if (axform<2.5) xform=mat2x3(  1.0, 0.0,0.0,  0.0, 1.0,0.0 );\n" // YREV
    "else if (axform<3.5) xform=mat2x3( -1.0, 0.0,1.0,  0.0, 1.0,0.0 );\n" // XREV|YREV
    "else if (axform<4.5) xform=mat2x3(  0.0,-1.0,1.0,  1.0, 0.0,0.0 );\n" // SWAP
    "else if (axform<5.5) xform=mat2x3(  0.0, 1.0,0.0,  1.0, 0.0,0.0 );\n" // SWAP|XREV
    "else if (axform<6.5) xform=mat2x3(  0.0,-1.0,1.0, -1.0, 0.0,1.0 );\n" // SWAP|YREV
    "else                 xform=mat2x3(  0.0, 1.0,0.0, -1.0, 0.0,1.0 );\n" // SWAP|XREV|YREV
  "}\n"
"";

static const char gp1_tile_normal_fsrc[]=
  "uniform sampler2D sampler;\n"
  "varying vec2 vtexcoord;\n"
  "varying mat2x3 xform;\n"
  "void main() {\n"
    "vec2 texcoord=vec2(\n"
      "gl_PointCoord.x*xform[0][0]+gl_PointCoord.y*xform[0][1]+xform[0][2],\n"
      "gl_PointCoord.x*xform[1][0]+gl_PointCoord.y*xform[1][1]+xform[1][2]\n"
    ");\n"
    "texcoord=vtexcoord+texcoord/16.0;\n"
    "gl_FragColor=texture2D(sampler,texcoord);\n"
  "}\n"
"";

static const char gp1_tile_1bit_fsrc[]=
  "uniform sampler2D sampler;\n"
  "uniform vec4 bgcolor;\n"
  "uniform vec4 fgcolor;\n"
  "varying vec2 vtexcoord;\n"
  "varying mat2x3 xform;\n"
  "void main() {\n"
    "vec2 texcoord=vec2(\n"
      "gl_PointCoord.x*xform[0][0]+gl_PointCoord.y*xform[0][1]+xform[0][2],\n"
      "gl_PointCoord.x*xform[1][0]+gl_PointCoord.y*xform[1][1]+xform[1][2]\n"
    ");\n"
    "texcoord=vtexcoord+texcoord/16.0;\n"
    "vec4 srccolor=texture2D(sampler,texcoord);\n"
    "vec4 mixcolor=mix(bgcolor,fgcolor,srccolor.r);\n"
    "gl_FragColor=vec4(mixcolor.rgb,mixcolor.a*srccolor.a);\n"
  "}\n"
"";

static const char gp1_tile_8bit_fsrc[]=
  "uniform sampler2D sampler;\n"
  "uniform vec4 fgcolor;\n"
  "varying vec2 vtexcoord;\n"
  "varying mat2x3 xform;\n"
  "void main() {\n"
    "vec2 texcoord=vec2(\n"
      "gl_PointCoord.x*xform[0][0]+gl_PointCoord.y*xform[0][1]+xform[0][2],\n"
      "gl_PointCoord.x*xform[1][0]+gl_PointCoord.y*xform[1][1]+xform[1][2]\n"
    ");\n"
    "texcoord=vtexcoord+texcoord/16.0;\n"
    "vec4 srccolor=texture2D(sampler,texcoord);\n"
    "gl_FragColor=vec4(fgcolor.rgb,fgcolor.a*srccolor.r);\n"
  "}\n"
"";

/* Cleanup.
 */
 
static void _gp1_gl2_program_tile_del(struct gp1_gl2_program *program) {
  if (PROGRAM->programid_normal>0) glDeleteProgram(PROGRAM->programid_normal);
  if (PROGRAM->programid_1bit>0) glDeleteProgram(PROGRAM->programid_1bit);
  if (PROGRAM->programid_8bit>0) glDeleteProgram(PROGRAM->programid_8bit);
}

/* Compile, etc.
 */
 
static int gp1_gl2_program_tile_setup(struct gp1_gl2_program *program,const char *src,int srcc,struct gp1_gl2_tile_loc *loc) {

  int vshader=gp1_gl2_program_compile_shader(program,gp1_tile_vsrc,sizeof(gp1_tile_vsrc),GL_VERTEX_SHADER);
  if (vshader<0) return -1;
  int fshader=gp1_gl2_program_compile_shader(program,src,srcc,GL_FRAGMENT_SHADER);
  if (fshader<0) {
    glDeleteShader(vshader);
    return -1;
  }
  
  int programid=gp1_gl2_program_link(program,vshader,fshader);
  glDeleteShader(vshader);
  glDeleteShader(fshader);
  if (programid<0) return -1;
  
  loc->screensize=glGetUniformLocation(programid,"screensize");
  loc->sampler=glGetUniformLocation(programid,"sampler");
  loc->bgcolor=glGetUniformLocation(programid,"bgcolor");
  loc->fgcolor=glGetUniformLocation(programid,"fgcolor");
  
  glBindAttribLocation(programid,0,"aposition");
  glBindAttribLocation(programid,1,"atileid");
  glBindAttribLocation(programid,2,"axform");
  
  return programid;
}

/* New.
 */
 
struct gp1_gl2_program *gp1_gl2_program_tile_new(struct gp1_renderer *renderer) {
  struct gp1_gl2_program *program=gp1_gl2_program_new(sizeof(struct gp1_gl2_program_tile));
  if (!program) return 0;
  
  program->renderer=renderer;
  program->name="tile";
  program->del=_gp1_gl2_program_tile_del;
  
  if (
    ((PROGRAM->programid_normal=gp1_gl2_program_tile_setup(program,gp1_tile_normal_fsrc,sizeof(gp1_tile_normal_fsrc),&PROGRAM->loc_normal))<0)||
    ((PROGRAM->programid_1bit=gp1_gl2_program_tile_setup(program,gp1_tile_1bit_fsrc,sizeof(gp1_tile_1bit_fsrc),&PROGRAM->loc_1bit))<0)||
    ((PROGRAM->programid_8bit=gp1_gl2_program_tile_setup(program,gp1_tile_8bit_fsrc,sizeof(gp1_tile_8bit_fsrc),&PROGRAM->loc_8bit))<0)||
  0) {
    gp1_gl2_program_del(program);
    return 0;
  }
  
  return program;
}

/* Render.
 */
 
void gp1_gl2_program_tiles_render(
  struct gp1_gl2_program *program,
  const struct gp1_render_tile *vtxv,int vtxc
) {
  if (vtxc<1) return;
  struct gp1_renderer *renderer=program->renderer;
  if (!RENDERER->srcimage) return;
  if (!RENDERER->dstimage) return; // to framebuffer only; never to main
  
  struct gp1_gl2_tile_loc *loc;
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

  glUniform2f(loc->screensize,RENDERER->dstimage->w,RENDERER->dstimage->h);
  glEnable(GL_POINT_SPRITE);
  glPointSize(RENDERER->srcimage->w>>4);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D,RENDERER->srcimage->texid);
  glUniform1i(loc->sampler,GL_TEXTURE0);
  
  //TODO Tell shader about the point aspect; we should be able to handle non-square tiles.
  int pointw=RENDERER->srcimage->w>>4;
  int pointh=RENDERER->srcimage->h>>4;
  if (pointw>=pointh) {
    glPointSize(pointw);
  } else {
    glPointSize(pointh);
  }
  
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(0,2,GL_SHORT,0,sizeof(vtxv[0]),&vtxv[0].dstx);
  glVertexAttribPointer(1,1,GL_UNSIGNED_BYTE,0,sizeof(vtxv[0]),&vtxv[0].tileid);
  glVertexAttribPointer(2,1,GL_UNSIGNED_BYTE,0,sizeof(vtxv[0]),&vtxv[0].xform);
  glDrawArrays(GL_POINTS,0,vtxc);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
}
