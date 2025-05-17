
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/events.h>
#include <detail/player_list.h>
#include <hacks/antiaim.h>
#include <sdk/client_entity_list.h>
#include <sdk/game_rules.h>
#include <sdk/global_vars_base.h>
#include <sdk/math.h>
#include <sdk/mdl_cache.h>

using namespace sdk;
using namespace hacks;
using namespace detail::aim_helper;

namespace detail
{
custom_prediction pred{};

void custom_prediction::initial(user_cmd *const cmd)
{
	// backup globals.
	backup_curtime = game->globals->curtime;
	backup_frametime = game->globals->frametime;

	// backup original data.
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	original_cmd = *cmd;

	// recreate entry.
	auto &entry = pred.info[cmd->command_number % input_max];
	if (cmd->command_number != entry.sequence)
	{
		entry = {};
		entry.sequence = cmd->command_number;
	}
	tickbase.compute_current_limit();

	// start prediction.
	predicting = true;

	// predict post pone time.
	predict_post_pone_time(cmd);

	// predict the local player to see if he fired.
	predict_can_fire(cmd);

	// predict the local player for real.
	repredict(cmd);

	// store off target prediction.
	backup_predicted = game->prediction->get_predicted_commands();
}

void custom_prediction::repredict(user_cmd *const cmd)
{
	if (game->client_state->delta_tick <= 0 || game->client_state->signon_state != 6 ||
		cmd->command_number - last_sequence > input_max || last_sequence == -1)
		return;

	// no sound when we run the predictor.
	for (auto i = game->client_state->last_command + 1; i <= cmd->command_number; i++)
		game->input->commands[i % input_max].predicted = true;

	// store old flags.
	auto &entry = info[cmd->command_number % input_max];
	entry.prev_flags = game->local_player->get_flags();

	// run computation.
	computing = true;
	const auto check = cmd->get_checksum();
	if (entry.checksum != check || game->input->verified_commands[cmd->command_number % input_max].crc ==
									   *reinterpret_cast<int32_t *>(&game->globals->interval_per_tick))
	{
		game->prediction->get_predicted_commands() = std::clamp(
			cmd->command_number - game->client_state->last_command_ack, 0, game->prediction->get_predicted_commands());
		entry.checksum = check;
	}
	game->prediction->update(game->client_state->delta_tick, true, game->client_state->last_command_ack,
							 cmd->command_number);
	computing = false;

	game->globals->curtime = TICKS_TO_TIME(game->local_player->get_tick_base() - 1);
	game->globals->frametime = game->globals->interval_per_tick;

	// store new flags.
	entry.flags = game->local_player->get_flags();
}

void custom_prediction::restore(user_cmd *const cmd, bool send_packet, bool end)
{
	// stop predicting.
	predicting = false;

	// restore globals.
	game->globals->curtime = backup_curtime;
	game->globals->frametime = backup_frametime;

	// let the game do it's own prediction.
	auto last = game->client_state->last_command_ack + backup_predicted;

	game->input->commands[cmd->command_number % input_max].predicted = false;

	if (send_packet && aa.get_shot_cmd() > game->client_state->last_command &&
		aa.get_shot_cmd() < cmd->command_number && aa.choked_shot_cmd == aa.get_shot_cmd() && !aa.is_manual_shot())
	{
		game->input->commands[aa.get_shot_cmd() % input_max].predicted = false;
		last = aa.get_shot_cmd();
	}
	if (can_shoot && (had_attack || had_secondary_attack))
	{
		if (send_packet)
			last = min(last, cmd->command_number);

		if (had_attack)
			cmd->buttons |= user_cmd::attack;
		if (had_secondary_attack)
			cmd->buttons |= user_cmd::attack2;
		game->input->verified_commands[cmd->command_number % input_max] = {*cmd, cmd->get_checksum()};
	}

	if (end)
		game->prediction->get_predicted_commands() =
			std::clamp(last - game->client_state->last_command_ack - 1, 0, backup_predicted);
}

bool custom_prediction::is_predicting() const { return predicting; }

bool custom_prediction::is_computing() const { return computing; }

void custom_prediction::predict_post_pone_time(user_cmd *const cmd)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (wpn && cmd->command_number > 0 && cmd->command_number - last_sequence <= input_max)
	{
		prediction_info *info = nullptr;
		for (auto i = cmd->command_number - 1; i >= cmd->command_number - max_new_cmds - 1; i--)
		{
			auto &inf = pred.info[i % input_max];
			if (inf.sequence != i || inf.tickbase.invalid_commands > 0)
				continue;
			info = &inf;
			break;
		}

		if (!info)
			return;

		const auto last_cmd = &game->input->commands[info->sequence % input_max];
		repredict(last_cmd);
		const auto last_tick = info->tickbase.base - 1;

		wpn->get_postpone_fire_ready_time() = info->post_pone_time;

		if (wpn->get_item_definition_index() != item_definition_index::weapon_revolver || wpn->get_in_reload() ||
			game->local_player->get_next_attack() > TICKS_TO_TIME(last_tick) ||
			wpn->get_next_primary_attack() > TICKS_TO_TIME(last_tick) ||
			(!(last_cmd->buttons & user_cmd::attack) && !(last_cmd->buttons & user_cmd::attack2)))
		{
			wpn->get_postpone_fire_ready_time() = FLT_MAX;
			prone_delay = -1;
		}
		else if (last_cmd->buttons & user_cmd::attack)
		{
			if (prone_delay == -1)
				prone_delay = last_tick + TIME_TO_TICKS(prone_time);

			if (prone_delay <= last_tick)
				wpn->get_postpone_fire_ready_time() = TICKS_TO_TIME(prone_delay) + post_delay;
		}

		pred.info[cmd->command_number % input_max].post_pone_time = wpn->get_postpone_fire_ready_time();
	}
}

