all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $(@F)" ; mkdir -p $(@D) ;

ifneq ($(MAKECMDGOALS),clean)
  include etc/make/configure.mk
  include etc/make/build.mk
endif
include etc/make/commands.mk
