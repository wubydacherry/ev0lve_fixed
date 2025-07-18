#include <gui/controls/notification_control.h>
#include <gui/gui.h>

GUI_NAMESPACE;

void evo::gui::notification_control::render()
{
	control::render();
	if (!is_visible)
		return;

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->font = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald
	d->add_text(r.tl() + vec2{5.f, 5.f}, obj->header, colors.text_light);
	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(r.tl() + vec2{5.f, 22.f}, obj->text, colors.text);
}