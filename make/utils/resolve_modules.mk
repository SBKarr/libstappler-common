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

define emplace_module =
LOCAL_MODULES += $(1)
$(eval $(call follow_deps_module,$(1)))
endef

define follow_deps_module =
$(foreach dep,$($(1)_DEPENDS_ON),\
	$(if $(filter $(dep),$(LOCAL_MODULES)),,$(eval $(call emplace_module,$(dep)))) \
)
endef

define merge_module =
$(TOOLKIT_NAME)_FLAGS += $($(1)_FLAGS) -D$(1)
$(TOOLKIT_NAME)_LIBS += $($(1)_LIBS)
$(TOOLKIT_NAME)_SRCS_DIRS += $($(1)_SRCS_DIRS)
$(TOOLKIT_NAME)_SRCS_OBJS += $($(1)_SRCS_OBJS)
$(TOOLKIT_NAME)_INCLUDES_DIRS += $($(1)_INCLUDES_DIRS)
$(TOOLKIT_NAME)_INCLUDES_OBJS += $($(1)_INCLUDES_OBJS)
endef

$(foreach module,$(LOCAL_MODULES),$(foreach module_name,$(MODULE_$(module)),\
	$(eval $(call follow_deps_module,$(module_name)))))

$(foreach module,$(LOCAL_MODULES),$(foreach module_name,$(MODULE_$(module)),\
	$(eval $(call merge_module,$(module_name)))))
	