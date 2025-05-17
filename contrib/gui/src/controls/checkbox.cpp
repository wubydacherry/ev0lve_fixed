#include <gui/controls/checkbox.h>

GUI_NAMESPACE;

void gui::checkbox::render()
{
	control::render();
	if (!is_visible)
		return;

	const auto area_actual = area_abs().floor();
	anim->animate();

	auto &d = draw.layers[ctx->content_layer];
	d->g.anti_alias = true;
	d->add_shadow_rect(area_abs(), 4.f, false, 0.15f);
	d->g.anti_alias = false;
	d->add_rect_filled(area_actual, colors.bg_bottom);
	d->add_rect(area_actual, colors.outline);

	if (anim->value.value.a > 0.f)
	{
		d->g.anti_alias = true;

		if (!is_radio)
		{
			d->add_line(area_actual.tl() + vec2{2.f, 6.f}, area_actual.tl() + vec2{5.f, 9.f}, anim->value, 2.f);
			d->add_line(area_actual.tl() + vec2{4.f, 9.f}, area_actual.tl() + vec2{10.f, 3.f}, anim->value, 2.f);
		}
		else
			d->add_rect_filled(area_actual.shrink(4.f), anim->value);

		d->g.anti_alias = false;
	}
}

void checkbox::reset()
{
	reset_internal();
	if (callback)
		callback(value);

	run_universal_callbacks();

	hotkey_mode = value.get_hotkey_mode();
	hotkeys = value.get_hotkey_table();

	// clean up bad hotkeys
	clean_up_hotkeys();

	// force refresh global elements
	control::update_hotkey_table();
}

void checkbox::on_mouse_enter()
{
	using namespace ren;
	if (value)
		return;

	anim->direct(colors.outline_light);
}

void checkbox::on_mouse_leave()
{
	using namespace ren;
	if (value)
		return;

	anim->direct(colors.outline_light.mod_a(0));
}

void checkbox::on_mouse_down(char key)
{
	if (key == m_left && !is_input_locked())
	{
		ctx->do_tick_sound();

		value = !value;
		if (callback)
			callback(value);

		run_universal_callbacks();
		reset_internal();
		lock_input();
	}
}

void checkbox::on_mouse_up(char key)
{
	if (key == m_left)
		unlock_input();
}

void checkbox::update_hotkey_table()
{
	control::update_hotkey_table();
	value.update_hotkeys(hotkey_mode, hotkeys);
}

void checkbox::reset_internal()
{
	anim->direct(value ? colors.accent : colors.outline_light.mod_a(is_mouse_on_me ? 255 : 0));
}

void checkbox::update_hotkey_value(hotkey_update upd, uint32_t v)
{
	if (upd == hku_click)
	{
		if (hotkey_mode == hkb_toggle)
			value = !value;
		else
			value = v;
	}
	else
	{
		if (hotkey_mode == hkb_hold)
			value = !v;
	}

	reset_internal();
	if (callback)
		callback(value);

	run_universal_callbacks();
}
