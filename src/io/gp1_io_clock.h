/* gp1_io_clock.h
 */
 
#ifndef GP1_IO_CLOCK_H
#define GP1_IO_CLOCK_H

#include <stdint.h>

double gp1_now_s();
double gp1_now_cpu_s();
int64_t gp1_now_us();
int64_t gp1_now_cpu_us();

void gp1_sleep(int us);

#endif
