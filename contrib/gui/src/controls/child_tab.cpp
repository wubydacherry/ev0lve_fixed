#include <gui/controls/child_tab.h>

GUI_NAMESPACE;

void child_tab::render()
{
	if (!is_visible)
		return;

	anim->animate();
	anim_bg->animate();
	shift_anim->animate();

	auto &d = draw.layers[ctx->content_layer];

	const auto r = area_abs();

	const auto c = anim->value;
	//d->font = /*ctx->res.fonts.bold;*/ctx->res.fonts.main;
	d->font = draw.fonts[GUI_HASH("gui_bold")];
	if (is_horizontal)
	{
		d->add_text(r.center(), text, c, text_params::with_vh(align_center, align_center));
	}
	else
	{
		d->add_rect_filled_multicolor(r.padding_right(1.f), {anim_bg->value, anim_bg->value.mod_a(.0f), anim_bg->value.mod_a(.0f), anim_bg->value});
		d->add_rect_filled_multicolor(
			r.width(2.f),
			{
				colors.accents.accent_dark.mod_a(shift_anim->value),
				colors.accents.accent.mod_a(shift_anim->value),
				colors.accents.accent.mod_a(shift_anim->value),
				colors.accents.accent_dark.mod_a(shift_anim->value),
			});

		auto offset = 10.f;
		if (icon && icon->obj)
		{
			d->g.texture = icon->obj;
			d->add_rect_filled(rect(r.tl() + vec2(offset, 15.f)).size({16.f, 16.f}), c);
			d->g.texture = {};

			offset += 21.f;
		}

		d->add_text(r.tl() + vec2(offset, size.y * 0.5f), text, c, text_params::with_v(align_center));
	}
}

void child_tab::on_first_render_call()
{
	tab::on_first_render_call();
	if (is_selected)
	{
		anim_bg->direct(colors.base.gray);
		shift_anim->direct(1.f);
	}
}

void child_tab::select()
{
	tab::select();
	anim_bg->direct(colors.base.gray);
	shift_anim->direct(1.f);
}

void child_tab::unselect()
{
	tab::unselect();
	anim_bg->direct(colors.base.gray_darker);
	shift_anim->direct(0.f);
}
