#include <gui/controls/button.h>

GUI_NAMESPACE;

void button::on_mouse_enter()
{
	if (is_input_locked())
		return;

	using namespace ren;
	anim->direct(colors.text_light);
}

void button::on_mouse_leave()
{
	if (is_input_locked())
		return;

	using namespace ren;
	anim->direct(colors.text);
}

void button::on_mouse_down(char key)
{
	if (key != m_left || is_input_locked())
		return;

	using namespace ren;
	anim->direct(colors.text_mid);

	if (callback)
		callback();

	run_universal_callbacks();
	ctx->do_tick_sound();
}

void button::on_mouse_up(char key)
{
	if (key != m_left)
		return;

	using namespace ren;
	anim->direct(is_mouse_on_me ? colors.text_light : colors.text);
}

void button::render()
{
	control::render();
	if (!is_visible)
		return;

	const auto r = area_abs();
	anim->animate();

	auto &l = draw.layers[ctx->content_layer];

	if (render_bg || !tex)
	{
		l->add_shadow_rect(r, 4.f);
		l->add_rect_filled_multicolor(r, {colors.bg_even, colors.bg_even, colors.bg_odd, colors.bg_odd});
	}

	const auto col = color.value.a == 0.f ? anim->value : color;

	l->font = draw.fonts[GUI_HASH("gui_main")];
	if (!tex)
		l->add_text(r.center() + vec2{0.f, 1.f}, text, col, text_params::with_vh(align_center, align_center));
	else
	{
		l->g.texture = tex->obj;
		l->add_rect_filled(icon_size.len_sqr() == 0.f
							   ? r.width(r.height())
							   : rect{r.center() - icon_size * 0.5f, r.center() + icon_size * 0.5f},
						   col);
		l->g.texture = {};
		l->add_text(r.tl() + vec2{r.height() + 4.f, r.height() * 0.5f + 1.f}, text, col,
					text_params::with_v(align_center));
	}

	if (render_bg || !tex)
		l->add_rect(r, colors.outline);
}
