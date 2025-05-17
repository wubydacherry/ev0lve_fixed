
#ifndef SDK_COLOR_H
#define SDK_COLOR_H

#undef MAKE_COLOR
#define MAKE_COLOR(name, r, g, b, a)                                                                                   \
	inline static auto name() { return color(r, g, b, a); }

#define and &&
#define and_eq &=
#define bitand &
#define bitor |
#define compl ~
#define not !
#define not_eq !=
#define or ||
#define or_eq |=
#define xor ^
#define xor_eq ^=

#include <ren/types/color.h>

namespace sdk
{
class color
{
public:
	MAKE_COLOR(black, 0, 0, 0, 255)
	MAKE_COLOR(white, 255, 255, 255, 255)
	MAKE_COLOR(attention, 240, 10, 10, 255)
	MAKE_COLOR(info, 63, 132, 242, 255)
	MAKE_COLOR(warning, 240, 194, 10, 255)
	MAKE_COLOR(health_start, 122, 255, 112, 255)
	MAKE_COLOR(health_end, 247, 49, 0, 255)
	MAKE_COLOR(armor, 72, 206, 255, 255)
	MAKE_COLOR(ammo, 122, 74, 218, 255)
	MAKE_COLOR(detonation, 245, 99, 66, 150)

	color() : color(0, 0, 0, 0) {}
	color(const evo::ren::color &c) : color(c.value.r * 255.f, c.value.g * 255.f, c.value.b * 255.f, c.value.a * 255.f)
	{
	}
	color(const uint8_t red, const uint8_t green, const uint8_t blue) : color(red, green, blue, 255) {}
	color(const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha)
		: r(red), g(green), b(blue), a(alpha)
	{
	}

	inline uint8_t red() const { return this->r; }
	inline uint8_t green() const { return this->g; }
	inline uint8_t blue() const { return this->b; }
	inline uint8_t alpha() const { return this->a; }
	inline color alpha(float alpha) const { return color(r, g, b, static_cast<uint8_t>(alpha * a)); }
	inline uint32_t encoded() const { return *reinterpret_cast<const uint32_t *>(this); }

private:
	uint8_t r, g, b, a;
};
} // namespace sdk

#undef MAKE_COLOR

#endif // SDK_COLOR_H
