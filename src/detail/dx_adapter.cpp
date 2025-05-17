
#include <detail/dx_adapter.h>
#include <ren/misc.h>
#include <sdk/math.h>
#include <sdk/misc.h>
#include <sdk/surface.h>

using namespace detail;
using namespace evo;
using namespace sdk;

std::shared_ptr<ren::layer> buf{};

void dx_adapter::line(const vec2 &from, const vec2 &to, const color &cl, float thickness) const
{
	buf->g.anti_alias = true;
	buf->add_line(ren::vec2(from.x, from.y).floor(), ren::vec2(to.x, to.y).floor(),
				  ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()), thickness);
	buf->g.anti_alias = false;
}

void dx_adapter::rect(const vec2 &from, const vec2 &to, const color &cl) const
{
	buf->add_rect(ren::rect({from.x, from.y}, {to.x, to.y}).floor(),
				  ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()));
}

void dx_adapter::rect_filled(const vec2 &from, const vec2 &to, const color &cl) const
{
	buf->add_rect_filled(ren::rect({from.x, from.y}, {to.x, to.y}).floor(),
						 ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()));
}

void dx_adapter::rect_filled_fade(const vec2 &from, const vec2 &to, const color &cl0, const color &cl1,
								  const bool horizontal) const
{
	const auto c0 = ren::color(cl0.red(), cl0.green(), cl0.blue(), cl0.alpha());
	const auto c1 = ren::color(cl1.red(), cl1.green(), cl1.blue(), cl1.alpha());
	buf->add_rect_filled_multicolor(ren::rect({from.x, from.y}, {to.x, to.y}).floor(),
									{c0, horizontal ? c0 : c1, c1, horizontal ? c1 : c0});
}

void dx_adapter::circle(const vec2 &center, int32_t radius, const color &cl, float progress, bool filled) const
{
	const auto segments = static_cast<int32_t>((float)radius * 1.5f);
	buf->g.anti_alias = true;
	if (filled)
		buf->add_circle_filled(ren::vec2(center.x, center.y).floor(), radius,
							   ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()), segments, progress);
	else
		buf->add_circle(ren::vec2(center.x, center.y).floor(), radius,
						ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()), 1.f, segments, progress);
	buf->g.anti_alias = false;
}

void dx_adapter::circle_filled_fade(const vec2 &center, int32_t radius, const color &cl, float progress) const
{
	const auto segments = static_cast<int32_t>((float)radius * 1.5f);
	buf->g.anti_alias = true;
	buf->add_circle_filled_multicolor(
		ren::vec2(center.x, center.y).floor(), radius,
		{ren::color(cl.red(), cl.green(), cl.blue(), static_cast<int32_t>(.8f * cl.alpha())),
		 ren::color(cl.red(), cl.green(), cl.blue(), static_cast<int32_t>(.2f * cl.alpha() * progress))},
		segments);
	buf->g.anti_alias = false;
}

void dx_adapter::polygon(vec2 pos, std::vector<sdk::vertex_t> vertices, const float rotation, const color &cl) const
{
	for (auto &v : vertices)
	{
		v.pos += pos;
		v = rotate_vertex(pos, v, rotation);
	}

	ren::vec2 a{vertices[0].pos.x, vertices[0].pos.y};
	ren::vec2 b{vertices[1].pos.x, vertices[1].pos.y};
	ren::vec2 c{vertices[2].pos.x, vertices[2].pos.y};

	buf->g.anti_alias = true;
	buf->add_triangle_filled(a.floor(), b.floor(), c.floor(), ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()));
	buf->g.anti_alias = false;
}

void dx_adapter::text(const vec2 &pos, const color &cl, const wchar_t *const text, const uint32_t font,
					  const gui_legacy::horizontal_alignment horizontal,
					  const gui_legacy::vertical_alignment vertical) const
{
	std::string utf8_str{};
	for (auto c : std::wstring(text))
		utf8_str += ren::utf8_encode(c);

	ren::text_alignment ah{}, av{};
	switch (horizontal)
	{
	case gui_legacy::horizontal_alignment::left:
		ah = ren::align_left;
		break;
	case gui_legacy::horizontal_alignment::right:
		ah = ren::align_right;
		break;
	case gui_legacy::horizontal_alignment::center:
		ah = ren::align_center;
		break;
	}

	switch (vertical)
	{
	case gui_legacy::vertical_alignment::top:
		av = ren::align_top;
		break;
	case gui_legacy::vertical_alignment::bottom:
		av = ren::align_bottom;
		break;
	case gui_legacy::vertical_alignment::center:
		av = ren::align_center;
		break;
	}

	buf->font = ren::draw.fonts[font];
	buf->add_text(ren::vec2(pos.x, pos.y).floor(), utf8_str, ren::color(cl.red(), cl.green(), cl.blue(), cl.alpha()),
				  ren::text_params::with_vh(av, ah));
}

gui_legacy::vec2 dx_adapter::get_text_size(const wchar_t *const text, const uint32_t font) const
{
	std::string utf8_str{};
	for (auto c : std::wstring(text))
		utf8_str += ren::utf8_encode(c);

	const auto sz = ren::draw.fonts[font]->get_text_size(utf8_str);
	return {sz.x, sz.y};
}

float dx_adapter::get_alpha_modifier() const { return buf->g.alpha; }

void dx_adapter::set_alpha_modifier(const float modifier) const { buf->g.alpha = modifier; }

std::shared_ptr<evo::ren::texture> dx_adapter::load_svg_to_texture(const uint8_t *buffer, size_t size, uint32_t w,
																   uint32_t h) const
{
	std::vector<uint32_t> texture(0xFFFFFF);
	image_data img(texture);

	if (img.load_svg(buffer, size, &w, &h) && w && h)
	{
		const auto tex = std::make_shared<evo::ren::texture>(texture.data(), w, h, w * 4);
		tex->create();
		return tex;
	}

	return nullptr;
}

