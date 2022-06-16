/* gp1_io_fs.h
 * Wrapper around basic filesystem functions.
 */
 
#ifndef GP1_IO_FS_H
#define GP1_IO_FS_H

int gp1_file_read(void *dstpp,const char *path);
int gp1_file_write(const char *path,const void *src,int srcc);

int gp1_dir_read(
  const char *path,
  int (*cb)(const char *path,const char *base,char type,void *userdata),
  void *userdata
);

char gp1_file_get_type(const char *path);

#endif
