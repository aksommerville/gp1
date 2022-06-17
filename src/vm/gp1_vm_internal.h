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
  wasm_function_inst_t fn_gp1_update; // required, if running.
  wasm_function_inst_t fn_gp1_http_response; // optional
  wasm_function_inst_t fn_gp1_ws_connect_complete; // optional
  wasm_function_inst_t fn_gp1_ws_receive; // optional
  wasm_function_inst_t fn_gp1_ws_disconnected; // optional
  int fatal;
};

// Keeps (rom), eliminates all live VM stuff.
void gp1_vm_drop_game(struct gp1_vm *vm);

int gp1_vm_receive_audio_commands(struct gp1_vm *vm,const void *src,int srcc);

// gp1_vm_intf.c 
uint16_t gp1_get_input_state_wrapper(wasm_exec_env_t execenv,int32_t playerid);
void gp1_video_send_wrapper(wasm_exec_env_t execenv,uint32_t srcr,int32_t srcc);
void gp1_audio_send_wrapper(wasm_exec_env_t execenv,uint32_t srcr,int32_t srcc);
int32_t gp1_store_wrapper(wasm_exec_env_t execenv,uint32_t k,uint32_t srcr,int32_t c);
int32_t gp1_load_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t a,uint32_t k);
int32_t gp1_get_string_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t dsta,uint32_t id);
int32_t gp1_get_data_wrapper(wasm_exec_env_t execenv,uint32_t dstr,int32_t dsta,uint32_t id);

// gp1_vm_net.c
int32_t gp1_http_request_wrapper(
  wasm_exec_env_t execenv,uint32_t methodr,uint32_t urlr,
  uint32_t bodyr,int32_t bodyc,void *userdata
);
int32_t gp1_ws_connect_wrapper(wasm_exec_env_t execenv,uint32_t urlr,void *userdata);
int32_t gp1_ws_send_wrapper(wasm_exec_env_t execenv,int32_t wsid,uint32_t srcr,int32_t srcc);
void gp1_ws_disconnect_wrapper(wasm_exec_env_t execenv,int32_t wsid);

#endif
