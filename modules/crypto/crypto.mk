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

MODULE_COMMON_CRYPTO_GNUTLS_LIBS := -l:libgnutls.a -l:libhogweed.a -l:libnettle.a -l:libgmp.a -l:libunistring.a -l:libcrypto.a -l:libssl.a
MODULE_COMMON_CRYPTO_GNUTLS_FLAGS := -DSP_CRYPTO_GNUTLS
MODULE_COMMON_CRYPTO_GNUTLS_SRCS_DIRS := $(COMMON_MODULE_DIR)/crypto
MODULE_COMMON_CRYPTO_GNUTLS_SRCS_OBJS := 
MODULE_COMMON_CRYPTO_GNUTLS_INCLUDES_DIRS :=
MODULE_COMMON_CRYPTO_GNUTLS_INCLUDES_OBJS := $(COMMON_MODULE_DIR)/crypto
MODULE_COMMON_CRYPTO_GNUTLS_DEPENDS_ON := common_idn

# module name resolution
MODULE_common_crypto_gnutls := MODULE_COMMON_CRYPTO_GNUTLS

MODULE_COMMON_CRYPTO_LIBS := -l:libcrypto.a -l:libssl.a
MODULE_COMMON_CRYPTO_FLAGS := -DSP_CRYPTO_OPENSSL
MODULE_COMMON_CRYPTO_SRCS_DIRS := $(COMMON_MODULE_DIR)/crypto
MODULE_COMMON_CRYPTO_SRCS_OBJS := 
MODULE_COMMON_CRYPTO_INCLUDES_DIRS :=
MODULE_COMMON_CRYPTO_INCLUDES_OBJS := $(COMMON_MODULE_DIR)/crypto
MODULE_COMMON_CRYPTO_DEPENDS_ON :=

# module name resolution
MODULE_common_crypto := MODULE_COMMON_CRYPTO
