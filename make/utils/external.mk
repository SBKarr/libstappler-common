# Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>
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

BUILD_SRCS := $(call sp_local_source_list,$(LOCAL_SRCS_DIRS),$(LOCAL_SRCS_OBJS))
BUILD_INCLUDES := $(call sp_local_include_list,$(LOCAL_INCLUDES_DIRS),$(LOCAL_INCLUDES_OBJS),$(LOCAL_ABSOLUTE_INCLUDES))
BUILD_OBJS := $(call sp_local_object_list,$(BUILD_OUTDIR),$(BUILD_SRCS))

ifdef LOCAL_MAIN
BUILD_MAIN_SRC := $(realpath $(addprefix $(LOCAL_ROOT)/,$(LOCAL_MAIN)))
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.c=.o)
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.cpp=.o)
BUILD_MAIN_SRC := $(BUILD_MAIN_SRC:.mm=.o)
BUILD_MAIN_OBJ := $(addprefix $(BUILD_OUTDIR),$(BUILD_MAIN_SRC))
else
BUILD_MAIN_OBJ := 
endif

ifdef LOCAL_TOOLKIT_OUTPUT
TOOLKIT_OUTPUT := $(LOCAL_TOOLKIT_OUTPUT)
endif

include $(GLOBAL_ROOT)/make/build.mk

LOCAL_TOOLKIT ?= material

ifdef LOCAL_TOOLKIT
TOOLKIT_SRCS := $($(TOOLKIT_NAME)_SRCS)
TOOLKIT_OBJS := $($(TOOLKIT_NAME)_OBJS)
TOOLKIT_CFLAGS := $($(TOOLKIT_NAME)_CFLAGS)
TOOLKIT_CXXFLAGS := $($(TOOLKIT_NAME)_CXXFLAGS)
TOOLKIT_LIBS := $($(TOOLKIT_NAME)_LIBS)
TOOLKIT_LIB_FLAGS := $($(TOOLKIT_NAME)_LDFLAGS)
TOOLKIT_H_GCH := $($(TOOLKIT_NAME)_H_GCH)
TOOLKIT_GCH := $($(TOOLKIT_NAME)_GCH)
TOOLKIT_MODULES := $($(TOOLKIT_NAME)_MODULES)
endif

BUILD_CFLAGS += $(LOCAL_CFLAGS) $(TOOLKIT_CFLAGS)
BUILD_CXXFLAGS += $(LOCAL_CFLAGS) $(LOCAL_CXXFLAGS) $(TOOLKIT_CXXFLAGS)

# Progress counter
BUILD_COUNTER := 0
BUILD_WORDS := $(words $(BUILD_OBJS) $(BUILD_MAIN_OBJ))

define BUILD_template =
$(eval BUILD_COUNTER=$(shell echo $$(($(BUILD_COUNTER)+1))))
$(1):BUILD_CURRENT_COUNTER:=$(BUILD_COUNTER)
$(1):BUILD_FILES_COUNTER := $(BUILD_WORDS)
ifdef LOCAL_EXECUTABLE
$(1):BUILD_LIBRARY := $(notdir $(LOCAL_EXECUTABLE))
else
$(1):BUILD_LIBRARY := $(notdir $(LOCAL_LIBRARY))
endif
endef

$(foreach obj,$(BUILD_OBJS) $(BUILD_MAIN_OBJ),$(eval $(call BUILD_template,$(obj))))

BUILD_LOCAL_OBJS := $(BUILD_OBJS) $(BUILD_MAIN_OBJ)

BUILD_OBJS += $(TOOLKIT_OBJS)

BUILD_CFLAGS += $(addprefix -I,$(BUILD_INCLUDES))
BUILD_CXXFLAGS += $(addprefix -I,$(BUILD_INCLUDES))

BUILD_LIBS := $(foreach lib,$(LOCAL_LIBS),-L$(dir $(lib)) -l:$(notdir $(lib))) $(TOOLKIT_LIBS) $(LDFLAGS)

-include $(patsubst %.o,%.o.d,$(BUILD_OBJS) $(BUILD_MAIN_OBJ))
