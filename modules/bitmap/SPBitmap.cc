/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPBitmap.h"

#include "SPBytesView.h"
#include "SPFilesystem.h"
#include "SPLog.h"
#include "SPBuffer.h"
#include "SPString.h"

namespace stappler::bitmap {

static Pair<FileFormat, StringView> _loadData(BitmapWriter &w, const uint8_t * data, size_t dataLen) {
	for (int i = 0; i < toInt(FileFormat::Custom); ++i) {
		auto &fmt = s_defaultFormats[i];
		if (fmt.is(data, dataLen) && fmt.isReadable()) {
			if (fmt.load(data, dataLen, w)) {
				return Pair(FileFormat(i), fmt.getName());
			}
		}
	}

	memory::vector<BitmapFormat> fns;
	fns.reserve_block_optimal();

	s_formatListMutex.lock();

	fns.reserve(s_formatList.size());
	for (auto &it : s_formatList) {
		if (it.isReadable()) {
			fns.emplace_back(it);
		}
	}

	s_formatListMutex.unlock();

	for (auto &it : fns) {
		if (it.is(data, dataLen) && it.load(data, dataLen, w)) {
			return Pair(FileFormat::Custom, it.getName());
		}
	}

	return Pair(FileFormat::Custom, StringView());
}

struct BitmapPoolTarget {
	memory::PoolInterface::BytesType *bytes;
	const StrideFn *strideFn;
};

struct BitmapStdTarget {
	memory::StandartInterface::BytesType *bytes;
	const StrideFn *strideFn;
};

static void _makeBitmapWriter(BitmapWriter &w, BitmapPoolTarget *target, BitmapTemplate<memory::PoolInterface> &bmp) {
	w.target = target;
	w.color = bmp.format();
	w.alpha = bmp.alpha();
	w.width = bmp.width();
	w.height = bmp.height();
	w.stride = bmp.stride();

	if (target->strideFn) {
		w.getStride = [] (void *ptr, PixelFormat f, uint32_t w) {
			return (*((BitmapPoolTarget *)ptr)->strideFn)(f, w);
		};
	}

	w.push = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto bytes = ((BitmapPoolTarget *)ptr)->bytes;
		auto origSize = bytes->size();
		bytes->resize(origSize + size);
		memcpy(bytes->data() + origSize, data, size);
	};
	w.resize = [] (void *ptr, uint32_t size) {
		((BitmapPoolTarget *)ptr)->bytes->resize(size);
	};
	w.getData = [] (void *ptr, uint32_t location) {
		return ((BitmapPoolTarget *)ptr)->bytes->data() + location;
	};
	w.assign = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto bytes = ((BitmapPoolTarget *)ptr)->bytes;
		bytes->resize(size);
		memcpy(bytes->data(), data, size);
	};
	w.clear = [] (void *ptr) {
		((BitmapPoolTarget *)ptr)->bytes->clear();
	};
}

static void _makeBitmapWriter(BitmapWriter &w, BitmapStdTarget *target, BitmapTemplate<memory::StandartInterface> &bmp) {
	w.target = target;
	w.color = bmp.format();
	w.alpha = bmp.alpha();
	w.width = bmp.width();
	w.height = bmp.height();
	w.stride = bmp.stride();

	if (target->strideFn) {
		w.getStride = [] (void *ptr, PixelFormat f, uint32_t w) {
			return (*((BitmapPoolTarget *)ptr)->strideFn)(f, w);
		};
	}

	w.push = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto bytes = ((BitmapStdTarget *)ptr)->bytes;
		auto origSize = bytes->size();
		bytes->resize(origSize + size);
		memcpy(bytes->data() + origSize, data, size);
	};
	w.resize = [] (void *ptr, uint32_t size) {
		((BitmapStdTarget *)ptr)->bytes->resize(size);
	};
	w.getData = [] (void *ptr, uint32_t location) {
		return ((BitmapStdTarget *)ptr)->bytes->data() + location;
	};
	w.assign = [] (void *ptr, const uint8_t *data, uint32_t size) {
		auto bytes = ((BitmapStdTarget *)ptr)->bytes;
		bytes->resize(size);
		memcpy(bytes->data(), data, size);
	};
	w.clear = [] (void *ptr) {
		((BitmapStdTarget *)ptr)->bytes->clear();
	};
}

template <>
bool BitmapTemplate<memory::PoolInterface>::loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn) {
	BitmapPoolTarget target { &_data, strideFn ? &strideFn : nullptr };
	BitmapWriter w;
	_makeBitmapWriter(w, &target, *this);
	auto ret = _loadData(w, data, dataLen);
	if (!ret.second.empty()) {
		_color = w.color;
		_alpha = w.alpha;
		_width = w.width;
		_height = w.height;
		_stride = w.stride;
		_originalFormat = ret.first;
		_originalFormatName = ret.second;
		return true;
	}
	return false;
}

