#include "gp1_vm_internal.h"

/* Deliver HTTP response.
 */
 
int gp1_vm_http_response(
  struct gp1_vm *vm,
  const struct gp1_http_request *request,
  const struct gp1_http_response *response
) {
  fprintf(stderr,"%s\n",__func__);
  return 0;
}

/* WebSocket connect status.
 */

int gp1_vm_ws_connect_fail(struct gp1_vm *vm,int wsid) {
  fprintf(stderr,"%s wsid=%d\n",__func__,wsid);
  return 0;
}

int gp1_vm_ws_connect_ok(struct gp1_vm *vm,int wsid) {
  fprintf(stderr,"%s wsid=%d\n",__func__,wsid);
  return 0;
}

/* WebSocket events from the remote end.
 */

int gp1_vm_ws_receive(struct gp1_vm *vm,int wsid,const void *src,int srcc) {
  fprintf(stderr,"%s wsid=%d srcc=%d\n",__func__,wsid,srcc);
  return 0;
}

int gp1_vm_ws_disconnect(struct gp1_vm *vm,int wsid) {
  fprintf(stderr,"%s wsid=%d\n",__func__,wsid);
  return 0;
}

/* Begin HTTP request, from Wasm side.
 */

int32_t gp1_http_request_wrapper(
  wasm_exec_env_t execenv,uint32_t methodr,uint32_t urlr,
  uint32_t bodyr,int32_t bodyc,void *userdata
) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  fprintf(stderr,"%s vm=%p methodr=0x%08x urlr=0x%08x bodyr=0x%08x bodyc=%d userdata=%p\n",__func__,vm,methodr,urlr,bodyr,bodyc,userdata);
  char *method=wasm_runtime_addr_app_to_native(vm->modinst,methodr);
  char *url=wasm_runtime_addr_app_to_native(vm->modinst,urlr);
  void *body=wasm_runtime_addr_app_to_native(vm->modinst,bodyr);
  if (
    !methodr||!urlr||(bodyc&&!bodyr)||
    !wasm_runtime_validate_app_str_addr(vm->modinst,methodr)||
    !wasm_runtime_validate_app_str_addr(vm->modinst,urlr)||
    !wasm_runtime_validate_native_addr(vm->modinst,body,bodyc)
  ) {
    fprintf(stderr,"Segfault at gp1_http_request (unclear which parameter)\n");
    vm->fatal=1;
    return -1;
  }
  //TODO validate against rom->rURL
  //TODO http request object
  fprintf(stderr,"%s '%s' '%s'\n",__func__,method,url);
  if (vm->delegate.http_request) {
    return vm->delegate.http_request(vm,0);
  }
  return -1;
}

/* Begin WebSocket connection, from Wasm.
 */

int32_t gp1_ws_connect_wrapper(
  wasm_exec_env_t execenv,uint32_t urlr,void *userdata
) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  fprintf(stderr,"%s vm=%p url=0x%08x userdata=%p\n",__func__,vm,urlr,userdata);
  char *url=wasm_runtime_addr_app_to_native(vm->modinst,urlr);
  if (!urlr||!wasm_runtime_validate_app_str_addr(vm->modinst,urlr)) {
    fprintf(stderr,"Segfault at gp1_ws_connect, 0x%08x\n",urlr);
    vm->fatal=1;
    return -1;
  }
  //TODO validate against rom->rURL
  int wsid=1;//TODO allocate wsid and register pending connection
  if (vm->delegate.ws_connect) {
    int urlc=strlen(url);
    if (vm->delegate.ws_connect(vm,wsid,url,urlc)<0) {
      //TODO fail connection
      return -1;
    }
  } else {
    //TODO fail connection
    return -1;
  }
  return 0;
}

/* Send data via WebSocket, from Wasm.
 */

int32_t gp1_ws_send_wrapper(
  wasm_exec_env_t execenv,int32_t wsid,uint32_t srcr,int32_t srcc
) {
  if (srcc<0) return -1;
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  fprintf(stderr,"%s vm=%p wsid=%d src=0x%08x srcc=%d\n",__func__,vm,wsid,srcr,srcc);
  void *src=wasm_runtime_addr_app_to_native(vm->modinst,srcr);
  if (!srcr||!wasm_runtime_validate_native_addr(vm->modinst,src,srcc)) {
    fprintf(stderr,"Segfault at gp1_ws_send, %d@0x%08x\n",srcc,srcr);
    vm->fatal=1;
    return -1;
  }
  //TODO look up connection by wsid
  if (vm->delegate.ws_send) {
    if (vm->delegate.ws_send(vm,wsid,src,srcc)<0) {
      return -1;
    }
  } else {
    return -1;
  }
  return 0;
}

/* Disconnect WebSocket, from Wasm.
 */

void gp1_ws_disconnect_wrapper(wasm_exec_env_t execenv,int32_t wsid) {
  struct gp1_vm *vm=wasm_runtime_get_custom_data(wasm_runtime_get_module_inst(execenv));
  fprintf(stderr,"%s vm=%p wsid=%d\n",__func__,vm,wsid);
  //TODO look up connection by wsid
}
