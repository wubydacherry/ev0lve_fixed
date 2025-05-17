
#include <gui/controls.h>
#include <gui/gui.h>
#include <gui/popups/public_software_popup.h>
#include <gui/public_software_item.h>
#include <menu/macros.h>

using namespace evo::gui;
using namespace evo::ren;
using namespace gui::popups;

void public_software_popup::on_first_render_call()
{
	size = {260.f, 80.f + links.size() * 40.f};
	pos = draw.display * 0.5f - size * 0.5f;

	popup::on_first_render_call();

	auto offset = 30.f;
	for (const auto &[name, license, url] : links)
	{
		XOR_STR_STACK(name_str, name);

		const auto ctrl =
			MAKE_RUNTIME("ev0lve.ps." + std::string(name_str), gui::public_software_item, name, license, url);
		ctrl->pos = vec2{0.f, offset};
		add(ctrl);

		offset += 40.f;
	}

	const auto close_btn = MAKE("ev0lve.ps.close", button, XOR("Close"));
	close_btn->pos = {size.x * 0.5f - close_btn->size.x * 0.5f, size.y - 40.f};
	close_btn->callback = [&]() { close(); };

	add(close_btn);
}

void public_software_popup::render()
{
	if (!is_visible)
		return;

	begin_render();

	const auto area = area_abs();
	const auto header = area_abs().height(24.f);

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(area, 12.f);
	d->g.anti_alias = true;

	add_with_blur(d, area, [&area](auto &d) { d->add_rect_filled_rounded(area, color::white(), 4.f); });
	d->add_rect_filled_rounded(area, colors.bg_bottom.mod_a(.8f), 4.f);
	d->add_rect_filled_rounded(header, colors.bg_block, 4.f, rnd_t);
	d->g.anti_alias = {};

	d->font = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald
	d->add_text(header.center(), XOR("Public Software"), colors.text, text_params::with_vh(align_center, align_center));
	d->add_line(header.bl(), header.br(), colors.accent);

	popup::render();
}