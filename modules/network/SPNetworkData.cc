/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPNetworkData.h"
#include "SPTime.h"
#include "SPValid.h"
#include "SPCrypto.h"

#if MODULE_COMMON_BITMAP
#include "SPBitmap.h"
#endif

#include <curl/curl.h>

namespace stappler::network {

template <typename Interface>
static void Handle_destroy(HandleData<Interface> &data) {
	if (data.process.sharedHandle) {
		curl_share_cleanup(data.process.sharedHandle);
	}
}

template <typename Interface>
static bool Handle_reset(HandleData<Interface> &data, Method method, StringView url) {
	data.send.url = url.str<Interface>();
	data.send.method = method;
	return true;
}

template <typename Interface>
static void Handle_addHeader(HandleData<Interface> &data, StringView name, StringView value) {
	name.trimChars<StringView::WhiteSpace>();
	value.trimChars<StringView::WhiteSpace>();

	auto nameStr = string::tolower<Interface>(name);

	auto it = data.send.headers.find(nameStr);
	if (it != data.send.headers.end()) {
		it->second = value.str<Interface>();
	} else {
		data.send.headers.emplace(move(nameStr), value.str<Interface>());
	}
}

template <typename Interface>
static void Handle_addMailTo(HandleData<Interface> &data, StringView name) {
	auto nameStr = name.str<Interface>();
	if (!valid::validateEmail(nameStr)) {
		log::vtext("NetworkHandle", "Fail to add MailTo: ", name, ": invalid email address");
		return;
	}

	auto lb = std::lower_bound(data.send.recipients.begin(), data.send.recipients.end(), nameStr);
	if (lb != data.send.recipients.end() && *lb != name) {
		data.send.recipients.emplace(lb, move(nameStr));
	} else if (lb != data.send.recipients.end()) {
		data.send.recipients.emplace_back(move(nameStr));
	}
}

template <typename Interface>
static void Handle_setAuthority(HandleData<Interface> &data, StringView user, StringView passwd, AuthMethod method) {
	if (method == AuthMethod::PKey) {
		return;
	}

	data.auth.data = pair(user.str<Interface>(), passwd.str<Interface>());
	data.auth.authMethod = method;
}

template <typename Interface>
static bool Handle_setPrivateKeyAuth(HandleData<Interface> &iface, const crypto::PrivateKey &pk) {
	auto pub = pk.exportPublic();
	if (!pub) {
		return false;
	}

	bool ret = false;
	pub.exportDer([&] (const uint8_t *pub, size_t pubLen) {
		pk.sign([&] (const uint8_t *sign, size_t signLen) {
			iface.auth.data = base64::encode<Interface>(data::write(data::ValueTemplate<Interface>({
				data::ValueTemplate<Interface>(BytesView(pub, pubLen)),
				data::ValueTemplate<Interface>(BytesView(sign, signLen))
			})));
			iface.auth.authMethod = AuthMethod::PKey;
			ret = true;
		}, BytesView(pub, pubLen), crypto::SignAlgorithm::RSA_SHA512);
	});
	return ret;
}

template <typename Interface>
static bool Handle_setPrivateKeyAuth(HandleData<Interface> &iface, BytesView data) {
	crypto::PrivateKey pk(data);
	if (pk) {
		return Handle_setPrivateKeyAuth(iface, pk);
	}
	return false;
}

template <typename Interface>
static void Handle_setSendFile(HandleData<Interface> &iface, StringView str, StringView type) {
	iface.send.data = str.str<Interface>();
	iface.send.size = 0;
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	} else {
#if LINUX
		auto t = network_getUserMime<Interface>(str);
		if (!t.empty()) {
			iface.addHeader("Content-Type", t);
			return;
		}
#endif
#if MODULE_COMMON_BITMAP
		// try image format
		auto fmt = bitmap::detectFormat(StringView(str));
		if (fmt.first != bitmap::FileFormat::Custom) {
			iface.addHeader("Content-Type", bitmap::getMimeType(fmt.first));
			return;
		} else {
			auto str = bitmap::getMimeType(fmt.second);
			if (!str.empty()) {
				iface.addHeader("Content-Type", str);
				return;
			}
		}
#endif
	}
}

template <typename Interface>
static void Handle_setSendCallback(HandleData<Interface> &iface, typename HandleData<Interface>::IOCallback &&cb, size_t size, StringView type) {
	iface.send.data = move(cb);
	iface.send.size = size;
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	}
}

