/**
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2015 Chukong Technologies Inc.
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

#ifndef LIBSTAPPLER_MODULE_GEOM_SPCOLOR_H_
#define LIBSTAPPLER_MODULE_GEOM_SPCOLOR_H_

#include "SPCommon.h"
#include "SPVec4.h"

namespace stappler::geom {

#define LAYOUT_COLOR_SPEC_BASE(Name) \
	static Color Name ## _50; \
	static Color Name ## _100; \
	static Color Name ## _200; \
	static Color Name ## _300; \
	static Color Name ## _400; \
	static Color Name ## _500; \
	static Color Name ## _600; \
	static Color Name ## _700; \
	static Color Name ## _800; \
	static Color Name ## _900;

#define LAYOUT_COLOR_SPEC_ACCENT(Name) \
	static Color Name ## _A100; \
	static Color Name ## _A200; \
	static Color Name ## _A400; \
	static Color Name ## _A700;

#define LAYOUT_COLOR_SPEC(Name) \
		LAYOUT_COLOR_SPEC_BASE(Name) \
		LAYOUT_COLOR_SPEC_ACCENT(Name)

struct Color3B;
struct Color4B;
struct Color4F;

enum class ColorMask : uint8_t {
	None = 0,
	R = 0x01,
	G = 0x02,
	B = 0x04,
	A = 0x08,
	Color = 0x07,
	All = 0x0F
};

SP_DEFINE_ENUM_AS_MASK(ColorMask)

bool readColor(const StringView &str, Color4B &color);
bool readColor(const StringView &str, Color3B &color);

/*
 * Based on cocos2d-x sources
 * stappler-cocos2d-x fork use this sources as a replacement of original
 */

/**
 * RGB color composed of bytes 3 bytes.
 */
struct Color3B {
	static Color3B getColorByName(StringView, const Color3B & = Color3B::BLACK);

	Color3B();
	Color3B(uint8_t _r, uint8_t _g, uint8_t _b);
	explicit Color3B(const Color4B& color);
	explicit Color3B(const Color4F& color);

	bool operator==(const Color3B& right) const;
	bool operator==(const Color4B& right) const;
	bool operator==(const Color4F& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4B& right) const;
	bool operator!=(const Color4F& right) const;

	bool equals(const Color3B& other) {
		return (*this == other);
	}

	uint8_t r;
	uint8_t g;
	uint8_t b;

	static const Color3B WHITE;
	static const Color3B BLACK;

	static Color3B progress(const Color3B &a, const Color3B &b, float p);
};

/**
 * RGBA color composed of 4 bytes.
 */
struct Color4B {
	static const Color4B WHITE;
	static const Color4B BLACK;

	static Color4B progress(const Color4B &a, const Color4B &b, float p);
	static Color4B getColorByName(StringView, const Color4B & = Color4B::BLACK);

	Color4B();
	Color4B(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a);
	Color4B(const Color3B& color, uint8_t _a);
	explicit Color4B(const Color3B& color);
	explicit Color4B(const Color4F& color);

	bool operator==(const Color4B& right) const;
	bool operator==(const Color3B& right) const;
	bool operator==(const Color4F& right) const;
	bool operator!=(const Color4B& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4F& right) const;

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	static Color4B white(uint8_t);
	static Color4B black(uint8_t);
};

/**
 * RGBA color composed of 4 floats.
 */
struct alignas(16) Color4F {
	static const Color4F WHITE;
	static const Color4F BLACK;

	static Color4F progress(const Color4F &a, const Color4F &b, float p);

	Color4F();
	Color4F(float _r, float _g, float _b, float _a);
	Color4F(const Color3B& color, uint8_t);
	explicit Color4F(const Color3B& color);
	explicit Color4F(const Color4B& color);

	bool operator==(const Color4F& right) const;
	bool operator==(const Color3B& right) const;
	bool operator==(const Color4B& right) const;
	bool operator!=(const Color4F& right) const;
	bool operator!=(const Color3B& right) const;
	bool operator!=(const Color4B& right) const;

	bool equals(const Color4F &other) {
		return (*this == other);
	}

	operator Vec4() const { return Vec4(r, g, b, a); }

