#include <gui/controls/selectable.h>

GUI_NAMESPACE;

void selectable::on_mouse_enter()
{
	if (is_selected)
		return;

	using namespace ren;
	anim->direct(u_fc(0.f, custom_color.value_or(colors.text_light)));
}

void selectable::on_mouse_leave()
{
	if (is_selected)
		return;

	using namespace ren;
	anim->direct(u_fc(0.f, custom_color.value_or(colors.text_mid)));
}

void selectable::on_mouse_down(char key)
{
	const auto p = get_parent()->as<list>();
	if (!p || (is_selected && !p->allow_multiple) || key != m_left)
		return;

	ctx->do_tick_sound();
	p->update_selected(shared_from_this());
	lock_input();
}

void selectable::on_mouse_up(char key) { unlock_input(); }

void selectable::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();
	const auto r = area_abs();

	auto &l = draw.layers[ctx->content_layer];
	l->add_rect_filled(r.width(anim->value.f), custom_color.value_or(colors.accent));

	l->font = draw.fonts[is_highlighted ? uint64_t(GUI_HASH("gui_bold")) : uint64_t(GUI_HASH("gui_main"))]; //gui_bald gui_main
	l->add_text(r.tl() + vec2(anim->value.f + 4.f, 2.f), text, is_loaded ? colors.accent : anim->value.c);
}

void selectable::select()
{
	is_selected = true;
	anim->direct(u_fc(4.f, custom_color.value_or(colors.accent)));
}

void selectable::unselect()
{
	is_selected = false;
	anim->direct(u_fc(0.f, custom_color.value_or(is_mouse_on_me ? colors.text_light : colors.text_mid)));
}
