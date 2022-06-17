#include "gp1_vm_internal.h"

/* TOC, native functions exposed to wasm code.
 */
 
static NativeSymbol gp1_vm_native_symbols[]={
  EXPORT_WASM_API2(gp1_get_input_state),
  EXPORT_WASM_API2(gp1_video_send),
  EXPORT_WASM_API2(gp1_audio_send),
  EXPORT_WASM_API2(gp1_store),
  EXPORT_WASM_API2(gp1_load),
  EXPORT_WASM_API2(gp1_get_string),
  EXPORT_WASM_API2(gp1_get_data),
  EXPORT_WASM_API2(gp1_http_request),
  EXPORT_WASM_API2(gp1_ws_connect),
  EXPORT_WASM_API2(gp1_ws_send),
  EXPORT_WASM_API2(gp1_ws_disconnect),
};

/* Initialize game.
 */

int gp1_vm_init_game(struct gp1_vm *vm) {
  gp1_vm_drop_game(vm);
  if (!vm->rom) return -1;
  
  RuntimeInitArgs runtimeInitArgs={
    .mem_alloc_type=Alloc_With_System_Allocator,
    .mem_alloc_option={0},
    .native_module_name="env",
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
  //TODO here we get warnings on stderr if a client function fails to link -- can we make those hard errors?
  // wasm_runtime.c:1104, looks like no, all it does is log.
  // wasm_module_imports() can show us the import list, but it doesn't say which are linked.
  // (which, i think it can't know that, that's the module and not the module instance).
  if (!(vm->modinst=wasm_runtime_instantiate(vm->mod,stack_size,heap_size,msg,sizeof(msg)))) {
    fprintf(stderr,"wasm_runtime_instantiate: %s\n",msg);
    return -1;
  }
  wasm_runtime_set_custom_data(vm->modinst,vm);
  
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
  // The remaining functions are optional, no worries if they come back null.
  vm->fn_gp1_http_response=wasm_runtime_lookup_function(vm->modinst,"gp1_http_response","");
  vm->fn_gp1_ws_connect_complete=wasm_runtime_lookup_function(vm->modinst,"gp1_ws_connect_complete","");
  vm->fn_gp1_ws_receive=wasm_runtime_lookup_function(vm->modinst,"gp1_ws_receive","");
  vm->fn_gp1_ws_disconnected=wasm_runtime_lookup_function(vm->modinst,"gp1_ws_disconnected","");
  
  uint32_t argv[2]={0}; // no params, but this is also the return vector
  if (wasm_runtime_call_wasm(vm->execenv,fn_gp1_init,2,argv)) {
    if (vm->fatal) return -1;
    int32_t result=argv[0];
    if (result<0) {
      fprintf(stderr,"gp1_init client failure: %d\n",result);
      return -1;
    } else {
      fprintf(stderr,"gp1_init ok: %d\n",result);
    }
  } else {
    const char *ex=wasm_runtime_get_exception(vm->modinst);
    fprintf(stderr,"gp1_init failed: %s\n",ex);
    return -1;
  }
  
  return 0;
}

/* Update game.
 */

int gp1_vm_update(struct gp1_vm *vm) {
  if (!vm->fn_gp1_update) return -1;
  if (vm->fatal) return -1;
  
  uint32_t argv[2]={0};
  if (wasm_runtime_call_wasm(vm->execenv,vm->fn_gp1_update,2,argv)) {
    int32_t result=argv[0];
    if (result<0) {
      fprintf(stderr,"gp1_update client failure: %d\n",result);
      return -1;
    } else {
    }
  } else {
    const char *ex=wasm_runtime_get_exception(vm->modinst);
    fprintf(stderr,"gp1_update failed: %s\n",ex);
    return -1;
  }
  
  if (vm->fatal) return -1;
  return 0;
}
