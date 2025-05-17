#include <gui/controls/slider.h>

GUI_NAMESPACE;

#define MAKE_TEMPLATE_FN_DEF(r, f)                                                                                     \
	template r slider<int>::f;                                                                                         \
	template r slider<float>::f;                                                                                       \
	template r slider<double>::f;

template <typename T> void slider<T>::on_mouse_down(char key)
{
	if (key == m_left)
	{
		lock_input();
		update_value();
	}
}

template <typename T> void slider<T>::on_mouse_move(const vec2 &p, const vec2 &d)
{
	if (is_locked_by_me())
		update_value();
}

template <typename T> void slider<T>::on_key_down(uint16_t key)
{
	if (!show_val)
		return;

	const auto o = *value;
	if (key == VK_LEFT || key == VK_DOWN)
		*value -= step;
	if (key == VK_RIGHT || key == VK_UP)
		*value += step;

	*value = std::clamp(*value, low, high);
	if (o != *value)
	{
		if (callback)
			callback(o, *value);

		run_universal_callbacks();
		ctx->do_tick_sound();
	}
}

template <typename T> void slider<T>::on_mouse_up(char key)
{
	if (key == m_left)
	{
		unlock_input();

		if (!area_abs().contains(input.cursor()))
			hide_value_tip();

		ctx->do_tick_sound();
	}
}

template <typename T> void slider<T>::on_mouse_enter()
{
	if ((!is_input_locked() || is_locked_by_me()))
		show_value_tip();
}

template <typename T> void slider<T>::on_mouse_leave()
{
	if (!is_locked_by_me())
		hide_value_tip();
}

template <typename T> void slider<T>::on_mouse_wheel(float factor)
{
	// don't do anything if control is down.
	if (!input.is_key_down(VK_CONTROL))
		return;

	const auto o = *value;
	*value += step * factor;
	*value = std::clamp(*value, low, high);

	if (o != *value)
	{
		if (callback)
			callback(o, *value);

		run_universal_callbacks();
		ctx->do_tick_sound();
	}
}

template <typename T> void slider<T>::render()
{
	control::render();
	anim->animate();

	const auto r = area_abs().floor();

	auto &d = draw.layers[ctx->content_layer];
	d->g.anti_alias = true;
	d->add_shadow_rect(r, 4.f, false, 0.15f);
	d->g.anti_alias = false;
	d->add_rect_filled(r, colors.bg_bottom);
	d->add_rect_filled(r.size(get_fill()), colors.accent);
	d->add_rect(r, colors.outline);

	if (anim->value > 0.f)
	{
		const auto &m = draw.fonts[GUI_HASH("gui_main")];
		const auto &b = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald

		const auto low_s = get_formatted(low);
		const auto high_s = get_formatted(high);
		const auto value_s = get_formatted(*value);

		const auto low_sz = m->get_text_size(low_s) + vec2(6.f, 0.f);
		const auto high_sz = m->get_text_size(high_s);
		const auto value_sz = b->get_text_size(value_s) + vec2(6.f, 0.f);

		const auto p =
			r.margin_bottom(size.y + 24.f).size(vec2(low_sz.x + high_sz.x + value_sz.x + 16.f, value_sz.y + 8.f));

		auto &f = draw.layers[l_top];
		f->g.alpha = anim->value;
		f->g.anti_alias = true;
		f->add_shadow_rect(r, 4.f, false, 0.15f);
		f->g.anti_alias = false;
		f->add_rect_filled(p, colors.bg_bottom);
		f->add_rect(p, colors.outline);
		f->font = m;
		f->add_text(vec2(p.tl().x + 8.f, p.center().y), low_s, colors.text_mid, text_params::with_v(align_center));
		f->add_text(vec2(p.tl().x + low_sz.x + value_sz.x + 8.f, p.center().y), high_s, colors.text_mid,
					text_params::with_v(align_center));
		f->font = b;
		f->add_text(vec2(p.tl().x + low_sz.x + 8.f, p.center().y), value_s, colors.text,
					text_params::with_v(align_center));
		f->g.alpha = 1.f;
	}
}

template <typename T> void slider<T>::update_value()
{
	const auto step_frac = 1.f / step;
	const auto cursor = input.cursor();

	const auto o = *value;

	*value = (cursor.x - pos_abs().x) / size.x * ((float)high - (float)low) + (float)low;
	*value = std::clamp(floorf(*value * step_frac + 0.5f) / step_frac, (float)low, (float)high);

	if (o != *value)
	{
		if (callback)
			callback(o, *value);

		run_universal_callbacks();
	}
}

template <typename T> void slider<T>::show_value_tip()
{
	using namespace ren;
	anim->direct(1.f);
	show_val = true;
}

template <typename T> void slider<T>::hide_value_tip()
{
	using namespace ren;
	anim->direct(0.f);
	show_val = false;
}

template <typename T> void slider<T>::determine_type() { type = std::is_floating_point<T>() ? st_float : st_int; }

template <typename T> void slider<T>::reset()
{
	if (callback)
		callback(value, value);
}

MAKE_TEMPLATE_FN_DEF(void, on_mouse_down(char key));
MAKE_TEMPLATE_FN_DEF(void, on_mouse_move(const ren::vec2 &p, const ren::vec2 &d));
MAKE_TEMPLATE_FN_DEF(void, on_key_down(uint16_t key));
MAKE_TEMPLATE_FN_DEF(void, on_mouse_up(char key));
MAKE_TEMPLATE_FN_DEF(void, on_mouse_wheel(float factor));
MAKE_TEMPLATE_FN_DEF(void, on_mouse_enter());
MAKE_TEMPLATE_FN_DEF(void, on_mouse_leave());
MAKE_TEMPLATE_FN_DEF(void, render());
MAKE_TEMPLATE_FN_DEF(void, update_value());
MAKE_TEMPLATE_FN_DEF(void, show_value_tip());
MAKE_TEMPLATE_FN_DEF(void, hide_value_tip());
MAKE_TEMPLATE_FN_DEF(void, determine_type());
MAKE_TEMPLATE_FN_DEF(void, reset());

#undef MAKE_TEMPLATE_FN_DEF