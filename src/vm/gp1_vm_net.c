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
