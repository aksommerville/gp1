# commands.mk

clean:;rm -rf mid out

all:$(EXES) $(LIBS) $(ROMS)

test:$(EXE_itest) $(EXES_UTEST);etc/tool/runtests.sh $^
test-%:$(EXE_itest) $(EXES_UTEST);GP1_TEST_FILTER="$*" etc/tool/runtests.sh $^

run:$(EXE_gp1) $(ROM_lightson);$(EXE_gp1) run $(ROM_lightson)
run-%:$(EXE_gp1) $(ROMS);$(EXE_gp1) run $(OUTDIR)/example/$*.gp1
help:$(EXE_gp1);$(EXE_gp1) help

install:;echo "TODO make $@" ; exit 1
