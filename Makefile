BUILD_DIR   := build
DIST_DIR    := dist
CMAKE       := cmake
CTEST       := ctest
PREFIX      ?= /usr/local
DESTDIR     ?=

SHARE = $(DIST)/share/lua/$(LUA_VER)
LIB   = $(DIST)/lib/lua/$(LUA_VER)

.PHONY: all build test dist install clean

all: build

build: $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR)

$(BUILD_DIR)/Makefile:
	$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug

dist: build
	$(CMAKE) --install $(BUILD_DIR) --prefix $(abspath $(DIST_DIR))

install: build
	$(CMAKE) --install $(BUILD_DIR) --prefix $(PREFIX) $(if $(DESTDIR),--destdir $(DESTDIR))

test: dist
	LUA_PATH="$(PWD)/$(SHARE)/?.lua;$(PWD)/$(SHARE)/?/init.lua;;" \
	LUA_CPATH="$(PWD)/$(LIB)/?.so;;" \

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
