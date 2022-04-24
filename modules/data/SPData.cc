// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPData.h"

#include "SPStringView.h"
#include "SPString.h"

#ifdef MODULE_COMMON_FILESYSTEM
#include "SPFilesystem.h"
#endif

#define LZ4_HC_STATIC_LINKING_ONLY 1
#include "lz4hc.h"

#ifdef MODULE_COMMON_BROTLI_LIB
#include "brotli/encode.h"
#include "brotli/decode.h"
#endif

namespace stappler::data {

EncodeFormat EncodeFormat::CborCompressed(EncodeFormat::Cbor, EncodeFormat::LZ4HCCompression);
EncodeFormat EncodeFormat::JsonCompressed(EncodeFormat::Json, EncodeFormat::LZ4HCCompression);

int EncodeFormat::EncodeStreamIndex = std::ios_base::xalloc();

namespace serenity {

bool shouldEncodePercent(char c) {
#define V16 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	static uint8_t s_decTable[256] = {
		V16, V16, // 0-1, 0-F
		1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, // 2, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, // 3, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, // 5, 0-F
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6, 0-F
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, // 7, 0-F
		V16, V16, V16, V16, V16, V16, V16, V16,
	};

	return bool(s_decTable[*((uint8_t *)(&c))]);
}

}

template <>
template <>
auto ValueTemplate<memory::PoolInterface>::convert<memory::PoolInterface>() const -> ValueTemplate<memory::PoolInterface> {
	return ValueTemplate<memory::PoolInterface>(*this);
}

template <>
template <>
auto ValueTemplate<memory::StandartInterface>::convert<memory::StandartInterface>() const -> ValueTemplate<memory::StandartInterface> {
	return ValueTemplate<memory::StandartInterface>(*this);
}

template <>
template <>
auto ValueTemplate<memory::PoolInterface>::convert<memory::StandartInterface>() const -> ValueTemplate<memory::StandartInterface> {
	switch (_type) {
	case Type::INTEGER: return ValueTemplate<memory::StandartInterface>(intVal); break;
	case Type::DOUBLE: return ValueTemplate<memory::StandartInterface>(doubleVal); break;
	case Type::BOOLEAN: return ValueTemplate<memory::StandartInterface>(boolVal); break;
	case Type::CHARSTRING:
		return ValueTemplate<memory::StandartInterface>(memory::StandartInterface::StringType(strVal->data(), strVal->size()));
		break;
	case Type::BYTESTRING:
		return ValueTemplate<memory::StandartInterface>(memory::StandartInterface::BytesType(bytesVal->data(), bytesVal->data() + bytesVal->size()));
		break;
	case Type::ARRAY: {
		ValueTemplate<memory::StandartInterface> ret(ValueTemplate<memory::StandartInterface>::Type::ARRAY);
		auto &arr = ret.asArray();
		arr.reserve(arrayVal->size());
		for (auto &it : *arrayVal) {
			arr.emplace_back(it.convert<memory::StandartInterface>());
		}
		return ret;
		break;
	}
	case Type::DICTIONARY: {
		ValueTemplate<memory::StandartInterface> ret(ValueTemplate<memory::StandartInterface>::Type::DICTIONARY);
		auto &dict = ret.asDict();
		for (auto &it : *dictVal) {
			dict.emplace(StringView(it.first).str<memory::StandartInterface>(), it.second.convert<memory::StandartInterface>());
		}
		return ret;
		break;
	}
	default:
		break;
	}
	return ValueTemplate<memory::StandartInterface>();
}

template <>
template <>
auto ValueTemplate<memory::StandartInterface>::convert<memory::PoolInterface>() const -> ValueTemplate<memory::PoolInterface> {
	switch (_type) {
	case Type::INTEGER: return ValueTemplate<memory::PoolInterface>(intVal); break;
	case Type::DOUBLE: return ValueTemplate<memory::PoolInterface>(doubleVal); break;
	case Type::BOOLEAN: return ValueTemplate<memory::PoolInterface>(boolVal); break;
	case Type::CHARSTRING:
		return ValueTemplate<memory::PoolInterface>(memory::PoolInterface::StringType(strVal->data(), strVal->size()));
		break;
	case Type::BYTESTRING:
		return ValueTemplate<memory::PoolInterface>(memory::PoolInterface::BytesType(bytesVal->data(), bytesVal->data() + bytesVal->size()));
		break;
	case Type::ARRAY: {
		ValueTemplate<memory::PoolInterface> ret(ValueTemplate<memory::PoolInterface>::Type::ARRAY);
		auto &arr = ret.asArray();
		arr.reserve(arrayVal->size());
		for (auto &it : *arrayVal) {
			arr.emplace_back(it.convert<memory::PoolInterface>());
		}
		return ret;
		break;
	}
	case Type::DICTIONARY: {
		ValueTemplate<memory::PoolInterface> ret(ValueTemplate<memory::PoolInterface>::Type::DICTIONARY);
		auto &dict = ret.asDict();
		dict.reserve(dictVal->size());
		for (auto &it : *dictVal) {
			dict.emplace(StringView(it.first).str<memory::PoolInterface>(), it.second.convert<memory::PoolInterface>());
		}
		return ret;
		break;
	}
	default:
		break;
	}
	return ValueTemplate<memory::PoolInterface>();
}

