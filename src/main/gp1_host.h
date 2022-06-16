/* gp1_host.h
 * Our main executable, the interactive runtime. (ie everything that isn't a command-line tool).
 */
 
#ifndef GP1_HOST_H
#define GP1_HOST_H

struct gp1_config;
struct gp1_rom;

struct gp1_host {
// Caller sets directly, all WEAK:
  struct gp1_config *config;
  const char *rompath;
  struct gp1_rom *rom;
};

void gp1_host_cleanup(struct gp1_host *host);

// Modal, runs the whole thing. We log errors.
int gp1_host_run(struct gp1_host *host);

#endif
