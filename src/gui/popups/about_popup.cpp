
#include <base/game.h>
#include <gui/controls.h>
#include <gui/gui.h>
#include <gui/popups/about_popup.h>
#include <gui/popups/public_software_popup.h>
#include <menu/macros.h>

#ifdef CSGO_SECURE
#include <network/helpers.h>
#endif

using namespace evo::gui;
using namespace evo::ren;
using namespace gui::popups;

void about_popup::on_first_render_call()
{
	const auto test_username = [](const std::tuple<about_popup::string_t, about_popup::string_t> &t)
	{
		const auto &[user, _] = t;

		XOR_STR_STACK(user_str, user);
		return user_str == ctx->user.username;
	};

	const auto enc_str = [](const std::string &s)
	{
		std::string nc{s};
		for (auto &c : nc)
			c ^= (char)69;
		return std::pair<std::string, char>(nc, (char)69);
	};

	const auto credits_check = std::find_if(credits.begin(), credits.end(), test_username) != credits.end();
	const auto special_thanks_check =
		std::find_if(special_thanks.begin(), special_thanks.end(), test_username) != special_thanks.end();

	if (!credits_check && !special_thanks_check)
		special_thanks.emplace_back(enc_str(ctx->user.username), XOR_STR_STORE("Supporting ev0lve"));

	size = {260.f, 220.f + (float)credits.size() * 20.f + (float)special_thanks.size() * 20.f};
	pos = draw.display * 0.5f - size * 0.5f;

	popup::on_first_render_call();

	const auto pub_soft = MAKE("ev0lve.about.ps", button, XOR("Public Software"));
	pub_soft->pos = {size.x * 0.5f - pub_soft->size.x - 2.f, size.y - 40.f};
	pub_soft->callback = [&]()
	{
		const auto ps_popup = MAKE("ev0lve.ps", gui::popups::public_software_popup);
		ps_popup->open();
	};

	const auto close_btn = MAKE("ev0lve.about.close", button, XOR("Close"));
	close_btn->pos = {size.x * 0.5f + 2.f, size.y - 40.f};
	close_btn->callback = [&]() { close(); };

	add(pub_soft);
	add(close_btn);
}

void about_popup::render()
{
	if (!is_visible)
		return;

	begin_render();

	const auto area = area_abs();
	const auto header = area_abs().height(24.f);
	const auto &fnt = draw.fonts[GUI_HASH("gui_main")];
	const auto &fnt_bold = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald

	const auto i_rect = rect(area.tl() + vec2{10.f, 30.f}).size({50.f, 50.f}).shrink(4.f);

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(area, 12.f, true, 0.25f);
	d->g.anti_alias = true;

	add_with_blur(d, area, [&area](auto &d) { d->add_rect_filled_rounded(area, color::white(), 4.f); });
	d->add_rect_filled_rounded(area, colors.bg_bottom.mod_a(.8f), 4.f);
	d->add_rect_filled_rounded(header, colors.bg_block, 4.f, rnd_t);
	d->g.anti_alias = {};

	XOR_STR_STACK(version_str, game->version);

	std::string build_str{XOR_STR(" ")};
#if defined(CSGO_SECURE) && !defined(_DEBUG)
	build_str += network::get_build();
#else
	build_str += XOR_STR("dev");
#endif

	d->font = fnt_bold;
	d->add_text(header.center(), XOR_STR("About (v") + std::string(version_str) + build_str + XOR_STR(")"), colors.text,
				text_params::with_vh(align_center, align_center));
	d->add_line(header.bl(), header.br(), colors.accent);

	// head
	d->g.texture = ctx->res.general.logo_head->obj;
	d->add_rect_filled(i_rect, color::white());

	// stripes
	d->g.texture = ctx->res.general.logo_stripes->obj;
	d->add_rect_filled(i_rect, colors.accent);
	d->g.texture = {};

	// ev0lve Digital
	const auto ev0_size = fnt_bold->get_text_size(XOR("ev0"));
	const auto txt_pos = i_rect.tr() + vec2{10.f, 5.f};

	const auto time = std::time(nullptr);

	std::tm tm{};
	localtime_s(&tm, &time);

	d->add_text(txt_pos, XOR("ev0"), colors.accent);
	d->add_text(txt_pos + vec2{ev0_size.x, 0.f}, XOR("lve Digital"), colors.text);
	d->font = fnt;
	d->add_text(txt_pos + vec2{0.f, 16.f}, tfm::format(XOR("© 2015-%i. All rights reserved."), tm.tm_year + 1900),
				colors.text_mid);

	// separator
	d->add_line(i_rect.bl() - vec2{14.f, -10.f}, i_rect.bl() + vec2{size.x - 14.f, 10.f}, colors.outline);

	// credits
	auto offset = i_rect.bl() + vec2{6.f, 20.f};
	d->font = fnt_bold;
	d->add_text(vec2{area.tl().x + size.x * 0.5f, offset.y}, XOR("Credits"), colors.text,
				text_params::with_h(align_center));

	d->font = fnt;
	for (const auto &[user, desc] : credits)
	{
		XOR_STR_STACK(user_str, user);
		XOR_STR_STACK(desc_str, desc);

		offset.y += 20.f;
		d->add_text(offset, user_str, colors.text);
		d->add_text(offset + vec2{size.x - 40.f, 0.f}, desc_str, colors.text_mid, text_params::with_h(align_right));
	}

	d->add_line(offset - vec2{20.f, -30.f}, offset + vec2{size.x - 20.f, 30.f}, colors.outline);
	offset.y += 40.f;

	// special thanks
	d->font = fnt_bold;
	d->add_text(vec2{area.tl().x + size.x * 0.5f, offset.y}, XOR("Special Thanks"), colors.text,
				text_params::with_h(align_center));

	d->font = fnt;
	for (const auto &[user, desc] : special_thanks)
	{
		XOR_STR_STACK(user_str, user);
		XOR_STR_STACK(desc_str, desc);

		offset.y += 20.f;
		d->add_text(offset, user_str, colors.text);
		d->add_text(offset + vec2{size.x - 40.f, 0.f}, desc_str, colors.text_mid, text_params::with_h(align_right));
	}

	d->add_line(offset - vec2{20.f, -30.f}, offset + vec2{size.x - 20.f, 30.f}, colors.outline);
	offset.y += 40.f;

	popup::render();
}