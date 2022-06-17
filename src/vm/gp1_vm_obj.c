#include "gp1_vm_internal.h"

/* Delete.
 */
 
void gp1_vm_drop_game(struct gp1_vm *vm) {
  memset(vm->input_state,0,sizeof(vm->input_state));
  if (vm->execenv) {
    wasm_runtime_destroy_exec_env(vm->execenv);
    vm->execenv=0;
  }
  if (vm->modinst) {
    wasm_runtime_deinstantiate(vm->modinst);
    vm->modinst=0;
  }
  if (vm->mod) {
    wasm_runtime_unload(vm->mod);
    vm->mod=0;
  }
  wasm_runtime_destroy();
}

void gp1_vm_del(struct gp1_vm *vm) {
  if (!vm) return;
  if (vm->refc-->1) return;
  
  gp1_vm_drop_game(vm);
  
  gp1_rom_del(vm->rom);
  
  free(vm);
}

/* Retain.
 */
  
int gp1_vm_ref(struct gp1_vm *vm) {
  if (!vm) return -1;
  if (vm->refc<1) return -1;
  if (vm->refc==INT_MAX) return -1;
  vm->refc++;
  return 0;
}

/* New.
 */

struct gp1_vm *gp1_vm_new(const struct gp1_vm_delegate *delegate) {
  struct gp1_vm *vm=calloc(1,sizeof(struct gp1_vm));
  if (!vm) return 0;
  vm->refc=1;
  if (delegate) vm->delegate=*delegate;
  return vm;
}

/* Trivial accessors.
 */

void *gp1_vm_get_userdata(const struct gp1_vm *vm) {
  if (!vm) return 0;
  return vm->delegate.userdata;
}

int gp1_vm_load_rom(struct gp1_vm *vm,struct gp1_rom *rom) {
  if (!vm||!rom) return -1;
  if (gp1_rom_ref(rom)<0) return -1;
  gp1_rom_del(vm->rom);
  vm->rom=rom;
  return 0;
}

/* Set input button.
 */

int gp1_vm_input_event(struct gp1_vm *vm,int playerid,int btnid,int value) {
  if (playerid<0) return 0;
  if (playerid>=GP1_PLAYER_COUNT) return 0;
  if (value) {
    vm->input_state[playerid]|=btnid;
    if (playerid) {
      vm->input_state[0]|=value;
    }
  } else {
    vm->input_state[playerid]&=~btnid;
    if (playerid) {
      int avalue=0,i=GP1_PLAYER_COUNT;
      while (i-->GP1_PLAYER_FIRST) {
        if (vm->input_state[playerid]&btnid) {
          avalue=1;
          break;
        }
      }
      if (avalue) {
        vm->input_state[0]|=btnid;
      } else {
        vm->input_state[0]&=~btnid;
      }
    }
  }
  return 0;
}
