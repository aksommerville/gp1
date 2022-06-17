#include "gp1_vm_internal.h"
 
uint16_t gp1_get_input_state_wrapper(wasm_exec_env_t execenv,int32_t playerid) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  if (playerid<0) return 0;
  if (playerid>=GP1_PLAYER_COUNT) return 0;
  return vm->input_state[playerid];
}

void gp1_video_send_wrapper(wasm_exec_env_t execenv,uint32_t srcr,int32_t srcc) {
  if (srcc<1) return;
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  void *src=wasm_runtime_addr_app_to_native(vm->modinst,srcr);
  if (!srcr||!wasm_runtime_validate_native_addr(vm->modinst,src,srcc)) {
    // Wasm runtime generates an exception on this, but we can give better detail:
    fprintf(stderr,"Segfault at gp1_video_send, %d@0x%08x\n",srcc,srcr);
    vm->fatal=1;
    return;
  }
  if (vm->delegate.render) {
    if (vm->delegate.render(vm,src,srcc)<0) {
      vm->fatal=1;
    }
  }
}

void gp1_audio_send_wrapper(wasm_exec_env_t execenv,uint32_t srcr,int32_t srcc) {
  if (srcc<1) return;
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  void *src=wasm_runtime_addr_app_to_native(vm->modinst,srcr);
  if (!srcr||!wasm_runtime_validate_native_addr(vm->modinst,src,srcc)) {
    fprintf(stderr,"Segfault at gp1_audio_send, %d@0x%08x\n",srcc,srcr);
    vm->fatal=1;
    return;
  }
  gp1_vm_receive_audio_commands(vm,src,srcc);
}

int32_t gp1_store_wrapper(wasm_exec_env_t execenv,uint32_t k,uint32_t srcr,int32_t c) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  //TODO validate against sTOR
  void *src=wasm_runtime_addr_app_to_native(vm->modinst,srcr);
  if (!srcr||!wasm_runtime_validate_native_addr(vm->modinst,src,c)) {
    fprintf(stderr,"Segfault at gp1_store, %d@0x%08x\n",c,srcr);
    vm->fatal=1;
    return -1;
  }
  if (vm->delegate.store) {
    return vm->delegate.store(vm,k,src,c);
  }
  return 0;
}

int32_t gp1_load_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t a,uint32_t k) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  //TODO validate against sTOR
  void *dst=wasm_runtime_addr_app_to_native(vm->modinst,dstr);
  if (!dstr||!wasm_runtime_validate_native_addr(vm->modinst,dst,a)) {
    fprintf(stderr,"Segfault at gp1_load, %d@0x%08x\n",a,dstr);
    vm->fatal=1;
    return -1;
  }
  if (vm->delegate.load) {
    return vm->delegate.load(vm,dst,a,k);
  }
  return 0;
}

int32_t gp1_get_string_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t dsta,uint32_t id) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  void *dst=wasm_runtime_addr_app_to_native(vm->modinst,dstr);
  if (!dstr||!wasm_runtime_validate_native_addr(vm->modinst,dst,dsta)) {
    fprintf(stderr,"Segfault at gp1_get_string[%d], %d@0x%08x\n",id,dsta,dstr);
    vm->fatal=1;
    return 0;
  }
  const void *src=0;
  int srcc=gp1_rom_string_get(&src,vm->rom,id);
  if (srcc<0) srcc=0;
  if (srcc<=dsta) memcpy(dst,src,srcc);
  if (srcc<dsta) ((char*)dst)[srcc]=0;
  return srcc;
}

int32_t gp1_get_data_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t dsta,uint32_t id) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  void *dst=wasm_runtime_addr_app_to_native(vm->modinst,dstr);
  if (!dstr||!wasm_runtime_validate_native_addr(vm->modinst,dst,dsta)) {
    fprintf(stderr,"Segfault at gp1_get_data[%d], %d@0x%08x\n",id,dsta,dstr);
    vm->fatal=1;
    return 0;
  }
  const void *src=0;
  int srcc=gp1_rom_data_get(&src,vm->rom,id);
  if (srcc<0) srcc=0;
  if (srcc<=dsta) memcpy(dst,src,srcc);
  if (srcc<dsta) ((char*)dst)[srcc]=0;
  return srcc;
}