size_t getCompressBounds(size_t size, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression:
	case EncodeFormat::LZ4HCCompression: {
		if (size < LZ4_MAX_INPUT_SIZE) {
			return LZ4_compressBound(size) + ((size <= 0xFFFF) ? 2 : 4);
		}
		return 0;
		break;
	}
#ifdef MODULE_COMMON_BROTLI_LIB
	case EncodeFormat::Brotli:
		if (size < LZ4_MAX_INPUT_SIZE) {
			return BrotliEncoderMaxCompressedSize(size) + ((size <= 0xFFFF) ? 2 : 4);
		}
		return 0;
		break;
#endif
	case EncodeFormat::NoCompression:
		break;
	}
	return 0;
}

thread_local uint8_t tl_lz4HCEncodeState[std::max(sizeof(LZ4_streamHC_t), sizeof(LZ4_stream_t))];
thread_local uint8_t tl_compressBuffer[128_KiB];

uint8_t *getLZ4EncodeState() {
	return tl_lz4HCEncodeState;
}

size_t compressData(const uint8_t *src, size_t srcSize, uint8_t *dest, size_t destSize, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression: {
		const int offSize = ((srcSize <= 0xFFFF) ? 2 : 4);
		const int ret = LZ4_compress_fast_extState(tl_lz4HCEncodeState, (const char *)src, (char *)dest + offSize, srcSize, destSize - offSize, 1);
		if (ret > 0) {
			if (srcSize <= 0xFFFF) {
				uint16_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			} else {
				uint32_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			}
			return ret + offSize;
		}
		break;
	}
	case EncodeFormat::LZ4HCCompression: {
		const int offSize = ((srcSize <= 0xFFFF) ? 2 : 4);
		const int ret = LZ4_compress_HC_extStateHC(tl_lz4HCEncodeState, (const char *)src, (char *)dest + offSize, srcSize, destSize - offSize, LZ4HC_CLEVEL_MAX);
		if (ret > 0) {
			if (srcSize <= 0xFFFF) {
				uint16_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			} else {
				uint32_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			}
			return ret + offSize;
		}
		break;
	}
#ifdef MODULE_COMMON_BROTLI_LIB
	case EncodeFormat::Brotli: {
		const int offSize = ((srcSize <= 0xFFFF) ? 2 : 4);
		size_t ret = destSize - offSize;
		if (BrotliEncoderCompress(BROTLI_MAX_QUALITY, BROTLI_MAX_WINDOW_BITS, BROTLI_DEFAULT_MODE,
				srcSize, (const uint8_t *)src, &ret,dest + offSize) == BROTLI_TRUE) {
			if (srcSize <= 0xFFFF) {
				uint16_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			} else {
				uint32_t sz = srcSize;
				memcpy(dest, &sz, sizeof(sz));
			}
			return ret + offSize;
		}
		break;
	}
#endif
	case EncodeFormat::NoCompression:
		break;
	}
	return 0;
}

void writeCompressionMark(uint8_t *data, size_t sourceSize, EncodeFormat::Compression c) {
	switch (c) {
	case EncodeFormat::LZ4Compression:
	case EncodeFormat::LZ4HCCompression:
		if (sourceSize <= 0xFFFF) {
			memcpy(data, "LZ4S", 4);
		} else {
			memcpy(data, "LZ4W", 4);
		}
		break;
#ifdef MODULE_COMMON_BROTLI_LIB
	case EncodeFormat::Brotli:
		if (sourceSize <= 0xFFFF) {
			memcpy(data, "SBrS", 4);
		} else {
			memcpy(data, "SBrW", 4);
		}
		break;
#endif
	case EncodeFormat::NoCompression:
		break;
	}
}

