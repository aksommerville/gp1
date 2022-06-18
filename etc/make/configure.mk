# configure.mk
#TODO named configurations

# XXX These are my installation locations. Can we not hard-code this with the shared rules?
WASI_SDK:=/home/andy/proj/thirdparty/wasi-sdk/wasi-sdk-16.0

MIDDIR:=mid
OUTDIR:=out

OPT_ENABLE:=alsa pulse glx drm evdev gl2

CCWARN:=-Werror -Wimplicit -Wno-overflow
CCINC:=-Isrc -I$(MIDDIR) -I/usr/include/libdrm
CCOPT:=-c -MMD -O3 $(foreach U,$(OPT_ENABLE),-DGP1_USE_$U=1)
CC:=gcc $(CCOPT) $(CCWARN) $(CCINC)

LD:=gcc
LDPOST:=-lvmlib -liwasm -lm -lz -lpthread -lasound -lpulse -lpulse-simple -lGLESv2 -lGLX -lX11 -ldrm -lgbm -lEGL

AR:=ar rc

CC_WASI:=$(WASI_SDK)/bin/clang -c -MMD -O3 -Isrc -I$(MIDDIR)
#TODO --no-entry is probably wrong

LD_WASI:=$(WASI_SDK)/bin/wasm-ld --no-entry --allow-undefined \
  --export=gp1_init \
  --export=gp1_update \
  --export-if-defined=gp1_http_response \
  --export-if-defined=gp1_ws_connect_complete \
  --export-if-defined=gp1_ws_receive \
  --export-if-defined=gp1_ws_disconnected

LDPOST_WASI:=