	Color3B getColor() const;
	uint8_t getOpacity() const;

	void setMasked(const Color4F &, ColorMask);
	void setUnmasked(const Color4F &, ColorMask);

	float r;
	float g;
	float b;
	float a;
};

class Color {
public:
	LAYOUT_COLOR_SPEC(Red); // 0
	LAYOUT_COLOR_SPEC(Pink); // 1
	LAYOUT_COLOR_SPEC(Purple); // 2
	LAYOUT_COLOR_SPEC(DeepPurple); // 3
	LAYOUT_COLOR_SPEC(Indigo); // 4
	LAYOUT_COLOR_SPEC(Blue); // 5
	LAYOUT_COLOR_SPEC(LightBlue); // 6
	LAYOUT_COLOR_SPEC(Cyan); // 7
	LAYOUT_COLOR_SPEC(Teal); // 8
	LAYOUT_COLOR_SPEC(Green); // 9
	LAYOUT_COLOR_SPEC(LightGreen); // 10
	LAYOUT_COLOR_SPEC(Lime); // 11
	LAYOUT_COLOR_SPEC(Yellow); // 12
	LAYOUT_COLOR_SPEC(Amber); // 13
	LAYOUT_COLOR_SPEC(Orange); // 14
	LAYOUT_COLOR_SPEC(DeepOrange); // 15

	LAYOUT_COLOR_SPEC_BASE(Brown); // 16
	LAYOUT_COLOR_SPEC_BASE(Grey); // 17
	LAYOUT_COLOR_SPEC_BASE(BlueGrey); // 18

	static Color White;
	static Color Black;

	enum class Level : int16_t {
		Unknown = -1,
		b50 = 0,
		b100,
		b200,
		b300,
		b400,
		b500,
		b600,
		b700,
		b800,
		b900,
		a100,
		a200,
		a400,
		a700
	};

	enum class Tone : int16_t {
		Unknown = -1,
		Red = 0,
		Pink,
		Purple,
		DeepPurple,
		Indigo,
		Blue,
		LightBlue,
		Cyan,
		Teal,
		Green,
		LightGreen,
		Lime,
		Yellow,
		Amber,
		Orange,
		DeepOrange,
		Brown,
		Grey,
		BlueGrey,
		BlackWhite,
	};

	inline Color3B asColor3B() const {
		return Color3B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF);
	}

	inline Color4B asColor4B() const {
		return Color4B((_value >> 16) & 0xFF, (_value >> 8) & 0xFF, _value & 0xFF, 255);
	}

	inline Color4F asColor4F() const {
		return Color4F(
				float((_value >> 16) & 0xFF) / float(0xFF),
				float((_value >> 8) & 0xFF) / float(0xFF),
				float(_value & 0xFF) / float(0xFF),
				1.0f);
	}

	inline operator Color3B () const {
		return asColor3B();
	}
	inline operator Color4B () const {
		return asColor4B();
	}
	inline operator Color4F () const {
		return asColor4F();
	}

	inline bool operator == (const Color &other) const {
		return _value == other._value;
	}

	inline bool operator != (const Color &other) const {
		return _value != other._value;
	}

	inline uint8_t r() const { return (_value >> 16) & 0xFF; }
	inline uint8_t g() const { return (_value >> 8) & 0xFF; }
	inline uint8_t b() const { return _value  & 0xFF; }

	inline uint32_t value() const { return _value; }
	inline uint32_t index() const { return _index; }

	Color text() const;

	inline Level level() const { return (_index == maxOf<uint16_t>())?Level::Unknown:((Level)(_index & 0x0F)); }
	inline Tone tone() const { return (_index == maxOf<uint16_t>())?Tone::Unknown:((Tone)((_index & 0xFFF0) / 16)); }

	Color previous() const;
	Color next() const;

	Color lighter(uint8_t index = 1) const;
	Color darker(uint8_t index = 1) const;

	Color medium() const;
	Color specific(uint8_t index) const;
	Color specific(Level l) const;

	Color(Tone, Level);
	Color(uint32_t value);
	Color(uint32_t value, int16_t index);
	Color(const Color3B &color);
	Color(const Color4B &color);

	Color() = default;

	Color(const Color &) = default;
	Color(Color &&) = default;
	Color & operator=(const Color &) = default;
	Color & operator=(Color &&) = default;

	template <typename Interface>
	auto name() const -> typename Interface::StringType;

	static Color getColorByName(const StringView &, const Color & = Color::Black);
	static Color progress(const Color &a, const Color &b, float p);

