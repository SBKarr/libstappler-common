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

COMMON_OUTPUT_DIR = $(abspath $(TOOLKIT_OUTPUT)/common)
COMMON_OUTPUT_STATIC = $(abspath $(TOOLKIT_OUTPUT)/libcommon.a)

COMMON_FLAGS :=
COMMON_LIBS :=

COMMON_PRECOMPILED_HEADERS += \
	common/core/SPCommon.h

COMMON_SRCS_DIRS := common thirdparty
COMMON_SRCS_OBJS := 
COMMON_INCLUDES_DIRS := common
COMMON_INCLUDES_OBJS := $(OSTYPE_INCLUDE) thirdparty

TOOLKIT_NAME := COMMON
TOOLKIT_TITLE := common

include $(GLOBAL_ROOT)/common-modules.mk

ifdef LOCAL_MODULES
include $(GLOBAL_ROOT)/make/utils/resolve-modules.mk
endif

include $(GLOBAL_ROOT)/make/utils/make-toolkit.mk

$(COMMON_OUTPUT_STATIC) : $(COMMON_H_GCH) $(COMMON_GCH) $(COMMON_OBJS)
	$(GLOBAL_QUIET_LINK) $(GLOBAL_AR) $(COMMON_OUTPUT_STATIC) $(COMMON_OBJS)

libcommon: $(COMMON_OUTPUT_STATIC)

.PHONY: libcommon