template <typename Interface>
static void Handle_setSendData(HandleData<Interface> &iface, StringView data, StringView type) {
	iface.send.data = BytesView((const uint8_t *)data.data(), data.size()).bytes<Interface>();
	iface.send.size = data.size();
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	}
}

template <typename Interface>
static void Handle_setSendData(HandleData<Interface> &iface, BytesView data, StringView type) {
	iface.send.data = data.bytes<Interface>();
	iface.send.size = data.size();
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	}
}

template <typename Interface>
static void Handle_setSendData(HandleData<Interface> &iface, typename HandleData<Interface>::Bytes &&data, StringView type) {
	iface.send.size = data.size();
	iface.send.data = move(data);
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	}
}

template <typename Interface>
static void Handle_setSendData(HandleData<Interface> &iface, const uint8_t *data, size_t size, StringView type) {
	iface.send.size = size;
	iface.send.data = BytesView(data, size).bytes<Interface>();
	if (!type.empty()) {
		iface.addHeader("Content-Type", type);
	}
}

template <typename Interface>
static void Handle_setSendData(HandleData<Interface> &iface, const data::ValueTemplate<Interface> &data, data::EncodeFormat fmt) {
	auto d = data::write<Interface>(data, fmt);
	iface.send.size = d.size();
	iface.send.data = move(d);
	if (fmt.format == data::EncodeFormat::Cbor || fmt.format == data::EncodeFormat::DefaultFormat) {
		iface.addHeader("Content-Type", "application/cbor");
	} else if (fmt.format == data::EncodeFormat::Json
			|| fmt.format == data::EncodeFormat::Pretty
			|| fmt.format == data::EncodeFormat::PrettyTime) {
		iface.addHeader("Content-Type", "application/json");
	}
}

template <typename Interface>
static StringView Handle_getReceivedHeaderString(const HandleData<Interface> &iface, StringView name) {
	auto h = string::tolower<Interface>(name);
	auto i = iface.receive.parsed.find(h);
	if (i != iface.receive.parsed.end()) {
		return i->second;
	}
	return StringView();
}

template <typename Interface>
static int64_t Handle_getReceivedHeaderInt(const HandleData<Interface> &iface, StringView name) {
	auto h = string::tolower<Interface>(name);
	auto i = iface.receive.parsed.find(h);
	if (i != iface.receive.parsed.end()) {
		if (!i->second.empty()) {
			return StringToNumber<int64_t>(i->second.c_str());
		}
	}
	return 0;
}

#define HANDLE_NAME(ret, name, ...) template <> ret HandleData<HANDLE_INTERFACE>::name(__VA_ARGS__)
#define HANDLE_NAME_CONST(ret, name, ...) template <> auto HandleData<HANDLE_INTERFACE>::name(__VA_ARGS__) const -> ret

#define HANDLE_INTERFACE memory::PoolInterface

HANDLE_NAME(,~HandleData) { Handle_destroy(*this); }
HANDLE_NAME(bool, reset, Method method, StringView url) { return Handle_reset(*this, method, url); }
HANDLE_NAME_CONST(long, getResponseCode) { return process.responseCode; }
HANDLE_NAME_CONST(long, getErrorCode) { return process.errorCode; }
HANDLE_NAME_CONST(StringView, getError) { return process.error; }
HANDLE_NAME(void, setCookieFile, StringView str) { process.cookieFile = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, setUserAgent, StringView str) { send.userAgent = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, setUrl, StringView str) { send.url = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, clearHeaders) { send.headers.clear(); }
HANDLE_NAME(void, addHeader, StringView header, StringView value) { Handle_addHeader(*this, header, value); }
HANDLE_NAME_CONST(const HeaderMap &, getRequestHeaders) { return send.headers; }

