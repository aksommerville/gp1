# build.mk

#----------------------------------------------------------------
# Generated source files, TOC only.

GENFILES:=$(addprefix $(MIDDIR)/, \
  test/int/gp1_itest_toc.h \
)

#------------------------------------------------------------------
# Discover sources, default C compilation.

OPT_AVAILABLE:=$(notdir $(wildcard src/opt/*))
OPT_IGNORE:=$(filter-out $(OPT_ENABLE),$(OPT_AVAILABLE))
OPT_IGNORE_PATTERN:= \
  $(foreach PFX,src/opt/ $(MIDDIR)/opt/ src/test/int/opt/ src/test/unit/opt/, \
    $(addsuffix /%,$(addprefix $(PFX),$(OPT_IGNORE))) \
  )

SRCFILES:=$(filter-out $(OPT_IGNORE_PATTERN),$(shell find src -type f) $(GENFILES))

CFILES:=$(filter %.c,$(SRCFILES))
GENHFILES:=$(filter %.h,$(GENFILES))

OFILES_ALL:=$(patsubst src/%,$(MIDDIR)/%,$(addsuffix .o,$(basename $(CFILES))))

-include $(OFILES_ALL:.o=.d)

$(MIDDIR)/%.o:src/%.c|$(GENHFILES);$(PRECMD) $(CC) -o $@ $<
$(MIDDIR)/%.o:$(MIDDIR)/%.c|$(GENHFILES);$(PRECMD) $(CC) -o $@ $<

$(MIDDIR)/example/%.o:src/example/%.c;$(PRECMD) $(CC_WASI) -o $@ $<
$(MIDDIR)/example/%.o:$(MIDDIR)/example/%.c;$(PRECMD) $(CC_WASI) -o $@ $<

#----------------------------------------------------------------
# Executables.

define EXE_RULES # $1=name $2=directories
  EXE_$1:=$(OUTDIR)/$1
  OFILES_EXE_$1:=$(filter $(addprefix $(MIDDIR)/,$(addsuffix /%,$2)),$(OFILES_ALL))
  $$(EXE_$1):$$(OFILES_EXE_$1);$$(PRECMD) $(LD) -o $$@ $$^ $(LDPOST)
  EXES+=$$(EXE_$1)
endef

EXES:=
$(eval $(call EXE_RULES,gp1,main io vm opt))
$(eval $(call EXE_RULES,itest,test/int test/common io vm opt))

#----------------------------------------------------------------
# Static libraries.

define LIB_RULES # $1=name $2=directories
  LIB_$1:=$(OUTDIR)/lib$1.a
  OFILES_LIB_$1:=$(filter $(addprefix $(MIDDIR)/,$(addsuffix /%,$2)),$(OFILES_ALL))
  $$(LIB_$1):$$(OFILES_LIB_$1);$$(PRECMD) $(AR) $$@ $$^
  LIBS+=$$(LIB_$1)
endef

LIBS:=
$(eval $(call LIB_RULES,gp1vm,vm))
$(eval $(call LIB_RULES,gp1io,io))

#---------------------------------------------------------------
# Unit tests. These are executables but follow a tighter pattern than the general case above.

OFILES_UTEST:=$(filter $(MIDDIR)/test/unit/%.o,$(OFILES_ALL))
OFILES_CTEST:=$(filter $(MIDDIR)/test/common/%.o,$(OFILES_ALL))
EXES_UTEST:=$(patsubst $(MIDDIR)/test/unit/%.o,$(OUTDIR)/utest/%,$(OFILES_UTEST))
$(OUTDIR)/utest/%:$(MIDDIR)/test/unit/%.o $(OFILES_CTEST);$(PRECMD) $(LD) -o $@ $^ $(LDPOST)
EXES+=$(EXES_UTEST)

#----------------------------------------------------------------
# Examples.

EXAMPLE_DATA_CHUNK_PATTERN:=%.png %.gif %.bmp %.jpeg %.img %.mid %.aucfg %.data %.bin
EXAMPLE_DATA_VERBATIM_PATTERN=%.meta %.strings

define EXAMPLE_RULES # $1=name
  ROM_$1:=$(OUTDIR)/example/$1.gp1
  OFILES_ROM_$1:=$(filter $(MIDDIR)/example/$1/%.o,$(OFILES_ALL))
  WASI_ROM_$1:=$(MIDDIR)/example/$1/$1.wasm
  $$(WASI_ROM_$1):$$(OFILES_ROM_$1);$$(PRECMD) $(LD_WASI) -o $$@ $$^ $(LDPOST_WASI)
  DATASRC_CHUNK_ROM_$1:=$(filter $(EXAMPLE_DATA_CHUNK_PATTERN),$(filter src/example/$1/%,$(SRCFILES)))
  CHUNKS_ROM_$1:=$$(patsubst src/example/$1/%,$(MIDDIR)/example/$1/%.chunk,$$(DATASRC_CHUNK_ROM_$1))
  DATASRC_VERBATIM_ROM_$1:=$(filter $(EXAMPLE_DATA_VERBATIM_PATTERN),$(filter src/example/$1/%,$(SRCFILES)))
  VERBATIMS_ROM_$1:=$$(patsubst src/%,$(MIDDIR)/%,$$(DATASRC_VERBATIM_ROM_$1))
  INPUTS_ROM_$1:=$$(WASI_ROM_$1) $$(CHUNKS_ROM_$1) $$(VERBATIMS_ROM_$1)
  $$(ROM_$1):$$(INPUTS_ROM_$1) $(EXE_gp1);$$(PRECMD) $(EXE_gp1) pack --out=$$@ $$(INPUTS_ROM_$1)
  ROMS+=$$(ROM_$1)
endef

ROMS:=
EXAMPLES:=$(notdir $(wildcard src/example/*))
$(foreach E,$(EXAMPLES),$(eval $(call EXAMPLE_RULES,$E)))

$(MIDDIR)/example/%.chunk:src/example/% $(EXE_gp1);$(PRECMD) $(EXE_gp1) convertchunk --out=$@ $<
$(MIDDIR)/example/%:src/example/%;$(PRECMD) cp $< $@

#----------------------------------------------------------------
# Rules for generating source files.
# Anything that depends on a generated header, call it out here.

$(MIDDIR)/test/int/gp1_itest_toc.h:etc/tool/genitesttoc.sh $(filter src/test/int/%.c,$(CFILES));$(PRECMD) $^ $@
$(MIDDIR)/test/int/gp1_itest_main.c:$(MIDDIR)/test/int/gp1_itest_toc.h
