/* gp1_io_os.h
 */
 
#ifndef GP1_IO_OS_H
#define GP1_IO_OS_H

#include <stdint.h>

//TODO config paths, locale, ...

/* Return a list of big-endian ISO 631 language codes in the user's preferred order.
 * Never returns more than (a) results. I suggest 16 as a sensible upper limit.
 */
int gp1_get_user_languages(uint16_t *v,int a);

#endif
