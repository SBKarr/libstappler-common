/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_NETWORK_SPURL_H_
#define COMMON_UTILS_NETWORK_SPURL_H_

#include "SPStringView.h"

#if MODULE_COMMON_DATA
#include "SPData.h"
#endif

namespace stappler {

struct UrlView {
	enum class UrlToken {
		Scheme,
		User,
		Password,
		Host,
		Port,
		Path,
		Query,
		Fragment,
		Blank,
	};

	StringView scheme;
	StringView user;
	StringView password;
	StringView host;
	StringView port;
	StringView path;
	StringView query;
	StringView fragment;

	StringView url;

	static bool isValidIdnTld(StringView);

	static bool parseUrl(StringView &s, const Callback<void(StringViewUtf8, UrlView::UrlToken)> &cb);

	template <typename Interface>
	static auto parsePath(StringView) -> typename Interface::VectorType<StringView>;

#if MODULE_COMMON_DATA
	template <typename Interface>
	static auto parseArgs(StringView, size_t max) -> data::ValueTemplate<Interface>;
#endif

	UrlView();
	UrlView(StringView);

	void clear();

	bool parse(const StringView &);
	bool parse(StringView &); // leaves unparsed part of the string in view

	template <typename Interface>
	auto get() const -> typename Interface::StringType;

	bool isEmail() const;
	bool isPath() const;
};

}

#endif /* COMMON_UTILS_NETWORK_SPURL_H_ */
