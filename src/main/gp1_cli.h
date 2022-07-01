/* gp1_cli.h
 */
 
#ifndef GP1_CLI_H
#define GP1_CLI_H

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

/* Configuration.
 *****************************************************/
 
#define GP1_COMMAND_UNSET        0
#define GP1_COMMAND_help         1
#define GP1_COMMAND_run          2
#define GP1_COMMAND_pack         3
#define GP1_COMMAND_convertchunk 5
#define GP1_COMMAND_info         6
 
struct gp1_config {
  const char *exename;
  int command;
  char *dstpath;
  char **posv; // positional args, typically input files but may vary on command
  int posc,posa;
  char *video_driver_name;
  char *audio_driver_name;
  char *input_driver_names; // comma-delimited
  char *renderer_name;
  int audio_rate; // hz
  int audio_chanc;
  char *input_config_path;
};

void gp1_config_cleanup(struct gp1_config *config);

int gp1_config_init(struct gp1_config *config,int argc,char **argv);

int gp1_command_eval(const char *src,int srcc);
const char *gp1_command_repr(int command);

void gp1_config_print_help(struct gp1_config *config);

/* Command-specific entry points.
 *******************************************************/
 
#define GP1_CHUNK_TYPE(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|d)

struct gp1_rompack_strings {
  struct gp1_rompack_strings_entry {
    uint16_t language;
    uint16_t stringid;
    char *v;
    int c;
  } *entryv;
  int entryc,entrya;
};

void gp1_rompack_strings_cleanup(struct gp1_rompack_strings *strings);

int gp1_rompack_strings_search(const struct gp1_rompack_strings *strings,uint16_t language,uint16_t stringid);
int gp1_rompack_strings_add_handoff(struct gp1_rompack_strings *strings,uint16_t stringid,uint16_t language,char *src,int srcc);
int gp1_rompack_strings_add_line(struct gp1_rompack_strings *strings,const char *src,int srcc); // we strip comment; empty is ok
int gp1_rompack_strings_add_text(struct gp1_rompack_strings *strings,const char *src,int srcc,const char *refname);
 
struct gp1_rompack {
  struct gp1_config *config;
  struct gp1_rompack_strings strings;
  
  struct gp1_rompack_input {
    void *v;
    int c;
    char *path;
  } *inputv;
  int inputc,inputa;
  
  struct gp1_rompack_chunk {
    uint32_t type;
    void *v;
    int c;
  } *chunkv;
  int chunkc,chunka;
};

void gp1_rompack_cleanup(struct gp1_rompack *rompack);

// We log all errors.
int gp1_rompack_add_input(struct gp1_rompack *rompack,const void *src,int srcc,const char *path);
int gp1_rompack_handoff_input(struct gp1_rompack *rompack,void *src,int srcc,const char *path);
int gp1_rompack_add_chunk(struct gp1_rompack *rompack,uint32_t type,const void *src,int srcc);
int gp1_rompack_handoff_chunk(struct gp1_rompack *rompack,uint32_t type,void *src,int srcc);
int gp1_rompack_generate_output(void *dstpp,struct gp1_rompack *rompack);

// We log all errors.
int gp1_convert_chunk(void *dstpp,struct gp1_config *config,const void *src,int srcc,const char *srcpath);

void gp1_print_rom_info(struct gp1_config *config,const void *src,int srcc,const char *path);

/* Odds, ends.
 ************************************************************/
 
const char *gp1_get_basename(const char *path); // => never null
const char *gp1_get_suffix(const char *path); // => never null
int gp1_valid_chunk_type(uint32_t type);
int gp1_image_format_eval(const char *src,int srcc);
int gp1_stride_for_image_format(int format,int w);

#endif
