#include <base/game.h>
#include <gui/gui.h>
#include <gui/widgets/bomb.h>
#include <hacks/esp.h>
#include <hacks/visuals.h>

namespace gui::widgets
{
using namespace evo::gui;
using namespace evo::ren;
using namespace hacks;

void bomb::reset()
{
	widget::reset();

	size = {300.f, 72.f};
	size_anim.direct(size);
}

void bomb::render()
{
	max_alpha = cfg.indicators.alpha.get() / 100.f;

	widget::render();
	if (!is_visible)
		return;

	// hack: force update this if no bomb but still planting
	const auto time = (float)game->globals->tickcount * game->globals->interval_per_tick;
	if (game->engine_client->is_ingame() && vis->bomb_info.plant_site != hacks::visuals::site_none && !vis->bomb)
		vis->on_bomb_planted();

	const auto is_active = vis->bomb || vis->bomb_info.plant_end > time;

	// update visibility
	if ((!is_active && has_content || !game->engine_client->is_ingame()) && !is_forced_visibility)
	{
		has_content = false;
		toggle_visibility(false);
		return;
	}

	if (is_active && !has_content && game->engine_client->is_ingame() || is_forced_visibility)
	{
		has_content = true;
		toggle_visibility(true);
	}

	if (alpha_anim.value == 0.f || (!game->engine_client->is_ingame() && is_forced_visibility))
	{
		title = XOR_STR("Bomb info");
		return;
	}

	bool is_danger{};
	float bar_fill{}, danger_frac{1.f};
	std::string info_text, danger_text, site_letter;

	// determine letter
	if (vis->bomb_info.plant_site > hacks::visuals::site_none && vis->bomb_info.plant_site < hacks::visuals::site_max)
		site_letter = vis->bomb_info.plant_site == hacks::visuals::site_a ? XOR_STR("A") : XOR_STR("B");

	// planting
	if (vis->bomb_info.plant_end >= time && !vis->bomb)
	{
		title = XOR_STR("Planting at ") + site_letter;
		bar_fill = std::clamp((vis->bomb_info.plant_end - time) / sdk::plant_time, 0.f, 1.f);

		const auto planter = vis->bomb_info.planter;
		info_text = tfm::format(XOR_STR("%s is planting at %s (%.1fs remaining)"),
								planter ? es->get_player_name(planter) : XOR_STR("Someone"), site_letter,
								vis->bomb_info.plant_end - time);
	}
	else if (vis->bomb)
	{
		title = XOR_STR("Bombsite ") + site_letter;

		const auto sec_max = vis->bomb->get_timer_length();
		auto sec_remaining = vis->bomb->get_c4_blow() - time;

		if (sec_remaining < 5.f)
			is_danger = true;

		sec_remaining = std::clamp(sec_remaining, 0.f, sec_max);
		bar_fill = std::clamp(sec_remaining / sec_max, 0.f, 1.f);

		const auto defuser =
			(sdk::cs_player_t *)game->client_entity_list->get_client_entity_from_handle(vis->bomb->get_bomb_defuser());
		if (defuser)
		{
			sec_remaining = vis->bomb->get_defuse_count_down() - time;
			info_text =
				tfm::format(XOR_STR("%s is defusing (%.1fs remaining)"), es->get_player_name(defuser), sec_remaining);

			bar_fill = std::clamp(sec_remaining / vis->bomb->get_defuse_length(), 0.f, 1.f);
		}
		else
			info_text = tfm::format(XOR_STR("%.1fs remaining"), sec_remaining);

		const auto dmg = vis->bomb->calculate_damage_to_player(game->local_player);
		if (dmg >= 1.f)
		{
			if (dmg < (float)game->local_player->get_health())
				danger_text = tfm::format(XOR_STR("-%.0f hp"), dmg);
			else
				danger_text = XOR_STR("FATAL");

			danger_frac = 1.f - std::clamp(dmg / (float)game->local_player->get_health(), 0.f, 1.f);
		}
	}

	auto &d = draw.layers[ctx->content_layer];
	const auto old_alpha = d->g.alpha;
	const auto old_clip = d->g.clip_rect;
	d->g.alpha = alpha_anim.value;
	d->override_clip_rect(area_abs());

	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(pos_abs() + vec2{10.f, 34.f}, info_text, color::white());

	if (!danger_text.empty())
	{
		d->font = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald
		d->add_text(pos_abs() + vec2{size.x - 10.f, 34.f}, danger_text,
					color::interpolate(colors.danger, colors.warning, danger_frac), text_params::with_h(align_right));
	}

	const auto bar = area_abs().height(4.f).width(size.x - 20.f).margin_top(58.f).margin_left(10.f);
	d->add_rect_filled(bar, color(255, 255, 255, 48));
	d->add_rect_filled(bar.width(bar.width() * bar_fill), is_danger ? colors.danger : colors.accent);

	d->g.clip_rect = old_clip;
	d->g.alpha = old_alpha;
}
} // namespace gui::widgets
