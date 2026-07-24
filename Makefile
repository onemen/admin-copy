CC ?= gcc
CFLAGS = -Os -s

ifdef DEBUG
  CFLAGS = -g -O0
endif

UNAME_S := $(shell uname -s 2>nul || echo Windows)

.PHONY: all clean

all: dist/helper_win.exe dist/helper_linux dist/helper_mac

dist:
	@mkdir -p dist

dist/helper_win.exe: src/helper_win.c | dist
	$(CC) src/helper_win.c -o dist/helper_win.exe $(CFLAGS) -static -static-libgcc -luser32 -lshell32 "-Wl,--gc-sections" "-Wl,--strip-all" "-Wl,--subsystem,windows"

dist/helper_linux: src/helper_linux.c src/common_posix.c | dist
	$(CC) src/helper_linux.c src/common_posix.c -o dist/helper_linux $(CFLAGS) -static -static-libgcc

dist/helper_mac: src/helper_mac.c src/common_posix.c | dist
	clang src/helper_mac.c src/common_posix.c -o dist/helper_mac $(CFLAGS) -framework CoreFoundation

clean:
	rm -f dist/helper_win.exe dist/helper_linux dist/helper_mac
	@if exist dist\helper_win.exe del /q dist\helper_win.exe 2>nul
	@if exist dist\helper_linux del /q dist\helper_linux 2>nul
	@if exist dist\helper_mac del /q dist\helper_mac 2>nul