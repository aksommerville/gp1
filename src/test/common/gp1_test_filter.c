#include "test/gp1_test.h"

/* Find strings in lists.
 */
 
static int gp1_string_in_list(const char *q,int qc,char **v,int c) {
  if (!q) return 0;
  if (qc<0) { qc=0; while (q[qc]) qc++; }
  for (;c-->0;v++) {
    if (memcmp(q,*v,qc)) continue;
    if ((*v)[qc]) continue;
    return 1;
  }
  return 0;
}

static int gp1_list_in_list(const char *src,char **v,int c) {
  if (!src) return 0;
  while (*src) {
    if ((unsigned char)(*src)<=0x20) { src++; continue; }
    if (*src==',') { src++; continue; }
    const char *q=src;
    int qc=0;
    while (*src&&(*src!=',')) { src++; qc++; }
    while (qc&&((unsigned char)q[qc-1]<=0x20)) qc++;
    if (!qc) continue;
    if (gp1_string_in_list(q,qc,v,c)) return 1;
  }
  return 0;
}

/* Apply test filter.
 */
 
int gp1_test_filter(
  int argc,char **argv, // exactly as provided to main()
  const char *name, // name of test, ie function name
  const char *tags, // comma-delimited tags belonging to this test
  const char *file, // path or basename of file containing test
  int enable // test's default disposition (args may override)
) {
  argc--; argv++;
  
  // No filter args from the command line, use the test's default disposition.
  if (argc<1) return enable;
  
  if (name&&gp1_string_in_list(name,-1,argv,argc)) return 1;
  if (file) {
    const char *base=file;
    int basec=0;
    int i=0; for (;file[i];i++) {
      if (file[i]=='/') {
        base=file+i+1;
        basec=0;
      } else basec++;
    }
    if (gp1_string_in_list(base,basec,argv,argc)) return 1;
  }
  if (gp1_list_in_list(tags,argv,argc)) return 1;
  
  return 0;
}
