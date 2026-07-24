CC ?= gcc
CFLAGS = -Os -s

ifdef DEBUG
  CFLAGS = -g -O0
endif

.PHONY: all clean

ifeq ($(OS),Windows_NT)
  TARGET = dist/helper_win.exe
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    TARGET = dist/helper_linux
  endif
  ifeq ($(UNAME_S),Darwin)
    TARGET = dist/helper_mac
  endif
endif

all: $(TARGET)

dist:
	@mkdir -p dist

dist/helper_win.exe: src/helper_win.c src/version.rc | dist
	windres src/version.rc -O coff -o src/version.res
	$(CC) src/helper_win.c src/version.res -o dist/helper_win.exe $(CFLAGS) -static -static-libgcc -luser32 -lshell32 "-Wl,--gc-sections" "-Wl,--strip-all" "-Wl,--subsystem,windows"

dist/helper_linux: src/helper_linux.c src/common_posix.c | dist
	$(CC) src/helper_linux.c src/common_posix.c -o dist/helper_linux $(CFLAGS)

dist/helper_mac: src/helper_mac.c src/common_posix.c src/Info.plist | dist
	clang src/helper_mac.c src/common_posix.c -o dist/helper_mac $(CFLAGS) -Wl,-dead_strip -Wl,-sectcreate,__TEXT,__info_plist,src/Info.plist

clean:
	rm -f dist/helper_win.exe dist/helper_linux dist/helper_mac