HANDLE_NAME(void, setMailFrom, StringView from) { send.from = from.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, clearMailTo) { send.recipients.clear(); }
HANDLE_NAME(void, addMailTo, StringView to) { Handle_addMailTo(*this, to); }
HANDLE_NAME(void, setAuthority, StringView user, StringView passwd, AuthMethod method) { Handle_setAuthority(*this, user, passwd, method); }
HANDLE_NAME(bool, setPrivateKeyAuth, BytesView priv) { return Handle_setPrivateKeyAuth(*this, priv); }
HANDLE_NAME(bool, setPrivateKeyAuth, const crypto::PrivateKey &priv) { return Handle_setPrivateKeyAuth(*this, priv); }
HANDLE_NAME(void, setProxy, StringView proxy, StringView authData) {
	auth.proxyAddress = proxy.str<HANDLE_INTERFACE>();
	auth.proxyAuth = authData.str<HANDLE_INTERFACE>();
}
HANDLE_NAME(void, setReceiveFile, StringView filename, bool resumeDownload) {
	receive.data = filename.str<HANDLE_INTERFACE>();
	receive.resumeDownload = resumeDownload;
}
HANDLE_NAME(void, setReceiveCallback, IOCallback &&cb) { receive.data = move(cb); }
HANDLE_NAME(void, setResumeDownload, bool resumeDownload) { receive.resumeDownload = resumeDownload; }
HANDLE_NAME(void, setSendSize, size_t size) { send.size = size; }
HANDLE_NAME(void, setSendFile, StringView filename, StringView type) { Handle_setSendFile(*this, filename, type); }
HANDLE_NAME(void, setSendCallback, IOCallback &&cb, size_t outSize, StringView type) { Handle_setSendCallback(*this, move(cb), outSize, type); }
HANDLE_NAME(void, setSendData, StringView data, StringView type) { Handle_setSendData(*this, data, type); }
HANDLE_NAME(void, setSendData, BytesView data, StringView type) { Handle_setSendData(*this, data, type); }
HANDLE_NAME(void, setSendData, Bytes &&data, StringView type) { Handle_setSendData(*this, move(data), type); }
HANDLE_NAME(void, setSendData, const uint8_t *data, size_t size, StringView type) { Handle_setSendData(*this, data, size, type); }
HANDLE_NAME(void, setSendData, const Value &value, data::EncodeFormat fmt) { Handle_setSendData(*this, value, fmt); }
HANDLE_NAME_CONST(StringView, getReceivedHeaderString, StringView name) { return Handle_getReceivedHeaderString(*this, name); }
HANDLE_NAME_CONST(int64_t, getReceivedHeaderInt, StringView name) { return Handle_getReceivedHeaderInt(*this, name); }

HANDLE_NAME_CONST(Method, getMethod) { return send.method; }
HANDLE_NAME_CONST(StringView, getUrl) { return send.url; }
HANDLE_NAME_CONST(StringView, getCookieFile) { return process.cookieFile; }
HANDLE_NAME_CONST(StringView, getUserAgent) { return send.userAgent; }
HANDLE_NAME_CONST(StringView, getResponseContentType) { return receive.contentType; }
HANDLE_NAME_CONST(const Vector<String> &, getRecievedHeaders) { return receive.headers; }

HANDLE_NAME(void, setDebug, bool value) { process.debug = value; }
HANDLE_NAME(void, setReuse, bool value) { process.reuse = value; }
HANDLE_NAME(void, setShared, bool value) { process.shared = value; }
HANDLE_NAME(void, setSilent, bool value) { process.silent = value; }
HANDLE_NAME_CONST(const StringStream &, getDebugData) { return process.debugData; }

HANDLE_NAME(void, setDownloadProgress, ProgressCallback &&cb) { process.downloadProgress = move(cb); }
HANDLE_NAME(void, setUploadProgress, ProgressCallback &&cb) { process.uploadProgress = move(cb); }

HANDLE_NAME(void, setConnectTimeout, int time) { process.connectTimeout = time; }
HANDLE_NAME(void, setLowSpeedLimit, int time, size_t limit) { process.lowSpeedTime = time; process.lowSpeedLimit = int(limit); }

#undef HANDLE_INTERFACE


#define HANDLE_INTERFACE memory::StandartInterface

HANDLE_NAME(,~HandleData) { Handle_destroy(*this); }
HANDLE_NAME(bool, reset, Method method, StringView url) { return Handle_reset(*this, method, url); }
HANDLE_NAME_CONST(long, getResponseCode) { return process.responseCode; }
HANDLE_NAME_CONST(long, getErrorCode) { return process.errorCode; }
HANDLE_NAME_CONST(StringView, getError) { return process.error; }
HANDLE_NAME(void, setCookieFile, StringView str) { process.cookieFile = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, setUserAgent, StringView str) { send.userAgent = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, setUrl, StringView str) { send.url = str.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, clearHeaders) { send.headers.clear(); }
HANDLE_NAME(void, addHeader, StringView header, StringView value) { Handle_addHeader(*this, header, value); }
HANDLE_NAME_CONST(const HeaderMap &, getRequestHeaders) { return send.headers; }