private:
	static Color getById(uint16_t index);
	static uint16_t getColorIndex(uint32_t);

	uint32_t _value = 0;
	uint16_t _index = uint16_t(19 * 16 + 1);
};

std::ostream & operator<<(std::ostream & stream, const Color & obj);
std::ostream & operator<<(std::ostream & stream, const Color3B & obj);
std::ostream & operator<<(std::ostream & stream, const Color4B & obj);
std::ostream & operator<<(std::ostream & stream, const Color4F & obj);

namespace simd_inline {

#if SP_GEOM_DEFAULT_SIMD == SP_GEOM_DEFAULT_SIMD_NEON

inline void multiplyColor4F_Inline (const Color4F &a, const Color4F &b, Color4F &dst) {
	simde_vst1q_f32(&dst.r,
		simde_vmulq_f32(
			simde_vld1q_f32(&a.r),
			simde_vld1q_f32(&b.r)));
}

inline void divideColor4F_Inline (const Color4F &a, const Color4F &b, Color4F &dst) {
#if defined(SIMDE_ARM_NEON_A64V8_NATIVE)
	simde_vst1q_f32(&dst.r,
		vdivq_f32(
			simde_vld1q_f32(&a.r),
			simde_vld1q_f32(&b.r)));
#else
	// vdivq_f32 is not defied in simde, use SSE-based replacement
	simde_mm_store_ps(&dst.r,
		simde_mm_div_ps(
			simde_mm_load_ps(&a.r),
			simde_mm_load_ps(&b.r)));
#endif
}

#else

inline void multiplyColor4F_Inline (const Color4F &a, const Color4F &b, Color4F &dst) {
	simde_mm_store_ps(&dst.r,
		simde_mm_mul_ps(
			simde_mm_load_ps(&a.r),
			simde_mm_load_ps(&b.r)));
}

inline void divideColor4F_Inline (const Color4F &a, const Color4F &b, Color4F &dst) {
	simde_mm_store_ps(&dst.r,
		simde_mm_div_ps(
			simde_mm_load_ps(&a.r),
			simde_mm_load_ps(&b.r)));
}

#endif

}

inline Color4F operator*(const Color4F &l, const Color4F &r) {
	Color4F dst;
	simd_inline::multiplyColor4F_Inline(l, r, dst);
	return dst;
}

inline Color4F operator/(const Color4F &l, const Color4F &r) {
	Color4F dst;
	simd_inline::divideColor4F_Inline(l, r, dst);
	return dst;
}

inline Color4F operator*(const Color4F &l, const Color4B &r) {
	return l * Color4F(r);
}

inline Color4F operator*(const Color4B &l, const Color4F &r) {
	return Color4F(l) * r;
}

inline Color4F operator/(const Color4F &l, const Color4B &r) {
	return l / Color4F(r);
}

inline Color4F operator/(const Color4B &l, const Color4F &r) {
	return Color4F(l) / r;
}

}

namespace stappler {

template <> inline
geom::Color progress<geom::Color>(const geom::Color &a, const geom::Color &b, float p) {
	return geom::Color::progress(a, b, p);
}

template <> inline
geom::Color3B progress<geom::Color3B>(const geom::Color3B &a, const geom::Color3B &b, float p) {
	return geom::Color3B::progress(a, b, p);
}

template <> inline
geom::Color4B progress<geom::Color4B>(const geom::Color4B &a, const geom::Color4B &b, float p) {
	return geom::Color4B::progress(a, b, p);
}

template <> inline
geom::Color4F progress<geom::Color4F>(const geom::Color4F &a, const geom::Color4F &b, float p) {
	return geom::Color4F::progress(a, b, p);
}

}

#endif /* LIBSTAPPLER_MODULE_GEOM_SPCOLOR_H_ */
