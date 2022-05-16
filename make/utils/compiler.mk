# Copyright (c) 2018-2022 Roman Katuntsev <sbkarr@stappler.org>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

GLOBAL_ROOT ?= .
TOOLKIT_OUTPUT ?= $(GLOBAL_ROOT)/build

UNAME := $(shell uname)

ifneq ($(UNAME),Darwin)
UNAME := $(shell uname -o)
endif

ifeq ($(findstring MSYS_NT,$(UNAME)),MSYS_NT)
UNAME := $(shell uname -o)
endif

ifeq ($(UNAME),Darwin)
include $(GLOBAL_ROOT)/make/os/darwin.mk
else ifeq ($(UNAME),Cygwin)
include $(GLOBAL_ROOT)/make/os/cygwin.mk
else ifeq ($(UNAME),Msys)
include $(GLOBAL_ROOT)/make/os/msys.mk
else
include $(GLOBAL_ROOT)/make/os/linux.mk
endif

ifndef GLOBAL_CPP
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CPP := clang++
else
GLOBAL_CPP := clang++-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CPP := $(MINGW)-g++
else
GLOBAL_CPP := g++
endif # ifdef MINGW
endif # ifdef CLANG
endif # ifndef GLOBAL_CPP

ifndef GLOBAL_CC
ifdef CLANG
ifeq ($(CLANG),1)
GLOBAL_CC := clang
else
GLOBAL_CC := clang-$(CLANG)
endif # ifeq ($(CLANG),1)
OSTYPE_GCHFLAGS := -x c++-header
else
ifdef MINGW
GLOBAL_CC := $(MINGW)-gcc
else
GLOBAL_CC := gcc
endif # ifdef MINGW
endif # ifdef CLANG
endif # ifndef GLOBAL_CC

include $(GLOBAL_ROOT)/make/utils/compile-rules.mk
