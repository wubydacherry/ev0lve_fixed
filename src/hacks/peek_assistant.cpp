
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/game.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <hacks/peek_assistant.h>
#include <sdk/debug_overlay.h>
#include <sdk/glow_manager.h>
#include <sdk/math.h>

using namespace sdk;
using namespace detail;
using namespace detail::aim_helper;

namespace hacks
{
peek_assistant_t peek_assistant{};

void peek_assistant_t::on_before_move()
{
	if (!game->local_player || !game->local_player->is_alive())
	{
		pos = std::nullopt;
		has_shot = false;
	}
}

void peek_assistant_t::on_create_move(user_cmd *cmd)
{
	const auto track_glowies = [this](const vec3 visual_pos)
	{
		for (auto n = game->glow_manager->glow_box_definitions.count() - 1; n >= 0; n--)
			if (game->glow_manager->glow_box_definitions[n].birth_time == box_time)
				game->glow_manager->glow_box_definitions.fast_remove(n);

		constexpr auto radius = 36;
		const auto clr = cfg.misc.peek_assistant_color.get();
		auto last_seg = visual_pos;
		for (auto i = 0; i < radius + 1; i++)
		{
			const auto x = visual_pos.x + .33f * radius * cosf(i * 2.f * sdk::pi / radius);
			const auto y = visual_pos.y - .33f * radius * sinf(i * 2.f * sdk::pi / radius);
			const auto seg = vec3{x, y, visual_pos.z};
			vec3 delta_pos(last_seg - seg);
			if (delta_pos.length2d() < .25f * .33f * radius)
			{
				const auto deg = RAD2DEG(atan2f(delta_pos.y, delta_pos.x));
				angle trajectory{0.f, deg - floorf(deg / 360.0f + .5f) * 360.0f, 0.f};
				game->glow_manager->add_glow_box(seg, trajectory, {0.f, -.2f, -.2f}, {delta_pos.length(), .2f, .2f},
												 clr, .5f);
			}
			last_seg = seg;
		}

		box_time = game->globals->curtime - .25f;
	};

	constexpr auto move_mask = user_cmd::forward | user_cmd::back | user_cmd::move_left | user_cmd::move_right;
	buttons_debounce &= cmd->buttons & move_mask;
	const auto npos = game->local_player->get_origin();

	if (cfg.misc.peek_assistant.get())
	{
		if (!pos.has_value() && game->local_player->get_flags() & cs_player_t::on_ground)
		{
			pos = npos;
			track_glowies(npos + vec3{0.f, 0.f, 1.f});
		}
		else
		{
			if (buttons_debounce & user_cmd::forward)
				cmd->forwardmove = min(0.f, cmd->forwardmove);
			if (buttons_debounce & user_cmd::back)
				cmd->forwardmove = max(0.f, cmd->forwardmove);
			if (buttons_debounce & user_cmd::move_left)
				cmd->sidemove = max(0.f, cmd->sidemove);
			if (buttons_debounce & user_cmd::move_right)
				cmd->sidemove = min(0.f, cmd->sidemove);
		}

		// extend live range of glow boxes.
		auto extended = false;
		for (auto n = game->glow_manager->glow_box_definitions.count() - 1; n >= 0; n--)
		{
			auto &box = game->glow_manager->glow_box_definitions[n];
			if (box.birth_time == box_time)
			{
				box.termination_time += game->globals->interval_per_tick;
				if ((box.termination_time - game->globals->curtime) / (box.termination_time - box.birth_time) <= .95f)
				{
					box.birth_time += game->globals->interval_per_tick;
					extended = true;
				}
			}
		}
		if (extended)
			box_time += game->globals->interval_per_tick;
	}

	if (!cfg.misc.peek_assistant.get() || (!state && cfg.misc.peek_assistant.get()))
	{
		pos = std::nullopt;
		buttons_debounce = buttons_current = 0;
		has_shot = false;
	}

	state = cfg.misc.peek_assistant.get();

	if (cmd->buttons & move_mask)
	{
		if (!has_shot && state)
		{
			buttons_current |= cmd->buttons & move_mask;
			return;
		}
	}
	else
		buttons_debounce = buttons_current = 0;

	if (cfg.misc.peek_assistant_mode->test(cfg_t::peek_assistant_free_roam) && !has_shot)
	{
		if (!state)
		{
			pos = std::nullopt;
			buttons_debounce = buttons_current = 0;
		}
		return;
	}

	if (cfg.misc.peek_assistant_mode->test(cfg_t::peek_assistant_retreat_early) && !has_shot)
	{
		has_shot = true;
		shot_pos = npos;
	}

	if (!pos.has_value())
		return;

	cmd->forwardmove = forward_bounds;
	cmd->sidemove = cmd->upmove = 0.f;
	auto view = calc_angle(npos, *pos);
	fix_movement(cmd, view);

	auto data = game->cs_game_movement->setup_move(game->local_player, cmd);
	const auto start = data;

	for (auto i = 0; i < TIME_TO_TICKS(1.f); i++)
	{
		std::tie(data.forward_move, data.side_move) = std::tie(cmd->forwardmove, cmd->sidemove);
		slow_movement(&data, 0.f);
		cs_game_movement_t::walk_move(&data, game->local_player);
		data.abs_origin += data.velocity * game->globals->interval_per_tick;

		if (data.velocity.length() < .1f)
			break;
	}

	if ((data.abs_origin - shot_pos).length() > (shot_pos - *pos).length() - 1.1f)
	{
		data = start;
		slow_movement(&data, 0.f);
		std::tie(cmd->forwardmove, cmd->sidemove) = std::tie(data.forward_move, data.side_move);
		cs_game_movement_t::walk_move(&data, game->local_player);

		if (data.velocity.length() < 1.1f)
		{
			if (!state)
				pos = std::nullopt;
			has_shot = false;
			buttons_debounce = buttons_current;
		}
	}
}

void peek_assistant_t::on_post_move(user_cmd *cmd)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!cfg.misc.nade_assistant.get() || has_shot || !state || !wpn->is_grenade() || !wpn->get_pin_pulled())
	{
		wall_dist = 0.f;
		return;
	}

	const auto ang = game->engine_client->get_view_angles();
	vec3 forward{};
	angle_vectors(ang, forward);
	forward.normalize();
	const auto start = game->local_player->get_eye_position() + game->local_player->get_velocity() *
																	game->globals->interval_per_tick *
																	max(0, TIME_TO_TICKS(.1f) - 2);

	constexpr auto scan_length = 4096.f;
	trace_world_filter filter{};
	game_trace tr{};
	ray r{};
	r.init(start, start + forward * scan_length);
	game->engine_trace->trace_ray(r, mask_solid, &filter, &tr);

	if (wall_dist > 0.f && tr.fraction * scan_length - wall_dist > 100.f)
	{
		cmd->buttons &= ~(user_cmd::attack | user_cmd::attack2);
		pred.repredict(cmd);
	}

	wall_dist = tr.fraction * scan_length;
}

void peek_assistant_t::take_shot()
{
	has_shot = true;
	shot_pos = game->local_player->get_origin();
}
} // namespace hacks
