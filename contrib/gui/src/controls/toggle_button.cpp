#include <gui/controls/toggle_button.h>

namespace evo::gui
{
using namespace evo::ren;

void toggle_button::reset() { reset_internal(); }

void toggle_button::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();
	const auto r = area_abs();

	auto &l = draw.layers[ctx->content_layer];

	l->add_shadow_rect(r, 4.f);
	l->add_rect_filled_multicolor(r, {colors.bg_even, colors.bg_even, colors.bg_odd, colors.bg_odd});

	if (show_spinner)
	{
		spinner_anim->animate();

		l->g.rotation = spinner_anim->value;
		l->g.texture = ctx->res.general.loader->obj;
		l->g.anti_alias = true;
		l->add_rect_filled({r.center() - vec2{8.f, 8.f}, r.center() + vec2{8.f, 8.f}}, colors.accent);
		l->g.texture = {};
		l->g.anti_alias = {};
		l->g.rotation = {};
	}
	else
	{
		l->g.texture = tex->obj;
		l->add_rect_filled(icon_size.len_sqr() == 0.f
							   ? r.width(r.height())
							   : rect{r.center() - icon_size * 0.5f, r.center() + icon_size * 0.5f},
						   anim->value);
		l->g.texture = {};
	}

	l->add_rect(r, colors.outline);
}

void toggle_button::on_mouse_enter()
{
	if (value)
		return;

	anim->direct(colors.text_light);
}

void toggle_button::on_mouse_leave()
{
	if (value)
		return;

	anim->direct(colors.text);
}

void toggle_button::on_mouse_down(char key)
{
	if (key != m_left || is_input_locked())
		return;

	ctx->do_tick_sound();

	value = !value;
	if (callback)
		callback(value);

	run_universal_callbacks();
	reset_internal();
}

void toggle_button::reset_internal()
{
	anim->direct(value ? colors.accent : (is_mouse_on_me ? colors.text_light : colors.text));
}
} // namespace evo::gui