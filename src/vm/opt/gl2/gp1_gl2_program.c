#include "gp1_gl2_internal.h"

/* Delete.
 */
 
void gp1_gl2_program_del(struct gp1_gl2_program *program) {
  if (!program) return;
  if (program->del) program->del(program);
  free(program);
}

/* New.
 */
 
struct gp1_gl2_program *gp1_gl2_program_new(int size) {
  if (size<(int)sizeof(struct gp1_gl2_program)) return 0;
  struct gp1_gl2_program *program=calloc(1,size);
  if (!program) return 0;
  return program;
}

/* Compile.
 */

int gp1_gl2_program_compile_shader(struct gp1_gl2_program *program,const char *src,int srcc,int type) {

  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char prefix[128];
  int prefixc=snprintf(prefix,sizeof(prefix),"#version 120\n");//TODO configurable
  if ((prefixc<1)||(prefixc>=sizeof(prefix))) return -1;
  
  int shader=glCreateShader(type);
  if (shader<1) return -1;
  
  const GLchar *const stringv[]={prefix,src};
  GLint lenv[]={prefixc,srcc};
  glShaderSource(shader,2,stringv,lenv);
  
  glCompileShader(shader);
  GLint status=0;
  glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
  if (status) return shader;
  
  GLint loga=0;
  glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&loga);
  fprintf(stderr,"gl2: Failed to compile %s shader for program '%s'.\n",(type==GL_VERTEX_SHADER)?"vertex":"fragment",program->name);
  if (loga>0) {
    GLchar *log=malloc(loga);
    if (log) {
      GLsizei logc=0;
      glGetShaderInfoLog(shader,loga,&logc,log);
      if ((logc>0)&&(logc<=loga)) {
        while ((logc>0)&&((unsigned char)log[logc-1]<=0x20)) logc--;
        fprintf(stderr,"%.*s\n",logc,log);
      }
      free(log);
    }
  }
  
  glDeleteShader(shader);
  return -1;
}

/* Link.
 */
 
int gp1_gl2_program_link(struct gp1_gl2_program *program,int vshader,int fshader) {

  int programid=glCreateProgram();
  if (programid<1) return -1;
  
  glAttachShader(programid,vshader);
  glAttachShader(programid,fshader);
  
  glLinkProgram(programid);
  GLint status=0;
  glGetProgramiv(programid,GL_LINK_STATUS,&status);
  if (status) return programid;
  
  GLint loga=0;
  glGetProgramiv(programid,GL_INFO_LOG_LENGTH,&loga);
  fprintf(stderr,"gl2: Failed to link program '%s'.\n",program->name);
  if (loga>0) {
    GLchar *log=malloc(loga);
    if (log) {
      GLsizei logc=0;
      glGetProgramInfoLog(programid,loga,&logc,log);
      if ((logc>0)&&(logc<=loga)) {
        while ((logc>0)&&((unsigned char)log[logc-1]<=0x20)) logc--;
        fprintf(stderr,"%.*s\n",logc,log);
      }
      free(log);
    }
  }
  
  glDeleteProgram(programid);
  return -1;
}
