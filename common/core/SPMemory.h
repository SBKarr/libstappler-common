/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_CORE_SPMEMORY_H_
#define COMMON_CORE_SPMEMORY_H_

#include "SPMemInterface.h"
#include "SPStringView.h"
#include "SPString.h"
#include "SPSpanView.h"
#include "SPTime.h"

#ifdef MODULE_COMMON_DATA
#include "SPData.h"
#endif

namespace stappler::mem_pool {

namespace pool = memory::pool;
namespace allocator = memory::allocator;

using CharGroupId = stappler::CharGroupId;

using memory::allocator_t;
using memory::pool_t;

using stappler::Time;
using stappler::TimeInterval;

using stappler::StringView;
using stappler::StringViewUtf8;
using stappler::WideStringView;
using stappler::BytesView;
using stappler::SpanView;

using AllocBase = stappler::memory::AllocPool;

using String = stappler::memory::string;
using WideString = stappler::memory::u16string;
using Bytes = stappler::memory::vector<uint8_t>;

using StringStream = stappler::memory::ostringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = stappler::memory::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = stappler::memory::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = stappler::memory::set<T, Compare>;

template <typename T>
using Function = stappler::memory::function<T>;

using stappler::Callback;

using stappler::Pair;

template <typename T, typename V, typename Compare = std::less<void>>
using dict = stappler::memory::dict<T, V, Compare>;

using Mutex = std::mutex;

using stappler::makeSpanView;

template <typename T>
inline bool emplace_ordered(Vector<T> &vec, T val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}


namespace stappler::mem_std {

namespace pool = memory::pool;
namespace allocator = memory::allocator;

using memory::allocator_t;
using memory::pool_t;

using stappler::Time;
using stappler::TimeInterval;

using stappler::StringView;
using stappler::StringViewUtf8;
using stappler::WideStringView;
using stappler::BytesView;
using stappler::SpanView;

using AllocBase = stappler::memory::AllocBase;

using String = std::string;
using WideString = std::u16string;
using Bytes = std::vector<uint8_t>;

using StringStream = std::stringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = std::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = std::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = std::set<T, Compare>;

template <typename T, typename V>
using HashMap = std::unordered_map<T, V>;

template <typename T>
using Function = std::function<T>;

using stappler::Callback;

using stappler::Pair;

using Mutex = std::mutex;

using stappler::makeSpanView;

template <typename T>
inline bool emplace_ordered(Vector<T> &vec, T val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}


#ifdef MODULE_COMMON_DATA

namespace stappler::mem_pool {

using Value = stappler::data::ValueTemplate<stappler::memory::PoolInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline auto writeData(const Value &data, EncodeFormat fmt = EncodeFormat()) -> Bytes {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(data, fmt);
}

inline bool writeData(std::ostream &stream, const Value &data, EncodeFormat fmt = EncodeFormat()) {
	return stappler::data::EncodeTraits<stappler::memory::PoolInterface>::write(stream, data, fmt);
}

inline bool emplace_ordered(Vector<Value> &vec, const Value &val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val, [&] (const Value &l, const Value &r) {
		return l.getInteger() < r.getInteger();
	});
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}


namespace stappler::mem_std {

using Value = stappler::data::ValueTemplate<stappler::memory::StandartInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline auto writeData(const Value &data, EncodeFormat fmt = EncodeFormat()) -> Bytes {
	return stappler::data::EncodeTraits<stappler::memory::StandartInterface>::write(data, fmt);
}

inline bool writeData(std::ostream &stream, const Value &data, EncodeFormat fmt = EncodeFormat()) {
	return stappler::data::EncodeTraits<stappler::memory::StandartInterface>::write(stream, data, fmt);
}

inline bool emplace_ordered(Vector<Value> &vec, const Value &val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val, [&] (const Value &l, const Value &r) {
		return l.getInteger() < r.getInteger();
	});
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}

#endif

#endif /* COMMON_CORE_SPMEMORY_H_ */
