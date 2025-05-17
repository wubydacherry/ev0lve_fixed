#include <gui/gui.h>
#include <gui/preview_model.h>

#include <base/cfg.h>
#include <base/draw_manager.h>
#include <detail/entity_rendering.h>

namespace gui
{
using namespace evo::gui;
using namespace evo::ren;
using namespace detail;

void preview_model::render()
{
	control::render();

	const auto pos = pos_abs();
	const auto rt_size = vec2(235.f, 370.f);
	const auto rt_rect = rect(pos + size * .5f - rt_size * .5f).size(rt_size);
	const auto bb = rt_rect.shrink(30.f).padding_bottom(30.f);

	auto &d = draw.layers[ctx->content_layer];

	const auto draw_icon =
		[&](std::optional<draw_manager::draw_impl::icon_texture> wpn, vec2 pos, float height, color clr)
	{
		const auto width = static_cast<size_t>(wpn->width_to_height * height);
		const auto from = vec2{pos.x - width / 2.f, pos.y};
		const auto to = vec2{pos.x + width / 2.f, pos.y + height};

		d->g.texture = wpn->texture->obj;
		d->add_rect_filled(rect({from.x + 1, from.y + 1}, {to.x + 1, to.y + 1}).floor(),
						   color(0, 0, 0, static_cast<uint8_t>(.6f * static_cast<float>(clr.value.a * 255.f))));
		d->add_rect_filled(rect({from.x, from.y}, {to.x, to.y}).floor(), clr);
		d->g.texture = {};
	};

	const auto col_to_col = [](sdk::color c) { return color(c.red(), c.green(), c.blue(), c.alpha()); };

	// draw fake ass esp. (but not for local player.)
	if (id == GUI_HASH("visuals.local.preview.preview"))
		return;

	setting<cfg_t::bitset> *esp{};
	setting<cfg_t::color> *esp_color{};
	if (id == GUI_HASH("visuals.enemy.preview.preview"))
	{
		esp = &cfg.player_esp.enemy;
		esp_color = &cfg.player_esp.enemy_color;
	}
	else
	{
		esp = &cfg.player_esp.team;
		esp_color = &cfg.player_esp.team_color;
	}

	// begin! (TODO: make this use the actual esp code.)
	auto padding = sdk::vec3(4, 0, 4);

	if (esp->get().test(cfg_t::esp_text))
	{
		d->font = draw.fonts[FNV1A("segoeuib13")];
		d->add_text({bb.center().x, bb.mins.y + 1}, ctx->user.username, color::white(),
					text_params::with_vh(align_bottom, align_center));
	}

	if (esp->get().test(cfg_t::esp_box))
	{
		d->add_rect(bb.shrink(1.f), color::black().mod_a(0.5f * (float)esp_color->get().value.a));
		d->add_rect(bb.expand(1.f), color::black().mod_a(0.5f * (float)esp_color->get().value.a));
		d->add_rect(bb, esp_color->get());
	}

	d->font = draw.fonts[FNV1A("small8")];
	if (esp->get().test(cfg_t::esp_health_bar))
	{
		const auto hc = sdk::interpolate(sdk::color::health_end(), sdk::color::health_start(), 0.75f, 1.f);

		const auto hbb = rect(bb.mins.x - 6.f, bb.mins.y - 1.f, bb.mins.x - 2.f, bb.maxs.y + 1.f);
		const auto length = hbb.maxs.y - hbb.mins.y - 1.f;
		const auto end = 0.75f * length;

		d->add_rect_filled(hbb, color::black().mod_a(0.5f * (float)esp_color->get().value.a));
		d->add_rect_filled(rect(hbb.mins + vec2(1, 1 + length - end), hbb.maxs - vec2(1, 1)),
						   color(hc.red(), hc.green(), hc.blue(), hc.alpha()).mod_a(0.75f));

		if (esp->get().test(cfg_t::esp_health_text))
			d->add_text(hbb.mins + vec2(1, length - end + 1), XOR_STR("90"), color::white(),
						text_params::with_vh(align_center, align_center));

		padding.x *= 2;
	}

	float pad{};
	if (esp->get().test(cfg_t::esp_distance))
	{
		d->add_text({bb.mins.x - padding.x - 1, bb.mins.y}, XOR_STR("15 YD"), color::white(),
					text_params::with_h(align_right));
		pad += 9;
	}

	if (esp->get().test(cfg_t::esp_warn_fastfire))
	{
		const auto tex =
			draw_mgr.adapter.icon_to_texture(XOR_STR("materials/panorama/images/icons/ui/warning.svg"), 12.f);
		if (tex.has_value() && tex->texture)
		{
			draw_icon(tex, {bb.mins.x - padding.x - 7, bb.mins.y + pad}, 12.f, color::white());
			pad += 12;
		}
	}

	if (esp->get().test(cfg_t::esp_software))
	{
		d->add_text({bb.mins.x - padding.x - 1, bb.mins.y + pad}, XOR_STR("E"), color(0, 172, 245),
					text_params::with_h(align_right));
		pad += 9;
	}

	pad = 0.f;

	if (esp->get().test(cfg_t::esp_armor))
	{
		const auto hc = sdk::color::armor();

		const auto hbb = rect(bb.maxs.x + 2.f, bb.mins.y - 1.f, bb.maxs.x + 6.f, bb.maxs.y + 1.f);
		const auto length = hbb.maxs.y - hbb.mins.y - 1.f;
		const auto end = 0.75f * length;

		d->add_rect_filled(hbb, color::black().mod_a(0.5f * (float)esp_color->get().value.a));
		d->add_rect_filled(rect(hbb.mins + vec2(1, 1 + length - end), hbb.maxs - vec2(1, 1)),
						   color(hc.red(), hc.green(), hc.blue(), hc.alpha()).mod_a(0.75f));

		padding.z *= 2;
	}

	if (esp->get().test(cfg_t::esp_flags))
	{
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("DUCK"), col_to_col(sdk::color::attention()));
		pad += 9;
	}

