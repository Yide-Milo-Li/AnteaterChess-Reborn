CONFIG ?= debug
PYTHON ?= python3
WINDOWS_CONSOLE ?= 0
ifeq ($(OS),Windows_NT)
PLATFORM := windows-x64
EXE := .exe
ifeq ($(WINDOWS_CONSOLE),1)
APP_LDFLAGS := -mconsole
BUILD_SUFFIX := -console
else ifeq ($(WINDOWS_CONSOLE),0)
APP_LDFLAGS := -mwindows
else
$(error WINDOWS_CONSOLE must be 0 or 1)
endif
else
PLATFORM := linux-x64
EXE :=
endif
BUILD ?= build/$(PLATFORM)/$(CONFIG)$(BUILD_SUFFIX)
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
