#include <gui/selectable_config.h>

using namespace evo::gui;
using namespace evo::ren;

void gui::selectable_config::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();

	const auto r = area_abs();

	auto &l = draw.layers[ctx->content_layer];
	l->add_rect_filled(r.width(anim->value.f), custom_color.value_or(colors.accent));

	const auto is_remote = remote_id > 0;
	if (is_remote)
	{
		l->g.texture = draw.textures[GUI_HASH("icon_cloud")]->obj;
		l->add_rect_filled(rect(r.tl(), r.tl() + vec2{12.f, 12.f}).translate({anim->value.f + 4.f, 5.f}),
						   anim->value.c);
		l->g.texture = {};
	}

	l->font = draw.fonts[is_highlighted ? uint64_t(GUI_HASH("gui_bold")) : uint64_t(GUI_HASH("gui_main"))]; //gui_bald gui_main
	l->add_text(r.tl() + vec2(anim->value.f + (is_remote ? 20.f : 4.f), 2.f), text,
				is_loaded ? colors.accent : anim->value.c);
}
