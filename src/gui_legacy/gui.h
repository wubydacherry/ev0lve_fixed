
#ifndef GUI_LEGACY_GUI_H
#define GUI_LEGACY_GUI_H

#include <sdk/color.h>
#include <sdk/vec2.h>
#include <util/fnv1a.h>

namespace sdk
{
class vec3;
struct vertex_t;
} // namespace sdk

namespace gui_legacy
{
using vec2 = sdk::vec2;
using color = sdk::color;

enum class horizontal_alignment
{
	left,
	center,
	right
};

enum class vertical_alignment
{
	top,
	center,
	bottom
};

class draw_adapter
{
public:
	inline static constexpr auto default_font = FNV1A("segoeui13");

	virtual ~draw_adapter() = default;
	virtual void reset() const = 0;
	virtual void line(const vec2 &from, const vec2 &to, const color &cl, float thickness = 1.f) const = 0;
	virtual void rect(const vec2 &from, const vec2 &to, const color &cl) const = 0;
	virtual void rect_filled(const vec2 &from, const vec2 &to, const color &cl) const = 0;
	virtual void rect_filled_fade(const vec2 &from, const vec2 &to, const color &cl0, const color &cl1,
								  const bool horizontal = false) const = 0;
	virtual void rect_filled_hue(const vec2 &from, const vec2 &to) const = 0;
	virtual void rect_filled_texture(const vec2 &from, const vec2 &to, uint32_t texture) const = 0;
	virtual void circle_filled_fade(const vec2 &center, int32_t radius, const color &cl,
									float progress = 1.f) const = 0;
	virtual void circle(const vec2 &center, int32_t radius, const color &cl, float progress = 1.f,
						bool filled = true) const = 0;
	virtual void polygon(vec2 pos, std::vector<sdk::vertex_t> vertices, const float rotation,
						 const color &cl) const = 0;
	virtual void text(const vec2 &pos, const color &cl, const wchar_t *const text, const uint32_t font = default_font,
					  const horizontal_alignment horizontal = horizontal_alignment::left,
					  const vertical_alignment vertical = vertical_alignment::top) const = 0;
	virtual vec2 get_text_size(const wchar_t *const text, const uint32_t font = default_font) const = 0;
	virtual float get_alpha_modifier() const = 0;
	virtual void set_alpha_modifier(const float modifier) const = 0;
	virtual void weapon(const vec2 &pos, const float height, const color &clr, const char *name) const = 0;
	virtual void icon(const vec2 &pos, const float height, const color &clr, const char *name) const = 0;
	virtual float get_weapon_width(const float height, const char *name) const = 0;
	virtual float get_icon_width(const float height, const char *name) const = 0;
};

class drawable
{
public:
	virtual ~drawable() = default;
	virtual void draw(const draw_adapter &draw) = 0;
};

#define SYMBOL_DROP_DOWN static_cast<wchar_t>(XOR_32(0x25BC))
#define SYMBOL_CHECK_MARK static_cast<wchar_t>(XOR_32(0x2713))
} // namespace gui_legacy

#endif // GUI_LEGACY_GUI_H
