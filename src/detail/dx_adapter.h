
#ifndef DETAIL_DX_ADAPTER_H
#define DETAIL_DX_ADAPTER_H

#include <gui_legacy/gui.h>
#include <mutex>
#include <ren/renderer.h>
#include <ren/types/texture.h>

namespace detail
{
class dx_adapter : public gui_legacy::draw_adapter
{
	using vec2 = gui_legacy::vec2;
	using color = gui_legacy::color;

public:
	struct icon_texture
	{
		std::shared_ptr<evo::ren::texture> texture;
		float width_to_height;
	};

	inline static constexpr uint8_t esp_layer = 16;
	inline static std::unordered_map<uint32_t, icon_texture> icon_textures = {};

	static void destroy_objects();
	static void create_objects();

	void reset() const final {}
	void line(const vec2 &from, const vec2 &to, const color &cl, float thickness = 1.f) const final;
	void rect(const vec2 &from, const vec2 &to, const color &cl) const final;
	void rect_filled(const vec2 &from, const vec2 &to, const color &cl) const final;
	void rect_filled_fade(const vec2 &from, const vec2 &to, const color &cl0, const color &cl1,
						  const bool horizontal = false) const final;
	void circle(const vec2 &center, int32_t radius, const color &cl, float progress = 1.f,
				bool filled = true) const final;
	void circle_filled_fade(const vec2 &center, int32_t radius, const color &cl, float progress = 1.f) const final;
	void polygon(vec2 pos, std::vector<sdk::vertex_t> vertices, const float rotation, const color &cl) const final;
	void text(const vec2 &pos, const color &cl, const wchar_t *const text, const uint32_t font = default_font,
			  const gui_legacy::horizontal_alignment horizontal = gui_legacy::horizontal_alignment::left,
			  const gui_legacy::vertical_alignment vertical = gui_legacy::vertical_alignment::top) const final;
	vec2 get_text_size(const wchar_t *const text, const uint32_t font = default_font) const final;
	float get_alpha_modifier() const final;
	void set_alpha_modifier(const float modifier) const final;
	std::shared_ptr<evo::ren::texture> load_svg_to_texture(const uint8_t *buffer, size_t size, uint32_t w,
														   uint32_t h) const;
	std::shared_ptr<evo::ren::texture> load_png_to_texture(const uint8_t *buffer) const;
	std::optional<icon_texture> icon_to_texture(const char *name, uint32_t target_height = 0) const;
	void weapon(const vec2 &pos, const float height, const color &clr, const char *name) const final;
	void icon(const vec2 &pos, const float height, const color &clr, const char *name) const final;
	float get_weapon_width(const float height, const char *name) const final;
	float get_icon_width(const float height, const char *name) const final;

	void rect_filled_hue(const vec2 &from, const vec2 &to) const final {}
	void rect_filled_texture(const vec2 &from, const vec2 &to, uint32_t texture) const final {}

	static void swap_buffers();
	static void clear_buffer();

	inline static std::mutex mtx{};
	inline static std::atomic_bool ready{};
};
} // namespace detail

extern std::shared_ptr<evo::ren::layer> buf;

#endif // DETAIL_DX_ADAPTER_H
