#include "gp1_vm_internal.h"

/* Initialize game.
 */
 
static NativeSymbol gp1_vm_native_symbols[]={
};

int gp1_vm_init_game(struct gp1_vm *vm) {
  gp1_vm_drop_game(vm);
  if (!vm->rom) return -1;
  
  RuntimeInitArgs runtimeInitArgs={
    .mem_alloc_type=Alloc_With_System_Allocator,
    .mem_alloc_option={0},
    .native_module_name="native",
    .native_symbols=gp1_vm_native_symbols,
    .n_native_symbols=sizeof(gp1_vm_native_symbols)/sizeof(NativeSymbol),
    .max_thread_num=0,
    .ip_addr={0},
    .platform_port=0,
    .instance_port=0,
  };
  if (!wasm_runtime_full_init(&runtimeInitArgs)) {
    fprintf(stderr,"wasm_runtime_full_init failed\n");
    return -1;
  }
  
  char msg[256]={0};
  if (!(vm->mod=wasm_runtime_load(vm->rom->exec,vm->rom->execc,msg,sizeof(msg)))) {
    fprintf(stderr,"wasm_runtime_load: %s\n",msg);
    return -1;
  }
  
  uint32_t stack_size=1<<20;//TODO get organized about available memory
  uint32_t heap_size=8<<20;
  if (!(vm->modinst=wasm_runtime_instantiate(vm->mod,stack_size,heap_size,msg,sizeof(msg)))) {
    fprintf(stderr,"wasm_runtime_instantiate: %s\n",msg);
    return -1;
  }
  
  if (!(vm->execenv=wasm_runtime_create_exec_env(vm->modinst,stack_size))) {
    fprintf(stderr,"wasm_runtime_create_exec_env failed\n");
    return -1;
  }
  
  wasm_function_inst_t fn_gp1_init=wasm_runtime_lookup_function(vm->modinst,"gp1_init","");
  vm->fn_gp1_update=wasm_runtime_lookup_function(vm->modinst,"gp1_update","");
  if (!fn_gp1_init) {
    fprintf(stderr,"Required entry point 'gp1_init' not found. This ROM file is not playable by GP1.\n");
    return -1;
  }
  if (!vm->fn_gp1_update) {
    fprintf(stderr,"Required entry point 'gp1_update' not found. This ROM file is not playable by GP1.\n");
    return -1;
  }
  
  uint32_t argv[2]={0}; // no params, but this is also the return vector
  if (wasm_runtime_call_wasm(vm->execenv,fn_gp1_init,2,argv)) {
    int32_t result=argv[0];
    if (result<0) {
      fprintf(stderr,"gp1_init client failure: %d\n",result);
      return -1;
    } else {
      fprintf(stderr,"gp1_init ok: %d\n",result);
    }
  } else {
    fprintf(stderr,"gp1_init failed to execute\n");
    return -1;
  }
  
  return 0;
}

/* Update game.
 */

int gp1_vm_update(struct gp1_vm *vm) {
  if (!vm->fn_gp1_update) return -1;
  
  uint32_t argv[2]={0};
  if (wasm_runtime_call_wasm(vm->execenv,vm->fn_gp1_update,2,argv)) {
    int32_t result=argv[0];
    if (result<0) {
      fprintf(stderr,"gp1_update client failure: %d\n",result);
      return -1;
    } else {
    }
  } else {
    fprintf(stderr,"gp1_update failed\n");
    return -1;
  }
  
  return 0;
}
