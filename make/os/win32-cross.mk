# Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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

MSVC_INCLUDE_CRT_INCLUDE ?= $(GLOBAL_ROOT)/libs/xwin/splat/crt/include
MSVC_INCLUDE_SDK_INCLUDE ?= $(GLOBAL_ROOT)/libs/xwin/splat/sdk/include/um
MSVC_INCLUDE_UCRT_INCLUDE ?= $(GLOBAL_ROOT)/libs/xwin/splat/sdk/include/ucrt
MSVC_INCLUDE_SHARED_INCLUDE ?= $(GLOBAL_ROOT)/libs/xwin/splat/sdk/include/shared

MSVC_LIB_CRT_LIB ?= $(GLOBAL_ROOT)/libs/xwin/splat/crt/lib/x86_64
MSVC_LIB_SDK_LIB ?= $(GLOBAL_ROOT)/libs/xwin/splat/sdk/lib/um/x86_64
MSVC_LIB_UCRT_LIB ?= $(GLOBAL_ROOT)/libs/xwin/splat/sdk/lib/ucrt/x86_64

GLOBAL_CC ?= clang-12
GLOBAL_CPP ?= clang++-12
GLOBAL_LD ?= lld-link-12

WIN32_ARCH ?= x86_64-pc-win32
WIN32_MACHINE_ARCH ?= x64

OSTYPE_LDFLAGS_COMMON = \
	-fuse-ld=$(GLOBAL_LD) \
	-target $(WIN32_ARCH) \
	-Wl,-machine:$(WIN32_MACHINE_ARCH) \
	-fmsc-version=1900 \
	-Wno-msvc-not-found 

OSTYPE_LDFLAGS_LIBS = \
	-L$(MSVC_LIB_CRT_LIB) \
	-L$(MSVC_LIB_SDK_LIB) \
	-L$(MSVC_LIB_UCRT_LIB) \
	-nostdlib \
	-llibvcruntime \
	-lWS2_32 \

OSTYPE_GCHFLAGS := -x c++-header

OSTYPE_PREBUILT_PATH := libs/win32/x86_64/lib
OSTYPE_INCLUDE :=  libs/win32/x86_64/include

OSTYPE_CFLAGS := -Wall \
	-target $(WIN32_ARCH) \
	-fms-compatibility-version=19 \
	-fms-extensions \
	-fdelayed-template-parsing \
	-fexceptions \
	-mthread-model posix \
	-fno-threadsafe-statics \
	-Wno-msvc-not-found \
	-Wno-microsoft-include \
	-Wno-unused-local-typedef \
	-Wno-pragma-pack \
	-Wno-ignored-pragma-intrinsic \
	-Wno-nonportable-include-path \
	-DWIN32 \
	-D_WIN32 \
	-D_MT \
	-D_DLL \
	-D_AMD64_ \
	-Xclang -disable-llvm-verifier \
	-Xclang '--dependent-lib=msvcrt' \
	-Xclang '--dependent-lib=ucrt' \
	-Xclang '--dependent-lib=oldnames' \
	-Xclang '--dependent-lib=vcruntime' \
	-D_CRT_SECURE_NO_WARNINGS \
	-D_CRT_NONSTDC_NO_DEPRECATE \
	-U__GNUC__ \
	-U__gnu_linux__ \
	-U__GNUC_MINOR__ \
	-U__GNUC_PATCHLEVEL__ \
	-U__GNUC_STDC_INLINE__  \
	-I$(MSVC_INCLUDE_CRT_INCLUDE) \
	-I$(MSVC_INCLUDE_SDK_INCLUDE) \
	-I$(MSVC_INCLUDE_UCRT_INCLUDE) \
	-I$(MSVC_INCLUDE_SHARED_INCLUDE)

OSTYPE_CPPFLAGS := -frtti

OSTYPE_LDFLAGS := $(OSTYPE_LDFLAGS_COMMON) -Wl,-z,defs -rdynamic
OSTYPE_EXEC_FLAGS := $(OSTYPE_LDFLAGS_COMMON)
