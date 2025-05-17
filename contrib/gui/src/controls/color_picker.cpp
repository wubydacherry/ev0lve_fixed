#include <gui/controls/color_picker.h>
#include <gui/gui.h>
#include <gui/popups/color_picker_popup.h>

GUI_NAMESPACE;

void color_picker::render()
{
	control::render();

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);
	d->add_rect_filled(r, *value);
	d->add_rect(r, colors.outline);
}

void color_picker::on_mouse_down(char key)
{
	if (key == m_left)
	{
		const auto picker_popup =
			std::make_shared<color_picker_popup>(control_id{id + 1, ""}, shared_from_this()->as<color_picker>());
		picker_popup->open();
	}
}
