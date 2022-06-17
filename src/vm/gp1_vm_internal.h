#ifndef GP1_VM_INTERNAL_H
#define GP1_VM_INTERNAL_H

#include "gp1_vm.h"
#include "gp1_rom.h"
#include "gp1/gp1.h"
#include <wasm_c_api.h>
#include <wasm_export.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

struct gp1_vm {
  struct gp1_vm_delegate delegate;
  int refc;
  uint16_t input_state[GP1_PLAYER_COUNT]; // including the fake aggregate
  struct gp1_rom *rom;
  
  wasm_module_t mod;
  wasm_module_inst_t modinst;
  wasm_exec_env_t execenv;
  wasm_function_inst_t fn_gp1_update;
};

// Keeps (rom), eliminates all live VM stuff.
void gp1_vm_drop_game(struct gp1_vm *vm);

#endif