void custom_prediction::predict_can_fire(user_cmd *const cmd)
{
	can_shoot = had_attack = had_secondary_attack = false;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn || wpn->is_grenade() || wpn->get_item_definition_index() == item_definition_index::weapon_healthshot ||
		wpn->get_item_definition_index() == item_definition_index::weapon_c4)
		return;

	const auto last_shot_time = wpn->get_last_shot_time();
	const auto last_secondary_attack = wpn->get_next_secondary_attack();
	const auto clip = wpn->get_clip1();
	const auto attack_flag =
		wpn->get_item_definition_index() == item_definition_index::weapon_shield ? user_cmd::attack2 : user_cmd::attack;
	had_attack = !!(cmd->buttons & user_cmd::attack);
	had_secondary_attack = !!(cmd->buttons & user_cmd::attack2);
	const auto engage = (wpn->is_shootable_weapon() && clip > 0) || wpn->is_knife() ||
						wpn->get_item_definition_index() == item_definition_index::weapon_shield;
	if (cfg.rage.auto_engage.get() && engage)
		cmd->buttons |= attack_flag;
	repredict(cmd);

	if (wpn == reinterpret_cast<weapon_t *>(
				   game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon())))
		can_shoot = (wpn->get_item_definition_index() == item_definition_index::weapon_shield || wpn->is_knife())
						? (wpn->get_next_secondary_attack() != last_secondary_attack)
						: (last_shot_time != wpn->get_last_shot_time() && (!wpn->is_shootable_weapon() || clip > 0));
	if (engage || had_attack)
		cmd->buttons &= ~attack_flag;
}

void custom_prediction::restore_animation(const int32_t sequence, const bool env)
{
	auto &log = pred.info[sequence % input_max];
	const auto local_player = game->local_player;
	if (!local_player || log.animation.sequence != sequence)
		return;

	if (env)
	{
		if (log.sequence == sequence)
		{
			local_player->get_view_height() = local_player->get_abs_origin().z + log.view_delta;
			local_player->get_bounds_change_time() = log.smooth_bounds_change_time;
			local_player->get_collideable()->obb_maxs() = log.obb_maxs;
			local_player->get_ground_entity() = log.ground_entity;
		}
		if (log.animation.sequence == sequence)
			game->prediction->set_local_view_angles(log.animation.eye);
		return;
	}

	const auto bak = *local_player->get_anim_state();
	*local_player->get_anim_state() = log.animation.state;
	bak.copy_meta(local_player->get_anim_state());
	*local_player->get_animation_layers() = log.animation.layers;
	local_player->get_pose_parameter() = log.animation.poses;

	if (log.sequence == sequence)
	{
		local_player->get_view_height() = log.view_height;
		local_player->get_bounds_change_time() = log.bounds_change_time;
		local_player->get_view_offset().z = log.view_offset;
		local_player->get_collideable()->obb_maxs() = log.obb_maxs;
		local_player->get_ground_entity() = log.ground_entity;
	}

	if (log.animation.sequence == sequence)
		game->prediction->set_local_view_angles(log.animation.eye);
}
} // namespace detail

