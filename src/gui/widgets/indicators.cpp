#include <base/game.h>
#include <gui/gui.h>
#include <gui/widgets/indicators.h>
#include <hacks/antiaim.h>

namespace gui::widgets
{
using namespace evo::gui;
using namespace evo::ren;

void indicators::reset()
{
	widget::reset();

	size.y = 130;
	size_anim.direct(size);
}

void indicators::render()
{
	max_alpha = cfg.indicators.alpha.get() / 100.f;

	widget::render();
	if (!is_visible)
		return;

	// update visibility
	if ((has_content && !game->engine_client->is_ingame()) && !is_forced_visibility)
	{
		has_content = false;
		toggle_visibility(false);
		return;
	}

	if (!has_content && game->engine_client->is_ingame() || is_forced_visibility)
	{
		has_content = true;
		toggle_visibility(true);
	}

	if (alpha_anim.value == 0.f)
		return;

	auto &d = draw.layers[ctx->content_layer];

	const auto old_alpha = d->g.alpha;
	const auto old_clip = d->g.clip_rect;
	d->g.alpha = alpha_anim.value;
	d->override_clip_rect(area_abs());

	d->g.anti_alias = true;
	d->font = draw.fonts[GUI_HASH("gui_main")];

	float offset{32.f};
	const auto render_indicator = [&](const std::string &name, float val)
	{
		d->add_text(pos_abs() + vec2{10.f, offset}, name, colors.text);
		d->g.rotation = -90.f;
		d->add_circle(pos_abs() + vec2{size.x - 18.f, offset + 8.f}, 4.f, color(255, 255, 255, 48), 32, 1.f, 2.f);
		d->g.rotation = -90.f;
		d->add_circle(pos_abs() + vec2{size.x - 18.f, offset + 8.f}, 4.f, colors.accent, 32, val, 2.f);
		d->g.rotation = {};
		offset += 24.f;
	};

	const auto tickbase =
		game->engine_client->is_ingame()
			? ((float)hacks::tickbase.compute_current_limit() / ((float)sv_maxusrcmdprocessticks - 2.f))
			: 0.f;
	const auto tickrate_fps = game->engine_client->is_ingame() ? (1.f / game->globals->interval_per_tick) : 60.f;

	render_indicator(XOR_STR("Tickbase"), std::clamp(tickbase, 0.f, 1.f));
	render_indicator(XOR_STR("Desync"), std::clamp(hacks::aa.previous_yaw_modifier, 0.f, 1.f));
	render_indicator(
		XOR_STR("Choke"),
		std::clamp(game->client_state ? (float)game->client_state->choked_commands / 14.f : 0.f, 0.f, 1.f));
	render_indicator(XOR_STR("Frames"), std::clamp(1.f / draw.frame_time, 0.f, tickrate_fps) / tickrate_fps);

	d->g.anti_alias = {};
	d->g.clip_rect = old_clip;
	d->g.alpha = old_alpha;
}
} // namespace gui::widgets
