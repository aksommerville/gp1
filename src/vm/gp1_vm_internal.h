#ifndef GP1_VM_INTERNAL_H
#define GP1_VM_INTERNAL_H

#include "gp1_vm.h"
#include "gp1_rom.h"
#include "gp1/gp1.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

struct gp1_vm {
  struct gp1_vm_delegate delegate;
  int refc;
  uint16_t input_state[GP1_PLAYER_COUNT]; // including the fake aggregate
  struct gp1_rom *rom;
};

#endif
