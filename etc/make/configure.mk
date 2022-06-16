# configure.mk
#TODO named configurations

# XXX These are my installation locations. Can we not hard-code this with the shared rules?
WASI_SDK:=/home/andy/proj/thirdparty/wasi-sdk/wasi-sdk-16.0

MIDDIR:=mid
OUTDIR:=out

OPT_ENABLE:=alsa pulse glx drm evdev

CCWARN:=-Werror -Wimplicit -Wno-overflow
CCINC:=-Isrc -I$(MIDDIR) -I/usr/include/libdrm
CCOPT:=-c -MMD -O3 $(foreach U,$(OPT_ENABLE),-DGP1_USE_$U=1)
CC:=gcc $(CCOPT) $(CCWARN) $(CCINC)

LD:=gcc
LDPOST:=-lm -lvmlib -liwasm -lz -lpthread -lasound -lpulse -lpulse-simple -lGLESv2 -lGLX -lX11 -ldrm -lgbm -lEGL

AR:=ar rc

CC_WASI:=$(WASI_SDK)/bin/clang -c -MMD -O3 -Isrc -I$(MIDDIR)
#TODO --no-entry is probably wrong
LD_WASI:=$(WASI_SDK)/bin/wasm-ld --no-entry
LDPOST_WASI:=
