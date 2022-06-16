#include "gp1_io_fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef O_BINARY
  #define O_BINARY 0
#endif

/* Read entire file.
 */
 
int gp1_file_read(void *dstpp,const char *path) {
  if (!dstpp||!path) return -1;
  int fd=open(path,O_RDONLY|O_BINARY);
  if (fd<0) return -1;
  off_t flen=lseek(fd,0,SEEK_END);
  if ((flen<0)||(flen>INT_MAX)||lseek(fd,0,SEEK_SET)) {
    close(fd);
    return -1;
  }
  char *dst=malloc(flen);
  if (!dst) {
    close(fd);
    return -1;
  }
  int dstc=0;
  while (dstc<flen) {
    int err=read(fd,dst+dstc,flen-dstc);
    if (err<=0) {
      close(fd);
      free(dst);
      return -1;
    }
    dstc+=err;
  }
  close(fd);
  *(void**)dstpp=dst;
  return dstc;
}

/* Write entire file.
 */
 
int gp1_file_write(const char *path,const void *src,int srcc) {
  if (!path||(srcc<0)||(srcc&&!src)) return -1;
  int fd=open(path,O_WRONLY|O_CREAT|O_TRUNC,0666);
  if (fd<0) return -1;
  int srcp=0;
  while (srcp<srcc) {
    int err=write(fd,(char*)src+srcp,srcc-srcp);
    if (err<=0) {
      close(fd);
      unlink(path);
      return -1;
    }
    srcp+=err;
  }
  close(fd);
  return 0;
}

/* Iterate directory.
 */
 
int gp1_dir_read(
  const char *path,
  int (*cb)(const char *path,const char *base,char type,void *userdata),
  void *userdata
) {
  DIR *dir=opendir(path);
  if (!dir) return -1;
  
  char subpath[1024];
  int pathc=0;
  while (path[pathc]) pathc++;
  if (pathc>=sizeof(subpath)) {
    closedir(dir);
    return -1;
  }
  memcpy(subpath,path,pathc);
  if (pathc&&(path[pathc-1]!='/')) subpath[pathc++]='/';
  
  struct dirent *de;
  while (de=readdir(dir)) {
    const char *base=de->d_name;
    int basec=0;
    while (base[basec]) basec++;
    if (basec<1) continue;
    if ((basec==1)&&(base[0]=='.')) continue;
    if ((basec==2)&&(base[0]=='.')&&(base[1]=='.')) continue;
    
    if (pathc>=sizeof(subpath)-basec) {
      closedir(dir);
      return -1;
    }
    
    char type='?';
    switch (de->d_type) {
      case DT_REG: type='f'; break;
      case DT_DIR: type='d'; break;
      case DT_CHR: type='c'; break;
      case DT_BLK: type='b'; break;
      case DT_SOCK: type='s'; break;
      case DT_LNK: type='l'; break;
    }
    
    int err=cb(subpath,base,type,userdata);
    if (err) {
      closedir(dir);
      return err;
    }
  }
  closedir(dir);
  return 0;
}

/* File type.
 */

char gp1_file_get_type(const char *path) {
  if (!path||!path[0]) return 0;
  struct stat st={0};
  if (stat(path,&st)<0) return 0;
  if (S_ISREG(st.st_mode)) return 'f';
  if (S_ISDIR(st.st_mode)) return 'd';
  if (S_ISCHR(st.st_mode)) return 'c';
  if (S_ISBLK(st.st_mode)) return 'b';
  if (S_ISSOCK(st.st_mode)) return 's';
  if (S_ISLNK(st.st_mode)) return 'l';
  return '?';
}
