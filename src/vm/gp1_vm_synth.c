#include "gp1_vm_internal.h"

/* Initialize synthesizer.
 */
 
int gp1_vm_init_synth(struct gp1_vm *vm,int rate,int chanc) {
  fprintf(stderr,"%s rate=%d chanc=%d\n",__func__,rate,chanc);
  return 0;
}

/* Synthesize audio.
 */
 
int gp1_vm_synthesize(int16_t *v,int c,struct gp1_vm *vm) {
  //TODO
  memset(v,0,c<<1);
  return 0;
}