std::shared_ptr<evo::ren::texture> dx_adapter::load_png_to_texture(const uint8_t *buffer) const
{
	uint32_t w, h;
	std::vector<uint32_t> texture(0xFFFFFF);
	image_data img(texture);

	if (img.load_png(buffer, &w, &h))
	{
		const auto tex = std::make_shared<evo::ren::texture>(texture.data(), w, h, w * 4);
		tex->create();
		return tex;
	}

	return nullptr;
}

std::optional<dx_adapter::icon_texture> dx_adapter::icon_to_texture(const char *name, uint32_t target_height) const
{
	const auto hash = util::fnv1a(name) + target_height;
	auto ico = icon_textures.find(hash);

	// lazy icon initialization.
	if (ico == icon_textures.end())
	{
		// load file from virtual file system.
		size_t size{};
		const auto file = load_text_file(name, &size);
		if (!file)
			return std::nullopt;

		// load in-place to get dimension.
		std::vector<uint32_t> texture(0xFFFFFF);
		image_data img(texture);
		uint32_t w{}, h{};
		if (!img.load_svg(file, size, &w, &h))
			return std::nullopt;

		// rasterize svg to texture.
		const auto width_to_height = static_cast<float>(w) / static_cast<float>(h);
		const auto svg = load_svg_to_texture(
			file, size, target_height ? static_cast<uint32_t>(width_to_height * target_height) : 0, target_height);
		game->mem_alloc->free(file);
		if (!svg)
			return std::nullopt;

		// insert new entry.
		ico = icon_textures.insert_or_assign(hash, icon_texture{svg, width_to_height}).first;
	}

	return ico->second;
}

void dx_adapter::weapon(const vec2 &pos, const float height, const color &clr, const char *name) const
{
	static const auto path = XOR_STR_STORE("materials/panorama/images/icons/equipment/%s.svg");

	// build full path.
	char p[MAX_PATH];
	XOR_STR_STACK(pa, path);
	sprintf(p, pa, name);

	// find icon.
	const auto wpn = icon_to_texture(p, height);

	if (!wpn)
		return;

	const auto width = static_cast<size_t>(wpn->width_to_height * height);
	const auto from = vec2{pos.x - width / 2.f, pos.y};
	const auto to = vec2{pos.x + width / 2.f, pos.y + height};

	buf->g.texture = wpn->texture->obj;
	buf->add_rect_filled(ren::rect({from.x + 1, from.y + 1}, {to.x + 1, to.y + 1}).floor(),
						 ren::color(0, 0, 0, static_cast<uint8_t>(.6f * static_cast<float>(clr.alpha()))));
	buf->add_rect_filled(ren::rect({from.x, from.y}, {to.x, to.y}).floor(),
						 ren::color(clr.red(), clr.green(), clr.blue(), clr.alpha()));
	buf->g.texture = {};
}

void dx_adapter::icon(const vec2 &pos, const float height, const color &clr, const char *name) const
{
	static const auto path = XOR_STR_STORE("materials/panorama/images/%s.svg");

	// build full path.
	char p[MAX_PATH];
	XOR_STR_STACK(pa, path);
	sprintf(p, pa, name);

	// find icon.
	const auto wpn = icon_to_texture(p, height);

	if (!wpn)
		return;

	const auto width = static_cast<size_t>(wpn->width_to_height * height);
	const auto from = vec2{pos.x - width / 2.f, pos.y};
	const auto to = vec2{pos.x + width / 2.f, pos.y + height};

	buf->g.texture = wpn->texture->obj;
	buf->add_rect_filled(ren::rect({from.x + 1, from.y + 1}, {to.x + 1, to.y + 1}).floor(),
						 ren::color(0, 0, 0, static_cast<uint8_t>(.6f * static_cast<float>(clr.alpha()))));
	buf->add_rect_filled(ren::rect({from.x, from.y}, {to.x, to.y}).floor(),
						 ren::color(clr.red(), clr.green(), clr.blue(), clr.alpha()));
	buf->g.texture = {};
}

float dx_adapter::get_weapon_width(const float height, const char *name) const
{
	static const auto path = XOR_STR_STORE("materials/panorama/images/icons/equipment/%s.svg");

	// build full path.
	char p[MAX_PATH];
	XOR_STR_STACK(pa, path);
	sprintf(p, pa, name);

	// find icon.
	const auto wpn = icon_to_texture(p, height);

	if (!wpn)
		return 0.f;

	return static_cast<size_t>(wpn->width_to_height * height);
}

float dx_adapter::get_icon_width(const float height, const char *name) const
{
	static const auto path = XOR_STR_STORE("materials/panorama/images/%s.svg");

	// build full path.
	char p[MAX_PATH];
	XOR_STR_STACK(pa, path);
	sprintf(p, pa, name);

	// find icon.
	const auto wpn = icon_to_texture(p, height);

	if (!wpn)
		return 0.f;

	return static_cast<size_t>(wpn->width_to_height * height);
}

void dx_adapter::swap_buffers()
{
	buf->ignore_flush = true;
	buf->swap(ren::draw.layers[esp_layer]);
	buf->ignore_flush = false;
}

void dx_adapter::clear_buffer() { buf->reset(true); }

void dx_adapter::destroy_objects()
{
	buf->destroy();
	for (const auto &[_, t] : icon_textures)
	{
		if (t.texture)
			t.texture->destroy();
	}
}

void dx_adapter::create_objects()
{
	buf->create();
	for (const auto &[_, t] : icon_textures)
	{
		if (t.texture)
			t.texture->create();
	}
}