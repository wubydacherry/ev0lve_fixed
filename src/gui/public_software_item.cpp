
#include <gui/gui.h>
#include <gui/public_software_item.h>
#include <shellapi.h>

using namespace evo::gui;
using namespace evo::ren;
using namespace gui;

void public_software_item::init() { anim = std::make_shared<anim_color>(colors.text, 0.15f); }

void public_software_item::render()
{
	const auto r = area_abs();
	anim->animate();

	XOR_STR_STACK(name_str, name);
	XOR_STR_STACK(license_str, license);

	auto &d = draw.layers[ctx->content_layer];
	d->font = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald
	d->add_text(r.tl() + vec2{20.f, 5.f}, name_str, anim->value);
	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(r.tl() + vec2{20.f, 20.f}, license_str, colors.text_mid);
}

void public_software_item::on_mouse_enter() { anim->direct(colors.accent); }

void public_software_item::on_mouse_leave() { anim->direct(colors.text); }

void public_software_item::on_mouse_down(char key)
{
	ctx->do_tick_sound();
	if (key == m_left)
		std::thread(
			[&]
			{
				XOR_STR_STACK(url_str, url);
				ShellExecuteA(nullptr, nullptr, url_str, nullptr, nullptr, SW_SHOWNORMAL);
			})
			.detach();
}