move_data cs_game_movement_t::setup_move(cs_player_t *const player, user_cmd *cmd)
{
	move_data data{};

	data.first_run_of_functions = false;
	data.game_code_moved_player = false;
	data.player_handle = player->get_handle();

	data.velocity = player->get_velocity();
	data.abs_origin = player->get_abs_origin();
	data.client_max_speed = player->get_maxspeed();

	data.angles = data.view_angles = data.abs_view_angles = cmd->viewangles;
	data.impulse_command = cmd->impulse;
	data.buttons = cmd->buttons;

	data.forward_move = cmd->forwardmove;
	data.side_move = cmd->sidemove;
	data.up_move = cmd->upmove;

	data.constraint_center = vec3();
	data.constraint_radius = 0.f;
	data.constraint_speed_factor = 0.f;

	setup_movement_bounds(&data);

	return data;
}

move_delta_t cs_game_movement_t::process_movement(cs_player_t *const player, move_data *const data)
{
	move_delta_t backup;
	backup.store(player);

	is_processing = true;
	process_movement_int(player, data);
	is_processing = false;

	move_delta_t ret;
	ret.store(player);
	backup.restore(player);
	return ret;
}

void cs_game_movement_t::air_move(cs_player_t *const player, move_data *const data)
{
	vec3 forward, right, up;
	angle_vectors(data->view_angles, forward, right, up);

	const auto fmove = data->forward_move;
	const auto smove = data->side_move;

	forward.z = right.z = 0.f;
	forward.normalize();
	right.normalize();

	vec3 wishvel(forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f);

	auto wishdir = wishvel;
	auto wishspeed = vector_normalize(wishdir);

	// clamp to server defined max speed
	if (wishspeed != 0.f && wishspeed > data->max_speed)
		wishspeed = data->max_speed;

	air_accelerate(wishdir, wishspeed, data, player->get_surface_friction());
}

void cs_game_movement_t::air_accelerate(const vec3 &wishdir, float wishspeed, move_data *mv, float friction)
{
	const auto wishspd = min(wishspeed, 30.f);
	const auto currentspeed = mv->velocity.dot(wishdir);
	const auto addspeed = wishspd - currentspeed;

	if (addspeed <= 0.f)
		return;

	auto accelspeed = GET_CONVAR_FLOAT("sv_airaccelerate") * wishspeed * game->globals->interval_per_tick * friction;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	mv->velocity += wishdir * accelspeed;
}

void cs_game_movement_t::check_parameters(move_data *mv, cs_player_t *player)
{
	const auto speed_squared =
		(mv->forward_move * mv->forward_move) + (mv->side_move * mv->side_move) + (mv->up_move * mv->up_move);

	// slow down by the speed factor
	auto speed_factor = 1.f;
	if (player->get_surface_props())
		speed_factor = player->get_surface_props()->game.maxspeedfactor;

	const auto weapon =
		(weapon_t *)game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon());
	mv->max_speed = weapon ? weapon->get_max_speed() : mv->client_max_speed;
	mv->max_speed *= speed_factor;

	// stamina slowing factor
	if (player->get_stamina() > 0)
	{
		auto speed_scale = std::clamp(1.f - player->get_stamina() / 100.f, 0.f, 1.f);
		speed_scale *= speed_scale;
		mv->max_speed *= speed_scale;
	}

	if ((speed_squared != 0.f) && (speed_squared > mv->max_speed * mv->max_speed))
	{
		const auto ratio = mv->max_speed / sqrt(speed_squared);
		mv->forward_move *= ratio;
		mv->side_move *= ratio;
		mv->up_move *= ratio;
	}
}

