# Compiler and flags
CFLAGS  := -std=c11 -Wall -O2 -D_REENTRANT
LIBS    := -lpthread -lm -lcrypto -lssl

# Path to the Machnet headers and source files
MACHNET_EXT_DIR := src/ext           # Path for machnet.h and related files
MACHNET_INCLUDE_DIR := src/include    # Path for additional headers like pause.h

# Add the include paths for Machnet and LuaJIT
CFLAGS += -I$(MACHNET_EXT_DIR) -I$(MACHNET_INCLUDE_DIR) -Ideps/luajit/src

# Detect the platform
TARGET  := $(shell uname -s | tr '[A-Z]' '[a-z]' 2>/dev/null || echo unknown)

ifeq ($(TARGET), sunos)
	CFLAGS += -D_PTHREADS -D_POSIX_C_SOURCE=200112L
	LIBS   += -lsocket
else ifeq ($(TARGET), darwin)
	export MACOSX_DEPLOYMENT_TARGET = $(shell sw_vers -productVersion)
	LDFLAGS += -pagezero_size 10000 -image_base 100000000
	LIBS += -L/usr/local/opt/openssl/lib
	CFLAGS += -I/usr/local/include -I/usr/local/opt/openssl/include
else ifeq ($(TARGET), linux)
	CFLAGS  += -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE
	LIBS    += -ldl
	LDFLAGS += -Wl,-E
else ifeq ($(TARGET), freebsd)
	CFLAGS  += -D_DECLARE_C99_LDBL_MATH
	LDFLAGS += -Wl,-E
endif

# Source files and output
SRC  := wrk.c net.c ssl.c aprintf.c stats.c script.c units.c \
		ae.c zmalloc.c http_parser.c tinymt64.c hdr_histogram.c \
		src/ext/machnet.c

BIN  := wrk

ODIR := obj
OBJ  := $(patsubst %.c,$(ODIR)/%.o,$(SRC)) $(ODIR)/bytecode.o

# LuaJIT directory and libraries
LDIR     := deps/luajit/src
LIBS := -lluajit -ldl -luuid $(LIBS)
LDFLAGS += -L$(LDIR)

# Targets
all: $(ODIR)/src/ext $(BIN)

clean:
	$(RM) $(BIN) obj/*
	@$(MAKE) -C deps/luajit clean

$(BIN): $(OBJ)
	@echo LINK $(BIN)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ): config.h Makefile $(LDIR)/libluajit.a | $(ODIR)

$(ODIR)/src/ext:
	@mkdir -p $(ODIR)/src/ext

$(ODIR)/bytecode.o: src/wrk.lua
	@echo LUAJIT $<
	@$(SHELL) -c 'cd $(LDIR) && ./luajit -b $(CURDIR)/$< $(CURDIR)/$@'

$(ODIR)/%.o : %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(LDIR)/libluajit.a:
	@echo Building LuaJIT...
	@$(MAKE) -C $(LDIR) BUILDMODE=static

# File type suffixes and vpath for locating source and header files
.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .c .o .lua

# vpath directive to handle source paths
vpath %.c   src $(MACHNET_EXT_DIR)
vpath %.h   src $(MACHNET_EXT_DIR) $(MACHNET_INCLUDE_DIR)
vpath %.lua scripts
