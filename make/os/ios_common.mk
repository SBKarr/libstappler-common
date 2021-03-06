# Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>
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

DEF_SYSROOT_SIM := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
DEF_SYSROOT_OS := "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
XCODE_BIN_PATH := /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin

DEF_MIN_IOS_VERSION := 11.0

IOS_EXPORT_PREFIX ?= $(realpath $(GLOBAL_ROOT))
IOS_EXPORT_PATH := $(if $(LOCAL_ROOT),,$(GLOBAL_ROOT)/)$(BUILD_OUTDIR)
IOS_ROOT := $(TOOLKIT_OUTPUT)/ios/$(if $(RELEASE),release,debug)

IOS_LIBS := $(OSTYPE_STAPPLER_LIBS_LIST) \
	-lz -lsqlite3 -liconv \
	-framework Foundation \
	-framework UIKit \
	-framework OpenGLES \
	-framework Security \
	-framework SystemConfiguration \
	-framework StoreKit \
	-framework CoreGraphics \
	-framework QuartzCore \
	-framework UserNotifications

ios:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) all ios-export
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) all

ios-arm64:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) all ios-export

ios-x86_64:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) allios-export

ios-clean:
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=arm64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_OS) clean
	$(MAKE) -f $(THIS_FILE) IOS_ARCH=x86_64 MIN_IOS_VERSION=$(DEF_MIN_IOS_VERSION) SYSROOT=$(DEF_SYSROOT_SIM) clean

ios_make_stappler_includes = \
	$(patsubst $(GLOBAL_ROOT)/%,$(IOS_EXPORT_PREFIX)/%,\
		$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(STAPPLER_INCLUDES))\
	)

ios_make_material_includes = \
	$(patsubst $(GLOBAL_ROOT)/%,$(IOS_EXPORT_PREFIX)/%,\
		$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(MATERIAL_INCLUDES))\
	)

ios_make_build_includes = \
	$(abspath $(patsubst $(GLOBAL_ROOT)/%,$(IOS_EXPORT_PREFIX)/%,\
		$(filter-out $(addprefix $(GLOBAL_ROOT)/,$(OSTYPE_INCLUDE)),$(BUILD_INCLUDES))\
	))

ios_make_library = \
	$(if $(LOCAL_LIBRARY),-l$(LOCAL_LIBRARY:lib%=%))

ios-export:
	$(GLOBAL_MKDIR) $(IOS_EXPORT_PATH)
	@echo '// Autogenerated by makefile' > $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_INCLUDES = $(call ios_make_stappler_includes)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nMATERIAL_INCLUDES = $(call ios_make_material_includes)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nBUILD_INCLUDES = $(call ios_make_build_includes)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_LDFLAGS = $(call ios_make_library) -lc++ $(IOS_LIBS)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_DEBUG[sdk=iphoneos*] = $(abspath $(IOS_EXPORT_PATH))/debug/$$(CURRENT_ARCH)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_RELEASE[sdk=iphoneos*] = $(abspath $(IOS_EXPORT_PATH))/release/$$(CURRENT_ARCH)' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_DEBUG[sdk=iphonesimulator*] = $(abspath $(IOS_EXPORT_PATH))/debug/x86_64' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nSTAPPLER_LIBPATH_RELEASE[sdk=iphonesimulator*] = $(abspath $(IOS_EXPORT_PATH))/release/x86_64' >> $(IOS_EXPORT_PATH)/export.xcconfig
	@echo '\nEXCLUDED_ARCHS[sdk=iphonesimulator*] = arm64' >> $(IOS_EXPORT_PATH)/export.xcconfig

.PHONY: ios ios-clean ios-export ios-armv7 ios-armv7s ios-arm64 ios-i386 ios-x86_64