void cs_game_movement_t::walk_move(move_data *mv, cs_player_t *player)
{
	vec3 forward, right, up;
	angle_vectors(mv->view_angles, forward, right, up);

	const auto fmove = mv->forward_move;
	const auto smove = mv->side_move;

	forward.z = right.z = 0.f;

	forward.normalize();
	right.normalize();

	vec3 wishvel(forward.x * fmove + right.x * smove, forward.y * fmove + right.y * smove, 0.f);

	auto wishdir = wishvel;
	auto wishspeed = wishdir.length();
	wishdir.normalize();

	// clamp to server defined max speed.
	if (wishspeed != 0.f && wishspeed > mv->max_speed)
	{
		wishvel = wishvel * (mv->max_speed / wishspeed);
		wishspeed = mv->max_speed;
	}

	mv->velocity.z = 0;
	accelerate(wishdir, wishspeed, GET_CONVAR_FLOAT("sv_accelerate"), mv, player);
	mv->velocity.z = 0;

	// additional max speed clamp to keep us from going faster than allowed while turning.
	if (mv->velocity.length_sqr() > mv->max_speed * mv->max_speed)
		mv->velocity *= mv->max_speed / mv->velocity.length();

	// if we made it all the way, then copy trace end as new player position.
	mv->out_wish_velocity += wishdir * wishspeed;
}

void cs_game_movement_t::accelerate(vec3 const &wishdir, float wishspeed, float accel, move_data *mv,
									cs_player_t *const player)
{
	auto stored_accel = accel;
	auto currentspeed = mv->velocity.dot(wishdir);
	const float addspeed = wishspeed - currentspeed;

	if (addspeed <= 0)
		return;

	if (currentspeed < 0)
		currentspeed = 0;

	const auto is_ducking =
		mv->buttons & user_cmd::duck || player->get_ducking() || player->get_flags() & cs_player_t::ducking;
	const auto is_walking = (mv->buttons & user_cmd::speed) != 0 && !is_ducking;

	constexpr auto max_speed = 250.f;
	auto acceleration_scale = max(max_speed, wishspeed);
	auto goal_speed = acceleration_scale;
	auto is_slow_sniper_scoped = false;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	if (GET_CONVAR_INT("sv_accelerate_use_weapon_speed") && wpn)
	{
		is_slow_sniper_scoped = wpn->get_zoom_level() > 0 && wpn->get_cs_weapon_data()->zoom_levels > 1 &&
								(wpn->get_max_speed() * .52f) < 110.f;

		goal_speed *= min(1.f, (wpn->get_max_speed() / max_speed));

		if ((!is_ducking && !is_walking) || ((is_walking || is_ducking) && is_slow_sniper_scoped))
			acceleration_scale *= min(1.f, (wpn->get_max_speed() / max_speed));
	}

	if (is_ducking)
	{
		if (!is_slow_sniper_scoped)
			acceleration_scale *= .34f;

		goal_speed *= .34f;
	}

	if (is_walking)
	{
		if (!is_slow_sniper_scoped)
			acceleration_scale *= .52f;

		goal_speed *= .52f;
	}

	if (is_walking && currentspeed > (goal_speed - 5))
		stored_accel *= std::clamp(
			1.f - (max(0.f, currentspeed - (goal_speed - 5)) / max(0.f, goal_speed - (goal_speed - 5))), 0.f, 1.f);

	auto accelspeed =
		stored_accel * game->globals->interval_per_tick * acceleration_scale * player->get_surface_friction();

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	mv->velocity += wishdir * accelspeed;
}

void cs_game_movement_t::friction(move_data *mv, cs_player_t *const player)
{
	const auto speed = mv->velocity.length();

	if (speed < .1f)
		return;

	auto drop = 0.f;

	// apply ground friction
	if (player->get_ground_entity() != -1)
	{
		const auto friction = GET_CONVAR_FLOAT("sv_friction") * player->get_surface_friction();
		const auto control = (speed < GET_CONVAR_FLOAT("sv_stopspeed")) ? GET_CONVAR_FLOAT("sv_stopspeed") : speed;
		drop += control * friction * game->globals->interval_per_tick;
	}

	// scale the velocity
	auto newspeed = speed - drop;
	if (newspeed < 0.f)
		newspeed = 0.f;

	if (newspeed != speed)
	{
		newspeed /= speed;
		mv->velocity *= newspeed;
	}

	mv->out_wish_velocity -= mv->velocity * (1.f - newspeed);
}
