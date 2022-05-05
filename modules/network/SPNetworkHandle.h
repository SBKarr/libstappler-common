/**
Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef MODULES_NETWORK_SPNETWORKHANDLE_H_
#define MODULES_NETWORK_SPNETWORKHANDLE_H_

#include "SPNetworkData.h"

namespace stappler::network {

template <typename Interface>
class Handle : private HandleData<Interface> {
public:
	using DataType = HandleData<Interface>;

	using DataType::Vector;
	using DataType::Function;
	using DataType::Map;
	using DataType::String;
	using DataType::StringStream;
	using DataType::Bytes;
	using DataType::Value;
	using DataType::ProgressCallback;
	using DataType::IOCallback;

	Handle() { }

	Handle(Handle &&) = default;
	Handle &operator=(Handle &&) = default;

	bool init(Method method, StringView url);

	bool perform();

	using DataType::getResponseCode;
	using DataType::getErrorCode;
	using DataType::getError;
	using DataType::setCookieFile;
	using DataType::setUserAgent;
	using DataType::clearHeaders;
	using DataType::addHeader;
	using DataType::getRequestHeaders;
	using DataType::setMailFrom;
	using DataType::clearMailTo;
	using DataType::addMailTo;
	using DataType::setAuthority;
	using DataType::setPrivateKeyAuth;
	using DataType::setProxy;
	using DataType::setReceiveFile;
	using DataType::setReceiveCallback;
	using DataType::setSendFile;
	using DataType::setSendCallback;
	using DataType::setSendData;
	using DataType::getReceivedHeaderString;
	using DataType::getReceivedHeaderInt;
	using DataType::getRecievedHeaders;
	using DataType::getMethod;
	using DataType::getUrl;
	using DataType::getCookieFile;
	using DataType::getUserAgent;
	using DataType::getResponseContentType;
	using DataType::setDebug;
	using DataType::setReuse;
	using DataType::setShared;
	using DataType::setSilent;
	using DataType::getDebugData;
	using DataType::setDownloadProgress;
	using DataType::setUploadProgress;
	using DataType::setConnectTimeout;
	using DataType::setLowSpeedLimit;

protected:
	template <typename I>
	friend class MultiHandle;

	HandleData<Interface> *getData() { return this; }
};

template <typename Interface>
class MultiHandle : public Interface::AllocBaseType {
public:
	// handle should be preserved until operation ends
	// multihandle do not stores handles by itself
	void addHandle(Handle<Interface> *handle, void *userdata) {
		pending.emplace_back(pair(handle, userdata));
	}

	// sync interface:
	// returns completed handles, so it can be immediately recharged with addHandle
	bool perform(const Callback<bool(Handle<Interface> *, void *)> &);

protected:
	typename Interface::VectorType<Pair<Handle<Interface> *, void *>> pending;
};

}

#endif /* COMMON_UTILS_NETWORK_SPNETWORKHANDLE_H_ */
