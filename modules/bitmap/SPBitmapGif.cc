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

#include "SPBitmapFormat.h"
#include "SPLog.h"
#include "gif_lib.h"

#include "SPFilesystem.h"

namespace stappler::bitmap::gif {

static bool isGif(const uint8_t * data, size_t dataLen) {
	if (dataLen <= 8) {
		return false;
	}

	static const unsigned char GIF_SIGNATURE_1[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61};
	static const unsigned char GIF_SIGNATURE_2[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61};
	return memcmp(GIF_SIGNATURE_1, data, sizeof(GIF_SIGNATURE_1)) == 0
			|| memcmp(GIF_SIGNATURE_2, data, sizeof(GIF_SIGNATURE_2)) == 0;
}

static bool getGifImageSize(const io::Producer &file, StackBuffer<512> &data, size_t &width, size_t &height) {
	if (isGif(data.data(), data.size())) {
		auto reader = BytesViewTemplate<Endian::Little>(data.data() + 6, 4);

		width = reader.readUnsigned16();
		height = reader.readUnsigned16();

		return true;
	}
	return false;
}

static int Gif_InputFunc(GifFileType *file, GifByteType *bytes, int count) {
	auto reader = (CoderSource *)file->UserData;

	if (count >= 0) {
		return reader->read(bytes, count);
	}
	return 0;
}

static bool loadGif(const uint8_t *inputData, size_t size, BitmapWriter &outputData) {
	CoderSource reader(inputData, size);

	int error = 0;
	auto f = DGifOpen((void *)&reader, &Gif_InputFunc, &error);

	if (!f || error != 0) {
		if (f) {
			DGifCloseFile(f, &error);
		}
		log::text("GIF", "fail to open file");
		return false;
	}

	if (DGifSlurp(f) != GIF_OK) {
		DGifCloseFile(f, &error);
		log::text("GIF", "fail to read file");
		return false;
	}

	if (f->ImageCount == 0) {
		DGifCloseFile(f, &error);
		log::text("GIF", "no images found");
		return false;
	}

	ColorMapObject *colors =  (f->SavedImages->ImageDesc.ColorMap) ? f->SavedImages->ImageDesc.ColorMap : f->SColorMap;
	if (!colors) {
		DGifCloseFile(f, &error);
		log::text("GIF", "no color profile found");
		return false;
	}

	auto checkGrayscale = [&] (GifColorType &c) {
		return c.Red == c.Green && c.Red == c.Blue;
	};

	bool isGrayscale = true;
	for (size_t i = 0; i < size_t(colors->ColorCount); ++ i) {
		if (!checkGrayscale(colors->Colors[i])) {
			isGrayscale = false;
			break;
		}
	}

	size_t transparent = maxOf<size_t>();
	if (f->ExtensionBlockCount > 0) {
		for (size_t i = 0; i < size_t(f->ExtensionBlockCount); ++ i) {
			GraphicsControlBlock GCB;
			if (DGifExtensionToGCB(f->ExtensionBlocks[i].ByteCount, f->ExtensionBlocks[i].Bytes, &GCB) == GIF_OK) {
				if (GCB.TransparentColor != NO_TRANSPARENT_COLOR) {
					transparent = GCB.TransparentColor;
				}
			}
		}
	}

	if (f->SavedImages->ExtensionBlockCount > 0) {
		GraphicsControlBlock GCB;
		if (DGifSavedExtensionToGCB(f, 0, &GCB) == GIF_OK) {
			if (GCB.TransparentColor != NO_TRANSPARENT_COLOR) {
				transparent = GCB.TransparentColor;
			}
		}
	}

	outputData.width = f->SavedImages->ImageDesc.Width;
	outputData.height = f->SavedImages->ImageDesc.Height;

	outputData.color = (transparent != maxOf<size_t>())
			? (isGrayscale ? PixelFormat::IA88 : PixelFormat::RGBA8888)
			: (isGrayscale ? ((outputData.color == PixelFormat::A8) ? PixelFormat::A8 : PixelFormat::I8) : PixelFormat::RGB888);

	outputData.alpha = (transparent != maxOf<size_t>() || outputData.color == PixelFormat::A8)
			? AlphaFormat::Unpremultiplied
			: AlphaFormat::Opaque;

	if (outputData.getStride) {
		outputData.stride = (uint32_t)outputData.getStride(outputData.target, outputData.color, outputData.width);
	}
	outputData.stride = max(outputData.stride, (uint32_t)(outputData.width * getBytesPerPixel(outputData.color)));

    auto dataLen = outputData.stride * outputData.height;
    outputData.resize(outputData.target, dataLen);

	auto input = f->SavedImages->RasterBits;
	auto location = outputData.getData(outputData.target, 0);

	if (outputData.color == PixelFormat::RGB888) {
		for (size_t i = 0; i < outputData.height; ++ i) {
			auto loc = location;
			for (size_t j = 0; j < outputData.width; ++ j) {
				auto &c = colors->Colors[input[i * outputData.width + j]];
				*loc++ = c.Red;
				*loc++ = c.Green;
				*loc++ = c.Blue;
			}
			location += outputData.stride;
		}
	} else if (outputData.color == PixelFormat::A8 || outputData.color == PixelFormat::I8) {
		for (size_t i = 0; i < outputData.height; ++ i) {
			auto loc = location;
			for (size_t j = 0; j < outputData.width; ++ j) {
				auto &c = colors->Colors[input[i * outputData.width + j]];
				*loc++ = c.Red;
			}
			location += outputData.stride;
		}
	} else if (outputData.color == PixelFormat::IA88) {
		for (size_t i = 0; i < outputData.height; ++ i) {
			auto loc = location;
			for (size_t j = 0; j < outputData.width; ++ j) {
				auto idx = input[i * outputData.width + j];
				*loc++ = colors->Colors[idx].Red;
				if (idx == transparent) {
					*loc++ = 0;
				} else {
					*loc++ = 255;
				}
			}
			location += outputData.stride;
		}
	} else if (outputData.color == PixelFormat::RGBA8888) {
		for (size_t i = 0; i < outputData.height; ++ i) {
			auto loc = location;
			for (size_t j = 0; j < outputData.width; ++ j) {
				auto idx = input[i * outputData.width + j];
				auto &c = colors->Colors[idx];
				*loc++ = c.Red;
				*loc++ = c.Green;
				*loc++ = c.Blue;
				if (idx == transparent) {
					*loc++ = 0;
				} else {
					*loc++ = 255;
				}
			}
			location += outputData.stride;
		}
	}

	DGifCloseFile(f, &error);
	return true;
}

}
