#include "gp1_io_clock.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* Current time.
 */

double gp1_now_s() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return tv.tv_sec+tv.tv_usec/1000000.0;
}

int64_t gp1_now_us() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

/* Current CPU time.
 * TODO clock_gettime() is a Linux thing, what is the equivalent for MacOS and Windows?
 */
 
double gp1_now_cpu_s() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return tv.tv_sec+tv.tv_nsec/1000000000.0;
}

int64_t gp1_now_cpu_us() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (int64_t)tv.tv_sec*1000000+tv.tv_nsec/1000;
}

/* Sleep.
 */

void gp1_sleep(int us) {
  usleep(us);
}
