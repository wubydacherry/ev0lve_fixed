#include <gui/controls/tab.h>
#include <gui/controls/tabs_layout.h>

GUI_NAMESPACE;

void tab::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();
	bg_anim->animate();
	icon_anim->animate();

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];

	d->font = draw.fonts[GUI_HASH("gui_bold")];

	d->add_rect_filled_rounded(r, colors.outlines.outline_light.mod_a(bg_anim->value), 3.f);

	if (icon && icon->obj)
	{
		d->g.texture = icon->obj;
		d->add_rect_filled(r.translate({8.f, 8.f}).size({12.f, 12.f}), icon_anim->value);
		d->g.texture = {};
	}
	
	d->add_text(r.tl() + vec2(icon ? 26.f : 8.f, size.y * .5f + 1.f), text, anim->value, text_params::with_v(align_center));
}

void tab::on_mouse_down(char key)
{
	if (key == m_left)
	{
		const auto p = get_parent()->as<tabs_layout>();
		if (!p || is_selected)
			return;

		ctx->do_tick_sound();
		p->update_selected(shared_from_this());
		is_highlighted = false;

		lock_input();
	}
}

void tab::on_mouse_up(char key)
{
	if (key == m_left)
		unlock_input();
}

void tab::on_mouse_enter()
{
	if (!is_selected)
	{
		anim->direct(colors.texts.hover);
		icon_anim->direct(colors.texts.hover);
	}
}

void tab::on_mouse_leave()
{
	if (!is_selected)
	{
		anim->direct(colors.texts.disabled);
		icon_anim->direct(colors.texts.disabled);
	}
}

void tab::unselect()
{
	is_selected = false;
	anim->direct(colors.texts.disabled);
	icon_anim->direct(colors.texts.disabled);
	bg_anim->direct(0.f);

	if (container)
		container->is_visible = false;
}

void tab::select()
{
	is_selected = true;
	anim->direct(colors.texts.enabled);
	icon_anim->direct(colors.accents.accent);
	bg_anim->direct(1.f);

	if (container)
		container->is_visible = true;
}

void tab::on_first_render_call()
{
	container = ctx->find(container_id);
	if (container)
	{
		container->set_breadcrumb(text);
		container->as<layout>()->nav_tab = shared_from_this();
	}
			size.x += draw.fonts[GUI_HASH("gui_main")]->get_text_size(text).x;
	//size.x += ctx->res.fonts.main->get_text_size(text).x; //ctx->res.fonts.bold->get_text_size(text).x;
	if (icon)
		size.x += 18.f;
}