template <>
bool BitmapTemplate<memory::StandartInterface>::loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn) {
	BitmapStdTarget target { &_data, strideFn ? &strideFn : nullptr };
	BitmapWriter w;
	_makeBitmapWriter(w, &target, *this);
	auto ret = _loadData(w, data, dataLen);
	if (!ret.second.empty()) {
		_color = w.color;
		_alpha = w.alpha;
		_width = w.width;
		_height = w.height;
		_stride = w.stride;
		_originalFormat = ret.first;
		_originalFormatName = ret.second;
		return true;
	}
	return false;
}

template <>
bool BitmapTemplate<memory::StandartInterface>::save(FileFormat fmt, StringView path, bool invert) {
	BitmapWriter w;
	_makeBitmapWriter(w, nullptr, *this);
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.save(path, _data.data(), w, invert);
	} else {
		// fallback to png
		return s_defaultFormats[toInt(FileFormat::Png)]
			.save(path, _data.data(), w, invert);
	}
	return false;
}

template <>
bool BitmapTemplate<memory::PoolInterface>::save(FileFormat fmt, StringView path, bool invert) {
	BitmapWriter w;
	_makeBitmapWriter(w, nullptr, *this);
	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable()) {
		return support.save(path, _data.data(), w, invert);
	} else {
		// fallback to png
		return s_defaultFormats[toInt(FileFormat::Png)]
			.save(path, _data.data(), w, invert);
	}
	return false;
}

template <>
bool BitmapTemplate<memory::StandartInterface>::save(StringView name, StringView path, bool invert) {
	BitmapFormat::save_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getSaveFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		BitmapWriter w;
		_makeBitmapWriter(w, nullptr, *this);
		return fn(path, _data.data(), w, invert);
	}
	return false;
}

template <>
bool BitmapTemplate<memory::PoolInterface>::save(StringView name, StringView path, bool invert) {
	BitmapFormat::save_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getSaveFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		BitmapWriter w;
		_makeBitmapWriter(w, nullptr, *this);
		return fn(path, _data.data(), w, invert);
	}
	return false;
}

template <>
auto BitmapTemplate<memory::StandartInterface>::write(FileFormat fmt, bool invert) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;
	BitmapStdTarget target { &ret, nullptr };
	BitmapWriter w;
	_makeBitmapWriter(w, &target, *this);

	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable() && support.write(_data.data(), w, invert)) {
		return ret;
	} else if (s_defaultFormats[toInt(FileFormat::Png)].write(_data.data(), w, invert)) {
		return ret;
	}
	return memory::StandartInterface::BytesType();
}

template <>
auto BitmapTemplate<memory::PoolInterface>::write(FileFormat fmt, bool invert) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;
	BitmapPoolTarget target { &ret, nullptr };
	BitmapWriter w;
	_makeBitmapWriter(w, &target, *this);

	auto &support = s_defaultFormats[toInt(fmt)];
	if (support.isWritable() && support.write(_data.data(), w, invert)) {
		return ret;
	} else if (s_defaultFormats[toInt(FileFormat::Png)].write(_data.data(), w, invert)) {
		return ret;
	}
	return memory::PoolInterface::BytesType();
}

template <>
auto BitmapTemplate<memory::StandartInterface>::write(StringView name, bool invert) -> memory::StandartInterface::BytesType {
	BitmapFormat::write_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getWriteFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		memory::StandartInterface::BytesType ret;
		BitmapStdTarget target { &ret, nullptr };
		BitmapWriter w;
		_makeBitmapWriter(w, &target, *this);
		if (fn(_data.data(), w, invert)) {
			return ret;
		}
	}
	return memory::StandartInterface::BytesType();
}

template <>
auto BitmapTemplate<memory::PoolInterface>::write(StringView name, bool invert) -> memory::PoolInterface::BytesType {
	BitmapFormat::write_fn fn = nullptr;

	s_formatListMutex.lock();

	for (auto &it : s_formatList) {
		if (it.getName() == name && it.isWritable()) {
			fn = it.getWriteFn();
		}
	}

	s_formatListMutex.unlock();

	if (fn) {
		memory::PoolInterface::BytesType ret;
		BitmapPoolTarget target { &ret, nullptr };
		BitmapWriter w;
		_makeBitmapWriter(w, &target, *this);
		if (fn(_data.data(), w, invert)) {
			return ret;
		}
	}
	return memory::PoolInterface::BytesType();
}

}
