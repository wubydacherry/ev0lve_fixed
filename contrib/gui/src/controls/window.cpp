#include <gui/controls/window.h>
#include <gui/popups/notifications_popup.h>
#include <gui/popups/search_popup.h>

GUI_NAMESPACE;

void window::render()
{
	pfp_anim->animate();
	src_anim->animate();

	const auto r = area_abs();
	const auto r_tab_line = r.margin_top(3.f).height(64.f);

	auto &d = draw.layers[ctx->content_layer];
	d->g.anti_alias = true;

	// background
	d->add_rect_filled_rounded(r.padding_top(3.f), colors.base.gray_darkest, 3.f, rnd_b);

	// top line
	const auto r_line = r.height(3.f);
	const auto r_line_gradient = r_line.width(r_line.width() - 6.f).margin_left(3.f);
	const auto r_line_half_w = r_line_gradient.width() * .5f;
	const auto r_line_gradient_h = r_line_gradient.width(r_line_half_w);

	d->add_glow(r_line_gradient, 8.f, colors.accents.accent.mod_a(.2f), gp_no_bottom);
	d->add_rect_filled_rounded(r_line.height(6.f), colors.accents.accent_dark, 4.f, rnd_t);
	d->add_rect_filled_multicolor(r_line_gradient_h, {colors.accents.accent_dark, colors.accents.accent, colors.accents.accent, colors.accents.accent_dark});
	d->add_rect_filled_multicolor(
		r_line_gradient_h.margin_left(r_line_half_w), {colors.accents.accent, colors.accents.accent_dark, colors.accents.accent_dark, colors.accents.accent});

	// main line
	d->add_rect_filled(r_tab_line, colors.base.gray);

	// logo
	const auto r_logo_center = r_tab_line.width(76.f).center();
	const auto r_logo_size = ctx->res.general.logo_head->get_size() * .5f;
	const auto r_logo = rect(r_logo_center - r_logo_size, r_logo_center + r_logo_size);

	d->g.texture = ctx->res.general.logo_head->obj;
	d->add_rect_filled(r_logo, color::white());
	d->g.texture = ctx->res.general.logo_stripes->obj;
	d->add_rect_filled(r_logo, colors.accents.accent);
	d->g.texture = {};

	// post logo delimiter
	d->add_line(r_tab_line.tl() + vec2{76.f, 15.f}, r_tab_line.tl() + vec2{76.f, r_tab_line.height() - 12.f}, colors.outlines.outline_light);

	// search icon
	const auto search_pos = get_search_pos();
	d->g.texture = ctx->res.icons.search->obj;
	d->add_rect_filled(rect(search_pos).size({30.f, 30.f}), src_anim->value);
	d->g.texture = {};

	// profile picture
	const auto pfp_pos = get_avatar_pos();
	const auto pfp_area = rect(pfp_pos).size({36.f, 36.f});
	if (const auto tex = ctx->user.avatar; tex && tex->obj)
	{
		d->g.texture = tex->obj;
		d->add_rect_filled_rounded(pfp_area, color::white(), 4.f);
		d->g.texture = {};
	}
	else
		d->add_rect_filled_rounded(pfp_area, colors.base.gray_darkest, 4.f);

	// profile picture hover
	//const auto old_alpha = d->g.alpha;
	//d->g.alpha = pfp_anim->value;
	//d->add_rect_filled_rounded(pfp_area, color::black().mod_a(.7f), 4.f);
	//d->g.texture = ctx->res.icons.pfp_hover->obj;
	//d->add_rect_filled(pfp_area, color::white().mod_a(.46f));
	//d->g.texture = {};
	//d->g.alpha = old_alpha;

	// pfp outline
	d->add_rect_rounded(pfp_area, colors.outlines.outline_dark, 4.f);

	// header shadow
	d->add_shadow_line(r_tab_line.margin_top(64.f).height(7.f), shadow_down, .1f);
	d->g.anti_alias = false;

	control_container::render();
}

void window::on_mouse_down(char key)
{
	control_container::on_mouse_down(key);

	const auto avatar_pos = get_avatar_pos();
	if (is_mouse_on_avatar && ctx->active_popups.empty())
	{
		const auto p = std::make_shared<notifications_popup>(control_id{GUI_HASH("notifications"), XOR_STR("notifications")}, avatar_pos + vec2{0.f, 36.f});
		p->open();

		return;
	}

	if (is_mouse_on_search && ctx->active_popups.empty())
	{
		const auto p = std::make_shared<search_popup>(control_id{GUI_HASH("search"), XOR_STR("search")}, get_search_pos() + vec2{0.f, 36.f});
		p->open();

		return;
	}

	if (!allow_move)
		return;

	if (key == m_left)
	{
		is_caring_about_hover = false;
		if (ctx->top_sort != sort)
			sort = ++ctx->top_sort;

		lock_input();
	}
}

void window::on_mouse_up(char key)
{
	control_container::on_mouse_up(key);
	if (!allow_move)
		return;

	if (key == m_left)
	{
		is_caring_about_hover = true;
		unlock_input();
	}
}

void window::on_mouse_move(const vec2 &p, const vec2 &d)
{
	control_container::on_mouse_move(p, d);

	const auto is_on_avatar = rect(get_avatar_pos()).size({36.f, 36.f}).contains(input.cursor());
	if (is_on_avatar != is_mouse_on_avatar)
	{
		pfp_anim->direct(is_on_avatar ? 1.f : 0.f);
		is_mouse_on_avatar = is_on_avatar;
	}

	const auto is_on_search = rect(get_search_pos()).size({30.f, 30.f}).contains(input.cursor());
	if (is_on_search != is_mouse_on_search)
	{
		src_anim->direct(is_on_search ? colors.texts.hover : colors.texts.disabled);
		is_mouse_on_search = is_on_search;
	}

	if (!allow_move || !ctx->active_popups.empty())
		return;

	if (is_locked_by_me())
	{
		const auto temp_pos = pos + d;
		if (is_out_of_clip(temp_pos))
			return;

		pos = temp_pos;
	}
}

ren::vec2 window::get_avatar_pos() { return area_abs().tr() - vec2{50.f, -17.f}; }

ren::vec2 window::get_search_pos() { return area_abs().tr() - vec2{89.f, -20.f}; }

void window::add(const std::shared_ptr<control> &c) { control_container::add(c); }
