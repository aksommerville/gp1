#include "gp1_io_os.h"

/* Get languages.
 */
 
int gp1_get_user_languages(uint16_t *v,int a) {
  //TODO get languages, os-dependent
  if (a<1) return 0;
  v[0]=('e'<<8)|'n';
  return 1;
}
