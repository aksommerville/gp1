#include "gp1_cli.h"
#include "gp1_host.h"
#include "io/gp1_io_fs.h"
#include "io/gp1_io_os.h"
#include "vm/gp1_rom.h"

/* Run game.
 */
 
static int gp1_main_run(struct gp1_config *config) {
  if (config->posc>1) {
    fprintf(stderr,"%s: Multiple ROM files\n",config->exename);
    return -1;
  }
  if (!config->posc) {
    fprintf(stderr,"%s: ROM file required\n",config->exename);
    return -1;
  }
  struct gp1_host host={
    .config=config,
    .rompath=config->posv[0],
  };
  void *src=0;
  int srcc=gp1_file_read(&src,config->posv[0]);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file\n",config->posv[0]);
    return -1;
  }
  uint16_t langv[16];
  int langc=gp1_get_user_languages(langv,16);
  host.rom=gp1_rom_new(src,srcc,langv,langc,config->posv[0]);
  free(src);
  if (!host.rom) return -1;
  int err=gp1_host_run(&host);
  gp1_rom_del(host.rom);
  host.rom=0;
  gp1_host_cleanup(&host);
  return err;
}

/* Pack ROM file.
 */
 
static int gp1_main_pack(struct gp1_config *config) {
  if (!config->dstpath) {
    fprintf(stderr,"%s: Please provide output path as '--out=PATH'\n",config->exename);
    return -1;
  }
  struct gp1_rompack rompack={
    .config=config,
  };
  int i=0; for (;i<config->posc;i++) {
    void *src=0;
    int srcc=gp1_file_read(&src,config->posv[i]);
    if (srcc<0) {
      fprintf(stderr,"%s: Failed to read file\n",config->posv[i]);
      gp1_rompack_cleanup(&rompack);
      return -1;
    }
    if (gp1_rompack_handoff_input(&rompack,src,srcc,config->posv[i])<0) {
      gp1_rompack_cleanup(&rompack);
      free(src);
      return -1;
    }
    // don't free src
  }
  void *dst=0;
  int dstc=gp1_rompack_generate_output(&dst,&rompack);
  gp1_rompack_cleanup(&rompack);
  if (dstc<0) return -1;
  int err=gp1_file_write(config->dstpath,dst,dstc);
  free(dst);
  if (err<0) {
    fprintf(stderr,"%s: Failed to write %d-byte file\n",config->dstpath,dstc);
    return -1;
  }
  return 0;
}

/* Convert chunk.
 */
 
static int gp1_main_convertchunk(struct gp1_config *config) {
  if (!config->dstpath) {
    fprintf(stderr,"%s: Please provide output path as '--out=PATH'\n",config->exename);
    return -1;
  }
  if (config->posc>1) {
    fprintf(stderr,"%s: Multiple input paths\n",config->exename);
    return -1;
  }
  if (!config->posc) {
    fprintf(stderr,"%s: No input path\n",config->exename);
    return -1;
  }
  void *src=0;
  int srcc=gp1_file_read(&src,config->posv[0]);
  if (srcc<0) {
    fprintf(stderr,"%s: Failed to read file\n",config->posv[0]);
    return -1;
  }
  void *dst=0;
  int dstc=gp1_convert_chunk(&dst,config,src,srcc,config->posv[0]);
  free(src);
  if (dstc<0) return -1;
  int err=gp1_file_write(config->dstpath,dst,dstc);
  free(dst);
  if (err<0) {
    fprintf(stderr,"%s: Failed to write %d-byte file\n",config->dstpath,dstc);
    return -1;
  }
  return 0;
}

/* File info.
 */
 
static int gp1_main_info(struct gp1_config *config) {
  int i=0; for (;i<config->posc;i++) {
    const char *path=config->posv[i];
    void *src=0;
    int srcc=gp1_file_read(&src,path);
    if (srcc<0) {
      fprintf(stderr,"%s: Failed to read file.\n",path);
      continue;
    }
    gp1_print_rom_info(config,src,srcc,path);
    free(src);
  }
  return 0;
}

/* Main, dispatch on command.
 */

int main(int argc,char **argv) {
  struct gp1_config config={0};
  if (gp1_config_init(&config,argc,argv)<0) return 1;
  int err=0;
  switch (config.command) {
    case GP1_COMMAND_help: gp1_config_print_help(&config); break;
    case GP1_COMMAND_run: err=gp1_main_run(&config); break;
    case GP1_COMMAND_pack: err=gp1_main_pack(&config); break;
    case GP1_COMMAND_convertchunk: err=gp1_main_convertchunk(&config); break;
    case GP1_COMMAND_info: err=gp1_main_info(&config); break;
    default: {
        fprintf(stderr,"%s: Unimplemented command %d (%s)\n",config.exename,config.command,gp1_command_repr(config.command));
        err=-1;
      }
  }
  gp1_config_cleanup(&config);
  return (err<0)?1:0;
}
