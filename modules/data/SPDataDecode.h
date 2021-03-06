/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef MODULES_DATA_SPDATADECODE_H_
#define MODULES_DATA_SPDATADECODE_H_

//#include "SPFilesystem.h"
#include "SPDataDecodeCbor.h"
#include "SPDataDecodeJson.h"
#include "SPDataDecodeSerenity.h"

namespace stappler::data {

enum class DataFormat {
	Unknown,
	Json,
	Cbor,
	Serenity,

	CborBase64,

	LZ4_Short,
	LZ4_Word,
#ifdef MODULE_COMMON_BROTLI_LIB
	Brotli_Short,
	Brotli_Word,
#endif

	// for future implementations
	// Encrypt,
};

inline DataFormat detectDataFormat(const uint8_t *ptr, size_t size) {
	if (size > 3 && ptr[0] == 0xd9 && ptr[1] == 0xd9 && ptr[2] == 0xf7) {
		return DataFormat::Cbor;
	} else if (size > 4 && ptr[0] == '2' && ptr[1] == 'd' && ptr[2] == 'n' && ptr[3] == '3') {
		return DataFormat::CborBase64;
	} else if (size > 3 && ptr[0] == 'L' && ptr[1] == 'Z' && ptr[2] == '4') {
		if (ptr[3] == 'S') {
			return DataFormat::LZ4_Short;
		} else if (ptr[3] == 'W') {
			return DataFormat::LZ4_Word;
		}
#ifdef MODULE_COMMON_BROTLI_LIB
	} else if (size > 3 && ptr[0] == 'S' && ptr[1] == 'B' && ptr[2] == 'r') {
		if (ptr[3] == 'S') {
			return DataFormat::Brotli_Short;
		} else if (ptr[3] == 'W') {
			return DataFormat::Brotli_Word;
		}
#endif
	} else if (ptr[0] == '(') {
		return DataFormat::Serenity;
	} else {
		return DataFormat::Json;
	}
	return DataFormat::Unknown;
}

template <typename Interface>
auto decompressLZ4(const uint8_t *, size_t, bool sh) -> ValueTemplate<Interface>;

template <typename Interface>
auto decompressBrotli(const uint8_t *, size_t, bool sh) -> ValueTemplate<Interface>;

template <typename Interface>
auto decompress(const uint8_t *, size_t) -> typename Interface::BytesType;

template <typename Interface, typename StringType>
auto read(const StringType &data, const StringView &key = StringView()) -> ValueTemplate<Interface> {
	if (data.size() == 0) {
		return ValueTemplate<Interface>();
	}
	auto ff = detectDataFormat((const uint8_t *)data.data(), data.size());
	switch (ff) {
	case DataFormat::Cbor:
		return cbor::read<Interface>(data);
		break;
	case DataFormat::Json:
		return json::read<Interface>(StringView((char *)data.data(), data.size()));
		break;
	case DataFormat::Serenity:
		return serenity::read<Interface>(StringView((char *)data.data(), data.size()));
		break;
	case DataFormat::CborBase64:
		return read<Interface>(base64::decode<Interface>(CoderSource(data)), key);
		break;
	case DataFormat::LZ4_Short:
		return decompressLZ4<Interface>((const uint8_t *)data.data() + 4, data.size() - 4, true);
		break;
	case DataFormat::LZ4_Word:
		return decompressLZ4<Interface>((const uint8_t *)data.data() + 4, data.size() - 4, false);
		break;
#ifdef MODULE_COMMON_BROTLI_LIB
	case DataFormat::Brotli_Short:
		return decompressBrotli<Interface>((const uint8_t *)data.data() + 4, data.size() - 4, true);
		break;
	case DataFormat::Brotli_Word:
		return decompressBrotli<Interface>((const uint8_t *)data.data() + 4, data.size() - 4, false);
		break;
#endif
	default:
		break;
	}
	return ValueTemplate<Interface>();
}

#ifdef MODULE_COMMON_FILESYSTEM
template <typename Interface>
auto readFile(StringView filename, const StringView &key = StringView()) -> ValueTemplate<Interface> {
	return read<Interface>(filesystem::readIntoMemory<Interface>(filename));
}
#endif
}

#endif /* MODULES_DATA_SPDATADECODE_H_ */
