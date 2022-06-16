#include "test/int/gp1_itest_toc.h"
#include "test/gp1_test.h"

int main(int argc,char **argv) {
  int i=sizeof(gp1_itestv)/sizeof(struct gp1_itest);
  const struct gp1_itest *itest=gp1_itestv;
  for (;i-->0;itest++) {
    if (!gp1_test_filter(argc,argv,itest->name,itest->tags,itest->file,itest->enable)) {
      fprintf(stderr,"GP1_TEST SKIP %s [%s:%d]\n",itest->name,itest->file,itest->line);
    } else if (itest->fn()<0) {
      fprintf(stderr,"GP1_TEST FAIL %s [%s:%d]\n",itest->name,itest->file,itest->line);
    } else {
      fprintf(stderr,"GP1_TEST PASS %s [%s:%d]\n",itest->name,itest->file,itest->line);
    }
  }
  return 0;
}
