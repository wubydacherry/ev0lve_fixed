#include <gui/controls/label.h>

GUI_NAMESPACE;

void label::on_draw_param_changed(char w)
{
	control::on_draw_param_changed(w);
	if (w == dp_text)
		size = fnt->get_text_size(text);
}

void label::render()
{
	control::render();
	if (!is_visible)
		return;

	auto &d = draw.layers[ctx->content_layer];

	vec2 offset{};
	if (is_new)
	{
		d->add_circle_filled(pos_abs() + vec2{2.f, size.y / 2.f}, 2.f, colors.accent);
		offset.x += 8.f;
	}

	d->font = fnt;
	d->add_text(pos_abs() + offset, text, col);
}
