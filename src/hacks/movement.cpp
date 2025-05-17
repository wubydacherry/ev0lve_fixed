
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <hacks/movement.h>
#include <hacks/peek_assistant.h>

using namespace sdk;
using namespace detail;
using namespace detail::aim_helper;

namespace hacks
{
movement mov{};

void movement::bhop(user_cmd *const cmd)
{
	const auto move_type = game->local_player->get_move_type();
	const auto in_water = game->local_player->get_water_level() >= 2;
	const auto can_jump =
		!in_water && move_type != movetype_ladder && move_type != movetype_noclip && move_type != movetype_observer;

	if (cmd->buttons & user_cmd::buttons::jump && can_jump)
	{
		if (cfg.misc.bhop.get() && !(game->local_player->get_flags() & user_cmd::on_ground))
			cmd->buttons &= ~user_cmd::buttons::jump;
	}

	autostrafe(cmd, can_jump);
}

void movement::fix_bhop(sdk::user_cmd *cmd)
{
	if (cfg.misc.bhop.get() && !GET_CONVAR_INT("sv_autobunnyhopping") && game->local_player->is_on_ground())
	{
		cmd->buttons &= ~user_cmd::jump;
		pred.repredict(cmd);
	}
}

void movement::autostrafe(user_cmd *const cmd, bool can_jump)
{
	if (!can_jump || (cfg.misc.peek_assistant.get() && peek_assistant.has_shot) || cmd->buttons & user_cmd::speed ||
		cfg.misc.autostrafe.get().test(cfg_t::autostrafe_disabled))
	{
		disabled_this_interval = true;
		return;
	}

	if (game->local_player->get_flags() & user_cmd::flags::on_ground)
		disabled_this_interval = false;

	if (game->local_player->get_flags() & user_cmd::flags::on_ground || disabled_this_interval)
		return;

	auto yaw = std::remainderf(cmd->viewangles.y, yaw_bounds);
	if (cfg.misc.autostrafe->test(cfg_t::autostrafe_move))
	{
		auto offset = 0.f;
		if (cmd->buttons & user_cmd::move_left)
			offset += 90.f;
		if (cmd->buttons & user_cmd::move_right)
			offset -= 90.f;
		if (cmd->buttons & user_cmd::forward)
			offset *= .5f;
		else if (cmd->buttons & user_cmd::back)
			offset = (-offset * .5f) + 180.f;

		yaw += offset;

		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;
	}

	// abort if trying to strafe without movement strafer
	if (cmd->forwardmove != 0.f || cmd->sidemove != 0.f)
		return;

	const auto velocity = game->local_player->get_velocity();
	auto velocity_angle = RAD2DEG(std::atan2f(velocity.y, velocity.x));
	if (velocity_angle < 0.f)
		velocity_angle += yaw_bounds;

	if (velocity_angle < 0.f)
		velocity_angle += yaw_bounds;

	velocity_angle -= floorf(velocity_angle / yaw_bounds + .5f) * yaw_bounds;

	const auto speed = velocity.length2d();
	const auto ideal = speed > 0.f ? std::clamp(RAD2DEG(std::atan2f(15.f, speed)), 0.f, 45.f) : 0.f;

	bool did_avoid{};
	if (cfg.misc.avoid_collisions)
	{
		const auto pos = game->local_player->get_origin();
		vec3 forward;
		angle_vectors({0.f, yaw, 0.f}, forward);
		const auto trace_to = pos + forward * speed * TICKS_TO_TIME(5);

		ray r;
		game_trace tr;
		trace_no_player_filter f;
		r.init(pos, trace_to, game->local_player->get_collideable()->obb_mins(),
			   game->local_player->get_collideable()->obb_maxs());
		game->engine_trace->trace_ray(r, mask_playersolid, &f, &tr);

		constexpr auto epsilon = .0001f;
		constexpr auto one_step = .03125f;

		if (tr.fraction < 1.f && fabsf(tr.plane.normal.z) < epsilon)
		{
			constexpr auto clip_velocity = [](const vec3 in, const vec3 &normal, vec3 &out)
			{
				const auto backoff = in.dot(normal);
				out = in - normal * vec3(backoff, backoff, backoff);

				auto adjust = out.dot(normal);
				if (adjust < 0.f)
				{
					adjust = min(adjust, -one_step);
					out -= normal * adjust;
				}
			};

			vec3 out;
			clip_velocity(pos + forward, tr.plane.normal, out);

			angle ang;
			vector_angles(out, ang);
			const auto wall_hit_angle = fabsf(std::remainderf(ang.y - yaw, yaw_bounds));

			if (fabsf(std::remainderf(wall_hit_angle - 90.f, yaw_bounds)) > 15.f)
			{
				if (wall_hit_angle > 90.f)
					ang.y += 180.f;

				const auto new_start = tr.endpos + forward * 250.f * TICKS_TO_TIME(1);
				r.init(new_start, new_start + vec3(0.f, 0.f, game->local_player->get_step_size() + one_step),
					   game->local_player->get_collideable()->obb_mins(),
					   game->local_player->get_collideable()->obb_maxs());
				game->engine_trace->trace_ray(r, mask_playersolid, &f, &tr);

				if (!tr.startsolid || tr.allsolid)
				{
					did_avoid = true;
					yaw =
						std::remainderf(ang.y, yaw_bounds) + std::remainderf(ang.y - velocity_angle, yaw_bounds) * .5f;
				}
			}
		}
	}

	const auto correct = (100.f - cfg.misc.autostrafe_turn_speed.get()) * .02f * (ideal + ideal);
	cmd->forwardmove = 0.f;
	const auto velocity_delta = std::remainderf(yaw - velocity_angle, yaw_bounds);

	const auto wep = (sdk::weapon_t *)game->client_entity_list->get_client_entity_from_handle(
		game->local_player->get_active_weapon());
	const auto is_throwing_nade = wep && wep->is_grenade() && wep->get_throw_time() != 0.f &&
								  TICKS_TO_TIME(game->local_player->get_tick_base()) >= wep->get_throw_time();

	if (is_throwing_nade && fabsf(velocity_delta) <= 20.f)
	{
		auto target = cmd->viewangles;
		target.y = std::remainderf(yaw, yaw_bounds);

		cmd->forwardmove = forward_bounds;
		cmd->sidemove = 0.f;
		fix_movement(cmd, target);
		return;
	}

	auto rotate_movement = [](user_cmd *cmd, float target_yaw)
	{
		const auto rot = DEG2RAD(cmd->viewangles.y - target_yaw);
		const auto forward = std::cos(rot) * cmd->forwardmove - std::sin(rot) * cmd->sidemove;
		cmd->sidemove = std::sin(rot) * cmd->forwardmove + std::cos(rot) * cmd->sidemove;
		cmd->forwardmove = forward;
	};

	if ((fabsf(velocity_delta) > 170.f || velocity_delta > correct) && speed > 80.f)
	{
		yaw = correct + velocity_angle;
		cmd->sidemove = -side_bounds;
		rotate_movement(cmd, std::remainderf(yaw, yaw_bounds));
		return;
	}

	side_switch = !side_switch;
	if (-correct <= velocity_delta || speed <= 80.f)
	{
		if (side_switch)
		{
			yaw -= ideal;
			cmd->sidemove = -side_bounds;
		}
		else
		{
			yaw += ideal;
			cmd->sidemove = side_bounds;
		}
		rotate_movement(cmd, std::remainderf(yaw, yaw_bounds));
	}
	else
	{
		yaw = velocity_angle - correct;
		cmd->sidemove = side_bounds;
		rotate_movement(cmd, std::remainderf(yaw, yaw_bounds));
	}
}
void movement::edge_jump(sdk::user_cmd *cmd)
{
	if (cfg.misc.edge_jump.get() && game->local_player->is_on_ground())
	{
		auto mv = game->cs_game_movement->setup_move(game->local_player, cmd);
		const auto data = game->cs_game_movement->process_movement(game->local_player, &mv);
		if (!(data.flags & cs_player_t::on_ground))
			cmd->buttons |= user_cmd::jump;
	}
}
} // namespace hacks
