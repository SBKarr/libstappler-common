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

MODULE_COMMON_NETWORK_LIBS := -l:libcurl.a -l:libngtcp2.a -l:libngtcp2_crypto_openssl.a -l:libnghttp3.a  -l:libssl.a -l:libcrypto.a
MODULE_COMMON_NETWORK_SRCS_DIRS := $(COMMON_MODULE_DIR)/network
MODULE_COMMON_NETWORK_SRCS_OBJS :=
MODULE_COMMON_NETWORK_INCLUDES_DIRS :=
MODULE_COMMON_NETWORK_INCLUDES_OBJS := $(COMMON_MODULE_DIR)/network
MODULE_COMMON_NETWORK_DEPENDS_ON := common_idn common_crypto common_filesystem common_brotli_lib

# module name resolution
MODULE_common_network := MODULE_COMMON_NETWORK