template <typename Interface>
static inline auto doCompress(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> typename Interface::BytesType {
	auto bufferSize = getCompressBounds(size, c);
	if (bufferSize == 0) {
		return typename Interface::BytesType();
	} else if (bufferSize <= sizeof(tl_compressBuffer)) {
		auto encodeSize = compressData(src, size, tl_compressBuffer, sizeof(tl_compressBuffer), c);
		if (encodeSize == 0 || (conditional && encodeSize + 4 > size)) { return typename Interface::BytesType(); }
		typename Interface::BytesType ret; ret.resize(encodeSize + 4);
		writeCompressionMark(ret.data(), size, c);
		memcpy(ret.data() + 4, tl_compressBuffer, encodeSize);
		return ret;
	} else {
		typename Interface::BytesType ret; ret.resize(bufferSize + 4);
		auto encodeSize = compressData(src, size, ret.data() + 4, bufferSize, c);
		if (encodeSize == 0 || (conditional && encodeSize + 4 > size)) { return typename Interface::BytesType(); }
		writeCompressionMark(ret.data(), size, c);
		ret.resize(encodeSize + 4);
		ret.shrink_to_fit();
		return ret;
	}
	return typename Interface::BytesType();
}

template <>
auto compress<memory::PoolInterface>(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> memory::PoolInterface::BytesType {
	return doCompress<memory::PoolInterface>(src, size, c, conditional);
}

template <>
auto compress<memory::StandartInterface>(const uint8_t *src, size_t size, EncodeFormat::Compression c, bool conditional) -> memory::StandartInterface::BytesType {
	return doCompress<memory::StandartInterface>(src, size, c, conditional);
}

using decompress_ptr = const uint8_t *;

static bool doDecompressLZ4Frame(const uint8_t *src, size_t srcSize, uint8_t *dest, size_t destSize) {
	return LZ4_decompress_safe((const char *)src, (char *)dest, srcSize, destSize) > 0;
}

template <typename Interface>
static inline auto doDecompressLZ4(BytesView data, bool sh) -> ValueTemplate<Interface> {
	size_t size = sh ? data.readUnsigned16() : data.readUnsigned32();

	ValueTemplate<Interface> ret;
	if (size <= sizeof(tl_compressBuffer)) {
		if (doDecompressLZ4Frame(data.data(), data.size(), tl_compressBuffer, size)) {
			ret = data::read<Interface>(BytesView(tl_compressBuffer, size));
		}
	} else {
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			ret = data::read<Interface>(res);
		}
	}
	return ret;
}

template <>
auto decompressLZ4(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::PoolInterface> {
	return doDecompressLZ4<memory::PoolInterface>(BytesView(srcPtr, srcSize), sh);
}

template <>
auto decompressLZ4(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::StandartInterface> {
	return doDecompressLZ4<memory::StandartInterface>(BytesView(srcPtr, srcSize), sh);
}

#ifdef MODULE_COMMON_BROTLI_LIB
static bool doDecompressBrotliFrame(const uint8_t *src, size_t srcSize, uint8_t *dest, size_t destSize) {
	size_t ret = destSize;
	return BrotliDecoderDecompress(srcSize, src, &ret, dest) == BROTLI_DECODER_RESULT_SUCCESS;
}
template <typename Interface>
static inline auto doDecompressBrotli(BytesView data, bool sh) -> ValueTemplate<Interface> {
	size_t size = sh ? data.readUnsigned16() : data.readUnsigned32();

	ValueTemplate<Interface> ret;
	if (size <= sizeof(tl_compressBuffer)) {
		if (doDecompressBrotliFrame(data.data(), data.size(), tl_compressBuffer, size)) {
			ret = data::read<Interface>(BytesView(tl_compressBuffer, size));
		}
	} else {
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressBrotliFrame(data.data(), data.size(), res.data(), size)) {
			ret = data::read<Interface>(res);
		}
	}
	return ret;
}

template <>
auto decompressBrotli(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::PoolInterface> {
	return doDecompressBrotli<memory::PoolInterface>(BytesView(srcPtr, srcSize), sh);
}

template <>
auto decompressBrotli(const uint8_t *srcPtr, size_t srcSize, bool sh) -> ValueTemplate<memory::StandartInterface> {
	return doDecompressBrotli<memory::StandartInterface>(BytesView(srcPtr, srcSize), sh);
}

#endif

template <typename Interface>
static inline auto doDecompress(const uint8_t *d, size_t size) -> typename Interface::BytesType {
	BytesView data(d, size);
	auto ff = detectDataFormat(data.data(), data.size());
	switch (ff) {
	case DataFormat::LZ4_Short: {
		data += 4;
		size_t size = data.readUnsigned16();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
	case DataFormat::LZ4_Word: {
		data += 4;
		size_t size = data.readUnsigned32();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressLZ4Frame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
#ifdef MODULE_COMMON_BROTLI_LIB
	case DataFormat::Brotli_Short: {
		data += 4;
		size_t size = data.readUnsigned16();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressBrotliFrame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
	case DataFormat::Brotli_Word: {
		data += 4;
		size_t size = data.readUnsigned32();
		typename Interface::BytesType res; res.resize(size);
		if (doDecompressBrotliFrame(data.data(), data.size(), res.data(), size)) {
			return res;
		}
		break;
	}
#endif
	default: break;
	}
	return typename Interface::BytesType();
}

template <>
auto decompress<memory::PoolInterface>(const uint8_t *d, size_t size) -> typename memory::PoolInterface::BytesType {
	return doDecompress<memory::PoolInterface>(d, size);
}

template <>
auto decompress<memory::StandartInterface>(const uint8_t *d, size_t size) -> typename memory::StandartInterface::BytesType {
	return doDecompress<memory::StandartInterface>(d, size);
}

}
