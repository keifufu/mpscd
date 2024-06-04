VERSION = $(shell cat VERSION)
CLEAN = rm -rf build
MKDIR = mkdir -p build
CFLAGS = -Wall
CFLAGS += -O2
# CFLAGS += -g

build/mpscd: | build
	clang $(CFLAGS) src/mpscd.c -o $@

build:
	$(MKDIR)
.PHONY: clean
clean:
	$(CLEAN)