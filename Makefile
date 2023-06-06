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

all: clone-submodules build-wasm3 wasm$(SUFFIX)

# We need the wasm3 submodule, update it just in case
clone-submodules:
	git submodule update --init --recursive #git submodule update --remote

build-wasm3:
	cd wasm3 && mkdir -p build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && make

wasm$(SUFFIX): wasm3-el.o
	$(LD) -shared -fPIC -o wasm$(SUFFIX) $< $(LDFLAGS)

# compile source file to object file
wasm3-el.o: wasm3-el.c
	$(CC) -c wasm3-el.c $(CFLAGS) -I$(ROOT)/src $(LDFLAGS)

start:
	$(EMACS) -nw -Q -L .

run: wasm$(SUFFIX)
	$(EMACS) -Q -L . -l demo.el -f demo

test:
	$(EMACS) -batch -L . -l ert -l tests/wasm3-test.el -f ert-run-tests-batch-and-exit

clean:
	rm -rf wasm3/build/ wasm$(SUFFIX) wasm3-el.o
