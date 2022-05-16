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


#ifndef MODULES_FILESYSTEM_SPFILEPATH_H_
#define MODULES_FILESYSTEM_SPFILEPATH_H_

#include "SPCommon.h"
#include "SPStringView.h"

namespace stappler::filepath {

// check if filepath is absolute
bool isAbsolute(StringView path);

bool isCanonical(StringView path);

// check if filepath is in application bundle
bool isBundled(StringView path);

// check if filepath above it's current root
bool isAboveRoot(StringView path);

// check for ".", ".." and double slashes in path
bool validatePath(StringView path);

// remove any ".", ".." and double slashes from path
template <typename Interface>
auto reconstructPath(StringView path) -> typename Interface::StringType;

// returns current absolute path for file (canonical prefix will be decoded), this path should not be cached
// if writable flag is false, platform path will be returned with canonical prefix %PLATFORM%
// if writable flag is true, system should resolve platform prefix to absolute path, if possible
// if platform path can not be resolved (etc, it's in archive or another FS), empty string will be returned
template <typename Interface>
auto absolute(StringView, bool writable = false) -> typename Interface::StringType;

// encodes path for long-term storage (default application dirs will be replaced with canonical prefix,
// like %CACHE%/dir)
template <typename Interface>
auto canonical(StringView path) -> typename Interface::StringType;

// extract root from path by removing last component (/dir/file.tar.bz -> /dir)
StringView root(StringView path);

// extract last component (/dir/file.tar.bz -> file.tar.bz)
StringView lastComponent(StringView path);
StringView lastComponent(StringView path, size_t allowedComponents);

// extract full filename extension (/dir/file.tar.gz -> tar.gz)
StringView fullExtension(StringView path);

// extract last filename extension (/dir/file.tar.gz -> gz)
StringView lastExtension(StringView path);

// /dir/file.tar.bz -> file
StringView name(StringView path);

// /dir/file.tar.bz -> 2
size_t extensionCount(StringView path);

template <typename Interface>
auto split(StringView) -> typename Interface::template VectorType<StringView>;

// merges two path component, removes or adds '/' where needed
template <typename Interface>
auto merge(StringView root, StringView path) -> typename Interface::StringType;

std::string merge(const std::vector<std::string> &);
memory::string merge(const memory::vector<memory::string> &);

std::string merge(const std::vector<StringView> &);
memory::string merge(const memory::vector<StringView> &);

template <typename Interface, class... Args>
inline auto merge(StringView root, StringView path, Args&&... args) -> typename Interface::StringType;

// translate some MIME Content-Type to common extensions
StringView extensionForContentType(StringView type);

// replace root path component in filepath
// replace(/my/dir/first/file, /my/dir/first, /your/dir/second)
// [/my/dir/first -> /your/dir/second] /file
// /my/dir/first/file -> /your/dir/second/file
template <typename Interface>
auto replace(StringView path, StringView source, StringView dest) -> typename Interface::StringType;

// Implementation

template <typename Interface>
auto reconstructPath(StringView path) -> typename Interface::StringType {
	typename Interface::StringType ret; ret.reserve(path.size());
	bool start = (path.front() == '/');
	bool end = (path.back() == '/');

	typename Interface::VectorType<StringView> retVec;
	StringView r(path);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if (str == ".." && str.size() == 2) {
			if (!retVec.empty()) {
				retVec.pop_back();
			}
		} else if ((str == "." && str.size() == 1) || str.size() == 0) {
		} else if (!str.empty()) {
			retVec.emplace_back(str);
		}
		if (r.is('/')) {
			++ r;
		}
	}

	if (start) {
		ret.push_back('/');
	}

	bool f = false;
	for (auto &it : retVec) {
		if (f) {
			ret.push_back('/');
		} else {
			f = true;
		}
		ret.append(it.data(), it.size());
	}
	if (end) {
		ret.push_back('/');
	}
	return ret;
}

template <typename Interface>
auto merge(StringView root, StringView path) -> typename Interface::StringType {
	if (path.empty()) {
		return root.str<Interface>();
	}
	if (root.back() == '/') {
		if (path.front() == '/') {
			return StringView::merge<Interface>(root, path.sub(1));
		} else {
			return StringView::merge<Interface>(root, path);
		}
	} else {
		if (path.front() == '/') {
			return StringView::merge<Interface>(root, path);
		} else {
			return StringView::merge<Interface>(root, "/", path);
		}
	}
}

template <typename Interface, class... Args>
auto merge(StringView root, StringView path, Args&&... args) {
	return merge<Interface>(merge<Interface>(root, path), std::forward<Args>(args)...);
}

template <typename Interface>
auto split(StringView str) -> typename Interface::template VectorType<StringView> {
	typename Interface::VectorType<StringView> ret;
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<StringView::Chars<'/', '?', ';', '&', '#'>>();
		ret.push_back(path);
	} while (!s.empty() && s.is('/'));
	return ret;
}

template <typename Interface>
auto replace(StringView path, StringView source, StringView dest) -> typename Interface::StringType {
	if (path.starts_with(source)) {
		return filepath::merge<Interface>(dest, path.sub(source.size()));
	}
	return path.str<Interface>();
}


}

#endif /* MODULES_FILESYSTEM_SPFILEPATH_H_ */
