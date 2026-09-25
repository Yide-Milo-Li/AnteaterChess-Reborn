CONFIG ?= debug
PYTHON ?= python3
ifeq ($(OS),Windows_NT)
PLATFORM := windows-x64
EXE := .exe
else
PLATFORM := linux-x64
EXE :=
endif
BUILD ?= build/$(PLATFORM)/$(CONFIG)
CPPFLAGS += -Iinclude -D_POSIX_C_SOURCE=200809L
CFLAGS += -std=c11 -Wall -Wextra -Werror
ifeq ($(CONFIG),release)
CFLAGS += -O2
else
CFLAGS += -O0 -g
endif
ifeq ($(CONFIG),sanitize)
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)
