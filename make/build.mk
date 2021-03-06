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

BUILD_CURRENT_COUNTER ?= 1
BUILD_FILES_COUNTER ?= 1

TOOLKIT_CLEARABLE_OUTPUT := $(TOOLKIT_OUTPUT)

ifeq ($(UNAME),Cygwin)
GLOBAL_PREBUILT_LIBS_PATH := $(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH)
else
GLOBAL_PREBUILT_LIBS_PATH := $(realpath $(GLOBAL_ROOT)/$(OSTYPE_PREBUILT_PATH))
endif

ifdef LOCAL_TOOLKIT
include $(LOCAL_TOOLKIT)
endif

ifeq (4.1,$(firstword $(sort $(MAKE_VERSION) 4.1)))
sp_counter_text = [$(BUILD_LIBRARY): $$(($(BUILD_CURRENT_COUNTER)*100/$(BUILD_FILES_COUNTER)))% $(BUILD_CURRENT_COUNTER)/$(BUILD_FILES_COUNTER)]
else
sp_counter_text = 
endif

ifdef verbose
ifneq ($(verbose),yes)
GLOBAL_QUIET_CC = @ echo $(call sp_counter_text) [$(notdir $(GLOBAL_CC))] $@ ;
GLOBAL_QUIET_CPP = @ echo $(call sp_counter_text) [$(notdir $(GLOBAL_CPP))] $@ ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif
else
GLOBAL_QUIET_CC = @ echo $(call sp_counter_text) [$(notdir $(GLOBAL_CC))] $(notdir $@) ;
GLOBAL_QUIET_CPP = @ echo $(call sp_counter_text) [$(notdir $(GLOBAL_CPP))] $(notdir $@) ;
GLOBAL_QUIET_LINK = @ echo [Link] $@ ;
endif

.PHONY: clean

clean:
	$(GLOBAL_RM) -r $(TOOLKIT_CLEARABLE_OUTPUT) $(GLOBAL_OUTPUT)