	if (esp->get().test(cfg_t::esp_warn_zeus))
	{
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("ZEUS"), col_to_col(sdk::color::warning()));
		pad += 9;
	}

	if (esp->get().test(cfg_t::esp_flags))
	{
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("C4"), col_to_col(sdk::color::warning()));
		pad += 9;
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("D"), col_to_col(sdk::color::armor()));
		pad += 9;
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("KH"), col_to_col(sdk::color::white()));
		pad += 9;
		d->add_text({bb.maxs.x + padding.z, bb.mins.y + pad}, XOR_STR("HIT"), col_to_col(sdk::color::info()));
		pad += 9;
	}

	if (esp->get().test(cfg_t::esp_ammo))
	{
		const auto hc = sdk::color::ammo();

		const auto hbb = rect(bb.mins.x - 1.f, bb.maxs.y + 2.f, bb.maxs.x + 1.f, bb.maxs.y + 6.f);
		const auto length = hbb.maxs.x - hbb.mins.x - 1.f;
		const auto end = 0.75f * length;

		d->add_rect_filled(hbb, color::black().mod_a(0.5f * (float)esp_color->get().value.a));
		d->add_rect_filled(rect(hbb.mins + vec2(1, 1), hbb.maxs - vec2(end, 1)),
						   color(hc.red(), hc.green(), hc.blue(), hc.alpha()).mod_a(0.75f));

		padding.y += 5;
	}

	if (esp->get().test(cfg_t::esp_weapon))
	{
		if (cfg.player_esp.prefer_icons.get())
		{
			const auto tex =
				draw_mgr.adapter.icon_to_texture(XOR_STR("materials/panorama/images/icons/equipment/ssg08.svg"), 12.f);
			if (tex.has_value())
				draw_icon(tex, {bb.center().x, bb.maxs.y + padding.y + 3}, 12.f, color::white());
		}
		else
			d->add_text({bb.center().x, bb.maxs.y + padding.y + 3}, XOR_STR("SSG-08"), color::white(),
						text_params::with_h(align_center));
	}
}

void preview_model::on_mouse_down(char key)
{
	if (key == m_left && !is_input_locked())
		lock_input();
}

void preview_model::on_mouse_up(char key)
{
	if (key == m_left && is_locked_by_me())
		unlock_input();
}

void preview_model::on_mouse_move(const evo::ren::vec2 &p, const evo::ren::vec2 &d)
{
	if (!is_locked_by_me())
		return;

	ent_ren.yaw += d.x;
}
} // namespace gui
