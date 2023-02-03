# path to the emacs source dir
# (you can provide it here or on the command line)
#ROOT=
EMACS   = emacs
BATCH   = $(EMACS) -batch -Q -L . -L tests
CC      = gcc
LD      = gcc
CFLAGS  = -ggdb3 -Wall
LDFLAGS = -I wasm3/source/ -L wasm3/source/ -I wasm3/build/source/ -L wasm3/build/source/ -lm3
LDLIBS  = $(shell $(EMACS) --batch --eval "(if (eq system-type 'windows-nt) (princ \"-lxinput1_3\"))")
SUFFIX  = $(shell $(EMACS) --batch --eval '(princ module-file-suffix)')

.PHONY: test

all: clone-submodules build-wasm3 wasm3$(SUFFIX)

# We need the wasm3 submodule, update it just in case
clone-submodules:
	git submodule update --remote

build-wasm3:
	cd wasm3 && mkdir -p build && cd build && cmake .. && make

wasm3$(SUFFIX): wasm3-el.o
	$(LD) -shared $(LDFLAGS) -o wasm3$(SUFFIX) $<

# compile source file to object file
wasm3-el.o: wasm3-el.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I$(ROOT)/src -fPIC -c wasm3-el.c

start:
	$(EMACS) -nw -Q -L .

run: wasm3$(SUFFIX)
	$(EMACS) -Q -L . -l demo.el -f demo

test:
	$(EMACS) -batch -L . -l ert -l tests/wasm3-test.el -f ert-run-tests-batch-and-exit

clean:
	rm -rf wasm3/build/ wasm3$(SUFFIX) wasm3-el.o