HANDLE_NAME(void, setMailFrom, StringView from) { send.from = from.str<HANDLE_INTERFACE>(); }
HANDLE_NAME(void, clearMailTo) { send.recipients.clear(); }
HANDLE_NAME(void, addMailTo, StringView to) { Handle_addMailTo(*this, to); }
HANDLE_NAME(void, setAuthority, StringView user, StringView passwd, AuthMethod method) { Handle_setAuthority(*this, user, passwd, method); }
HANDLE_NAME(bool, setPrivateKeyAuth, BytesView priv) { return Handle_setPrivateKeyAuth(*this, priv); }
HANDLE_NAME(bool, setPrivateKeyAuth, const crypto::PrivateKey &priv) { return Handle_setPrivateKeyAuth(*this, priv); }
HANDLE_NAME(void, setProxy, StringView proxy, StringView authData) {
	auth.proxyAddress = proxy.str<HANDLE_INTERFACE>();
	auth.proxyAuth = authData.str<HANDLE_INTERFACE>();
}
HANDLE_NAME(void, setReceiveFile, StringView filename, bool resumeDownload) {
	receive.data = filename.str<HANDLE_INTERFACE>();
	receive.resumeDownload = resumeDownload;
}
HANDLE_NAME(void, setReceiveCallback, IOCallback &&cb) { receive.data = move(cb); }
HANDLE_NAME(void, setResumeDownload, bool resumeDownload) { receive.resumeDownload = resumeDownload; }
HANDLE_NAME(void, setSendSize, size_t size) { send.size = size; }
HANDLE_NAME(void, setSendFile, StringView filename, StringView type) { Handle_setSendFile(*this, filename, type); }
HANDLE_NAME(void, setSendCallback, IOCallback &&cb, size_t outSize, StringView type) { Handle_setSendCallback(*this, move(cb), outSize, type); }
HANDLE_NAME(void, setSendData, StringView data, StringView type) { Handle_setSendData(*this, data, type); }
HANDLE_NAME(void, setSendData, BytesView data, StringView type) { Handle_setSendData(*this, data, type); }
HANDLE_NAME(void, setSendData, Bytes &&data, StringView type) { Handle_setSendData(*this, move(data), type); }
HANDLE_NAME(void, setSendData, const uint8_t *data, size_t size, StringView type) { Handle_setSendData(*this, data, size, type); }
HANDLE_NAME(void, setSendData, const Value &value, data::EncodeFormat fmt) { Handle_setSendData(*this, value, fmt); }
HANDLE_NAME_CONST(StringView, getReceivedHeaderString, StringView name) { return Handle_getReceivedHeaderString(*this, name); }
HANDLE_NAME_CONST(int64_t, getReceivedHeaderInt, StringView name) { return Handle_getReceivedHeaderInt(*this, name); }

HANDLE_NAME_CONST(Method, getMethod) { return send.method; }
HANDLE_NAME_CONST(StringView, getUrl) { return send.url; }
HANDLE_NAME_CONST(StringView, getCookieFile) { return process.cookieFile; }
HANDLE_NAME_CONST(StringView, getUserAgent) { return send.userAgent; }
HANDLE_NAME_CONST(StringView, getResponseContentType) { return receive.contentType; }
HANDLE_NAME_CONST(const Vector<String> &, getRecievedHeaders) { return receive.headers; }

HANDLE_NAME(void, setDebug, bool value) { process.debug = value; }
HANDLE_NAME(void, setReuse, bool value) { process.reuse = value; }
HANDLE_NAME(void, setShared, bool value) { process.shared = value; }
HANDLE_NAME(void, setSilent, bool value) { process.silent = value; }
HANDLE_NAME_CONST(const StringStream &, getDebugData) { return process.debugData; }

HANDLE_NAME(void, setDownloadProgress, ProgressCallback &&cb) { process.downloadProgress = move(cb); }
HANDLE_NAME(void, setUploadProgress, ProgressCallback &&cb) { process.uploadProgress = move(cb); }

HANDLE_NAME(void, setConnectTimeout, int time) { process.connectTimeout = time; }
HANDLE_NAME(void, setLowSpeedLimit, int time, size_t limit) { process.lowSpeedTime = time; process.lowSpeedLimit = int(limit); }

#undef HANDLE_INTERFACE

}

#undef SP_TERMINATED_DATA
