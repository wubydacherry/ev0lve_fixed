#include <gui/controls/search_result.h>
#include <gui/gui.h>

using namespace evo;

namespace evo::gui
{
using namespace ren;

void search_result::on_mouse_down(char key)
{
	if (key != m_left)
		return;

	if (const auto c = ctrl.lock(); c)
		c->highlight();
	if (const auto w = wnd.lock(); w)
		w->as<popup>()->close();
}

void search_result::render()
{
	if (!is_visible)
		return;

	control::render();

	const auto r = area_abs();
	auto &d = draw.layers[ctx->content_layer];

	if (is_mouse_on_me)
		d->add_rect_filled(r.shrink(1.f), colors.text.mod_a(.1f));

	d->font = draw.fonts[GUI_HASH("gui_main")];

	const auto c = ctrl.lock();
	if (!c)
	{
		d->add_text(r.tl() + vec2{4.f, r.center().y}, XOR("Invalid control"), colors.text,
					text_params::with_v(align_center));
		return;
	}

	std::string breadcrumbs;
	for (const auto &b : c->get_breadcrumbs())
		breadcrumbs += XOR(" > ") + b;

	if (!breadcrumbs.empty())
		breadcrumbs = breadcrumbs.substr(3);

	d->add_text(r.tl() + vec2{8.f, 4.f}, text, colors.text);
	d->add_text(r.tl() + vec2{8.f, 20.f}, breadcrumbs, colors.text_mid);
}
} // namespace evo::gui