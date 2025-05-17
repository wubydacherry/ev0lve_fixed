
#include <base/debug_overlay.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/custom_tracing.h>
#include <detail/shot_tracker.h>
#include <hacks/antiaim.h>
#include <hacks/peek_assistant.h>
#include <hacks/rage.h>
#include <hacks/tickbase.h>
#include <sdk/client_entity_list.h>
#include <sdk/client_state.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_client.h>
#include <sdk/game_rules.h>
#include <sdk/global_vars_base.h>
#include <sdk/math.h>
#include <sdk/mdl_cache.h>
#include <sdk/model_info_client.h>
#include <sdk/weapon_system.h>

using namespace sdk;
using namespace detail;
using namespace detail::aim_helper;

namespace sdk
{
void cs_player_t::modify_eye_position(vec3 &pos)
{
	const auto state = get_anim_state();

	if (!state || !hacks::rag.should_recalculate_eye_position())
		return;

	if (state->in_hit_ground || state->duck_amount != 0.f || get_ground_entity() == -1)
	{
		const auto head = lookup_bone(XOR_STR("head_0"));
		auto bone_pos = matrix_get_origin(local_shot_record.mat[head]);

		const auto bone_z = bone_pos.z + 1.7f;
		if (pos.z > bone_z)
		{
			const auto view_modifier = std::clamp((fabsf(pos.z - bone_z) - 4.f) * .16666667f, 0.f, 1.f);
			const auto view_modifier_sqr = view_modifier * view_modifier;
			pos.z += (bone_z - pos.z) * (3.f * view_modifier_sqr - 2.f * view_modifier_sqr * view_modifier);
		}
	}
}
} // namespace sdk

namespace hacks
{
rage rag{};

bool rage::on_create_move(sdk::user_cmd *cmd, bool send_packet)
{
	had_target = false;

	if (!cfg.rage.enable.get())
		return false;

	pred.repredict(cmd);
	update_scan_map();

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn->is_shootable_weapon() && !wpn->is_knife() &&
		wpn->get_item_definition_index() != item_definition_index::weapon_shield)
		return false;

	const auto info = wpn->get_cs_weapon_data();

	cock_revolver(cmd);

	if (!pred.had_attack && !pred.had_secondary_attack && !cfg.rage.auto_engage.get())
		cancel_autostop();

	if (!pred.can_shoot && (!info->bfullauto || wpn->get_in_reload() || !wpn->get_clip1()) &&
		cfg.rage.auto_engage.get())
		cancel_autostop();

	if (peek_assistant.has_shot)
		cancel_autostop();

	if (should_stop)
		autostop(cmd, true);

	if (tickbase.force_choke && !pred.can_shoot)
		return false;

	if (!(aa.get_shot_cmd() < cmd->command_number - game->client_state->choked_commands ||
		  aa.get_shot_cmd() >= cmd->command_number) ||
		(aa.is_fakeducking() && !send_packet))
		return false;

	auto target_cmd =
		&game->input->commands[(cmd->command_number -
								min(sv_maxusrcmdprocessticks / 2 - 1,
									TIME_TO_TICKS(game->globals->curtime - wpn->get_next_primary_attack()))) %
							   input_max];
	if (!aa.is_fakeducking() || target_cmd->command_number < game->client_state->last_command + 1)
		target_cmd = cmd;

	pred.repredict(target_cmd);
	return full_scan(target_cmd, send_packet) && intermediate_shot.has_value();
}

void rage::on_finalize_cycle(sdk::user_cmd *const cmd, bool &send_packet)
{
	if (!intermediate_shot.has_value())
		return;

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	// recalculate eye position.
	const auto prev_z = intermediate_shot->start.z;
	if (local_shot_record.valid)
	{
		recalculate_eye_position = true;
		intermediate_shot->start = game->local_player->get_eye_position();
		recalculate_eye_position = false;
	}

	// recalculate view angles.
	auto viewangles = calc_angle(intermediate_shot->start, intermediate_shot->end);
	viewangles -= game->local_player->get_punch_angle_scaled();

	// check if the shot still is valid.
	if (!intermediate_shot->manual && fabsf(prev_z - intermediate_shot->start.z) > .01f)
	{
		const auto pen =
			trace.wall_penetration(intermediate_shot->start, intermediate_shot->end, intermediate_shot->record,
								   custom_tracing::bullet_safety::none, intermediate_shot->target_direction);
		if (!pen.did_hit || (pen.damage < get_mindmg(intermediate_shot->record->player, pen.hitbox) &&
							 pen.damage < get_adjusted_health(intermediate_shot->record->player)))
		{
			tickbase.to_shift = 0;
			tickbase.prepare_cycle();
			peek_assistant.has_shot = false;
			drop_shot();
			aa.on_send_packet(cmd, true);
		}
	}

	if (!intermediate_shot.has_value())
	{
		reset();
		return;
	}

	// fixup following commands.
	for (auto i = cmd->command_number; i <= game->client_state->last_command + game->client_state->choked_commands + 1;
		 i++)
	{
		auto &c = game->input->commands[i % input_max];
		auto &verified = game->input->verified_commands[i % input_max];

		auto target = c.viewangles;
		c.viewangles = viewangles;
		normalize(c.viewangles);
		fix_movement(&c, target);
		normalize_move(c.forwardmove, c.sidemove, c.upmove);
		verified.cmd = c;
		verified.crc = c.get_checksum();
	}

	if (!intermediate_shot->manual && intermediate_shot->record->override_bounds_change_time != 0.f)
	{
		auto &entry = GET_PLAYER_ENTRY(intermediate_shot->record->player);
		entry.bones.bounds_change_time = intermediate_shot->record->player->get_bounds_change_time() =
			game->globals->curtime;
		entry.bones.view_height = intermediate_shot->record->player->get_view_height() =
			intermediate_shot->record->origin.z + intermediate_shot->record->obb_maxs.z;
	}

	if (wpn && !wpn->is_knife())
	{
		// store off estimated impacts.
		detail::custom_tracing::wall_pen pen{};
		if (!intermediate_shot->manual)
			pen = trace.wall_penetration(intermediate_shot->start, intermediate_shot->end, intermediate_shot->record,
										 custom_tracing::bullet_safety::none, intermediate_shot->target_direction,
										 nullptr, true);
		else
			pen = trace.wall_penetration(intermediate_shot->start, intermediate_shot->end, nullptr);
		for (auto i = 0; i < pen.impact_count; i++)
			intermediate_shot->impacts.push_back(pen.impacts[i]);

		// draw impacts.
		if (cfg.local_visuals.impacts.get())
			for (const auto &impact : intermediate_shot->impacts)
				game->debug_overlay->box_overlay_2(
					impact, {-1.25f, -1.25f, -1.25f}, {1.25f, 1.25f, 1.25f}, sdk::angle(),
					cfg.local_visuals.client_impacts.get(),
					sdk::color(250, 250, 250,
							   static_cast<uint8_t>(.25f * cfg.local_visuals.client_impacts.get().value.a * 255.f)),
					4.f);
		// update resolver and remap allocation.
		if (!intermediate_shot->manual)
		{
			auto &entry = GET_PLAYER_ENTRY(intermediate_shot->record->player);

			if (!intermediate_shot->record->player->is_fake_player())
			{
				auto &resolver = entry.resolver;
				auto &resolver_state = resolver.get_resolver_state(*intermediate_shot->record);
				resolver_state.switch_to_opposite(*intermediate_shot->record, intermediate_shot->record->res_direction);

				if (intermediate_shot->record->res_direction != resolver_state.direction)
				{
					resolver.map_resolver_state(*intermediate_shot->record, resolver_state);
					for (auto &other : entry.records)
					{
						other.res_direction = resolver.get_resolver_state(other).direction;
						if (other.has_matrix)
							memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
					}
				}
			}

			// remap to different allocation...
			if (entry.shot_records.exhausted())
				entry.shot_records.pop_back();

			if (entry.shot_records.exhausted())
			{
				intermediate_shot->record = nullptr;
				intermediate_shot->manual = true;
			}
			else
			{
				const auto old = intermediate_shot->record;
				intermediate_shot->record = entry.shot_records.push_front();
				memcpy(intermediate_shot->record, old, sizeof(lag_record));
			}
		}

		// finalize shot.
		shot_track.register_shot(std::move(*intermediate_shot));
	}

	reset();
}

void rage::on_manual(sdk::user_cmd *cmd, shot &&s)
{
	intermediate_cmd = *cmd;
	intermediate_shot = std::move(s);
}

bool rage::did_stop() const { return has_stopped; }

bool rage::should_recalculate_eye_position() const { return recalculate_eye_position; }

void rage::cock_revolver(sdk::user_cmd *const cmd)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn || wpn->get_item_definition_index() != item_definition_index::weapon_revolver)
		return;

	cmd->buttons &= ~user_cmd::attack2;

	if (!wpn->get_clip1() || wpn->get_in_reload() || wpn->get_next_primary_attack() <= 0.f ||
		game->local_player->get_next_attack() > game->globals->curtime)
		return;

	if (pred.info[cmd->command_number % input_max].post_pone_time > game->globals->curtime)
		cmd->buttons |= user_cmd::attack;
	else if (game->globals->curtime < wpn->get_next_secondary_attack() - TICKS_TO_TIME(3))
		cmd->buttons |= user_cmd::attack2;
}

void rage::prioritize_player(cs_player_t *const player, const uint32_t amount)
{
	if (is_priority_scan)
		return;

	const auto idx = player->index();

	auto contains = 0;
	for (const auto &el : scan_list_priority)
		if (el == idx)
			contains++;

	for (auto i = 0u; i < min(amount, 6 - contains); i++)
		scan_list_priority.push_back(idx);
}

bool rage::is_player_in_range(cs_player_t *const player)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto shoot = aa.is_fakeducking() ? aa.get_visual_eye_position() : game->local_player->get_eye_position();

	auto &entry = GET_PLAYER_ENTRY(player);
	if (entry.records.empty() || (player->get_abs_origin() - shoot).length() > wpn->get_cs_weapon_data()->range)
		return false;

	float first_time, last_time;
	entry.get_boundary_times(first_time, last_time);

	const auto latest = entry.get_record(first_time);
	if (!latest)
		return false;

	auto first_shoot =
		latest->origin +
		vec3(0, 0,
			 game->rules->data->get_view_vectors()->duck_view.z +
				 (game->rules->data->get_view_vectors()->view - game->rules->data->get_view_vectors()->duck_view).z *
					 (1.f - latest->duck));
	const auto right = vec3(shoot - first_shoot).normalize().cross({0.f, 0.f, 1.f}).normalize();

	if (in_range(shoot, vec3(first_shoot.x, first_shoot.y, latest->origin.z + 25.f)) ||
		in_range(shoot, first_shoot + right * 22.f) || in_range(shoot, first_shoot - right * 22.f))
		return true;

	if (last_time > 0.f)
		return false;

	if (const auto oldest = entry.get_record(last_time); oldest && oldest != latest)
	{
		first_shoot =
			oldest->origin +
			vec3(
				0, 0,
				game->rules->data->get_view_vectors()->duck_view.z +
					(game->rules->data->get_view_vectors()->view - game->rules->data->get_view_vectors()->duck_view).z *
						(1.f - oldest->duck));

		return in_range(shoot, vec3(first_shoot.x, first_shoot.y, oldest->origin.z + 25.f)) ||
			   in_range(shoot, first_shoot + right * 22.f) || in_range(shoot, first_shoot - right * 22.f);
	}

	return false;
}

bool rage::has_target() const { return had_target; }

bool rage::did_shoot() const { return intermediate_shot.has_value(); }

void rage::drop_shot()
{
	intermediate_shot = std::nullopt;
	if (intermediate_cmd.has_value())
	{
		user_cmd cmd{};
		cmd.command_number = 0;
		aa.set_shot_cmd(&cmd);
		game->input->commands[intermediate_cmd->command_number % input_max] = *intermediate_cmd;
	}
	intermediate_cmd = std::nullopt;
	pred.had_attack = false;
	peek_assistant.has_shot = false;
}

void rage::reset()
{
	intermediate_shot = std::nullopt;
	intermediate_cmd = std::nullopt;
	active = false;
}

bool rage::has_priority_targets() const { return !scan_list_priority.empty(); }

bool rage::is_active() const { return active; }

void rage::set_attempt_jump(bool jump) { attempted_to_jump = jump; }

bool rage::is_player_target(cs_player_t *player) const { return scan_map[player->index()]; }

void rage::update_scan_map()
{
	// determine if we want to scan this player.
	auto old_scan_map = scan_map;
	scan_map.fill(false);
	game->client_entity_list->for_each_player(
		[&](cs_player_t *const player)
		{
			auto &entry = GET_PLAYER_ENTRY(player);

			if (!player->is_enemy() || !player->is_valid() || player->get_gun_game_immunity() ||
				player->get_move_type() == movetype_noclip ||
				(!entry.records.empty() && entry.records.front().dormant) || entry.pvs)
			{
				entry.last_target = nullptr;
				return;
			}

			const auto was_scanned = old_scan_map[player->index()];
			entry.run_extrapolation(was_scanned);
			if (!is_player_in_range(player))
			{
				entry.last_target = nullptr;
				return;
			}

			if (!was_scanned)
			{
				while (!entry.records.empty() && entry.records.front().extrapolated)
					entry.records.pop_front();
				entry.bones.restore(player);
				entry.run_extrapolation(true);
			}

			scan_map[player->index()] = true;
		});

	// remove outdated and priority entries in scan list.
	for (auto it = scan_list_default.begin(); it != scan_list_default.end();)
	{
		const auto player = reinterpret_cast<cs_player_t *const>(game->client_entity_list->get_client_entity(*it));

		if (!player || !scan_map[player->index()])
			it = scan_list_default.erase(it);
		else
			it = std::next(it);
	}

	for (auto it = scan_list_priority.begin(); it != scan_list_priority.end();)
	{
		const auto player = reinterpret_cast<cs_player_t *const>(game->client_entity_list->get_client_entity(*it));

		if (!player || !scan_map[player->index()])
			it = scan_list_priority.erase(it);
		else
			it = std::next(it);
	}

	// populate scan list with new entries.
	game->client_entity_list->for_each_player(
		[&](cs_player_t *const player)
		{
			if (!scan_map[player->index()])
				return;

			if (std::find(scan_list_default.begin(), scan_list_default.end(), player->index()) !=
				scan_list_default.end())
				return;

			scan_list_default.push_front(player->index());
		});

	for (auto i = 0; i < min(scan_list_default.size(), 2); i++)
		player_list.on_target_player(
			reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(scan_list_default[i])));
}

bool rage::full_scan(sdk::user_cmd *const cmd, bool send_packet)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	is_engaged = false;
	auto target = scan_targets(cmd, send_packet);

	if (!target.has_value() || !target->pen.did_hit)
		target = scan_dormants(cmd);

	if (!target.has_value() || !target->pen.did_hit)
		return is_engaged;

	// make sure to track him regardless of the last scan state.
	is_priority_scan = false;
	prioritize_player(target->record->player, 3);

#ifndef NDEBUG
	if (game->dbg.draw_target_shot_matrix.get())
		debug->draw_player(target->record->player, target->record->mat, sdk::color(255, 0, 0, 130), 4.f);
#endif

	// store shot.
	auto s = shot();
	s.damage = target->pen.damage;
	s.start = game->local_player->get_eye_position();
	s.end = target->position;
	s.hitgroup = target->hitgroup;
	s.hitbox = target->pen.hitbox;
	s.time = game->globals->curtime;
	s.manual = false;
	s.alt_attack = target->alt_attack;
	s.server_info.health = target->record->player->get_health();
	s.record = target->record;
	s.target = std::make_shared<rage_target>(*target);
	s.cmd_num = cmd->command_number;
	s.cmd_tick = cmd->tick_count;
	s.server_tick = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
	s.target_tick = cmd->tick_count = target->record->tick_count + TIME_TO_TICKS(calculate_lerp());
	s.target_direction = resolver_networked;

	intermediate_cmd = *cmd;
	aa.set_shot_cmd(cmd);
	auto wish = cmd->viewangles;
	cmd->viewangles = calc_angle(s.start, s.end) - game->local_player->get_punch_angle_scaled();
	normalize(cmd->viewangles);
	fix_movement(cmd, wish);
	cmd->buttons |= s.alt_attack ? user_cmd::attack2 : user_cmd::attack;

	if (!target->record->player->is_fake_player())
		s.target_direction = target->record->res_direction;

	intermediate_shot = s;
	return true;
}

std::optional<rage_target> rage::scan_targets(user_cmd *const cmd, bool send_packet)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	auto current_target = 0, current_priority_target = 0;

	if (!scan_list_priority.empty())
	{
		current_priority_target = scan_list_priority.back();
		scan_list_priority.pop_back();
	}
	if (!scan_list_default.empty())
	{
		current_target = scan_list_default.back();
		scan_list_default.pop_back();
	}

	if (!current_target)
	{
		cancel_autostop();
		return std::nullopt;
	}

	const auto player = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(current_target));
	auto hitpoints = scan_target(cmd, player);

	if (current_priority_target && current_priority_target != current_target)
	{
		is_priority_scan = true;
		const auto priority_player =
			reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(current_priority_target));
		const auto priority_hitpoints = scan_target(cmd, priority_player);
		std::copy(priority_hitpoints.begin(), priority_hitpoints.end(), std::back_inserter(hitpoints));
		is_priority_scan = false;
	}

	rage_target *best_match = nullptr;

	// find best target spot of all valid spots.
	for (auto &hitpoint : hitpoints)
	{
		auto target = !best_match ? std::nullopt : std::optional(*best_match);
		auto alternative = std::optional(hitpoint);

		if (cfg.rage.fov.get() < 180.f && get_fov(cmd->viewangles, game->local_player->get_eye_position(),
												  alternative.value().position) > cfg.rage.fov.get())
			continue;

		if (should_prefer_record(target, alternative))
			best_match = &*alternative;
	}

	// stop if no target found.
	if (!best_match)
	{
		cancel_autostop();
		return std::nullopt;
	}

	had_target = is_engaged = true;

	// abort here if we are using the knife.
	if (wpn->is_knife() || wpn->get_item_definition_index() == item_definition_index::weapon_shield)
		return pred.can_shoot ? std::optional(*best_match) : std::nullopt;

	// perform final step optimization.
	optimize_cornerpoint(best_match);

	if (const auto info = wpn->get_cs_weapon_data();
		!pred.can_shoot &&
		(wpn->get_in_reload() || !wpn->get_clip1() ||
		 game->local_player->get_next_attack() > game->globals->curtime + .18f ||
		 (!info->bfullauto &&
		  wpn->get_next_primary_attack() > game->globals->curtime + min(info->cycle_time - TICKS_TO_TIME(1), .18f)) ||
		 ((!info->bfullauto || peek_assistant.has_shot) && tickbase.to_shift > 0)))
		return std::nullopt;

	// calculate optimal hitchance.
	best_match->hc = calculate_hitchance(best_match, true);

	// abort if optimal hitchance is not suitable.
	const auto target_hitchance = aim_helper::get_hitchance(best_match->record->player);
	if (best_match->hc < min(target_hitchance, .85f))
	{
		cancel_autostop();
		return std::nullopt;
	}

	// stop the player, we have targets!
	if (cfg.rage.auto_engage.get())
		autostop(cmd, should_stop = true);

	if (!pred.can_shoot)
		return std::nullopt;

	// delay the shot if we can peek even further.
	if (cfg.rage.hack.delay_shot.get() && (tickbase.fast_fire || !send_packet) &&
		game->local_player->get_velocity().length() >= 2.f &&
		(tickbase.fast_fire ? (tickbase.lag.first - TIME_TO_TICKS(game->globals->curtime) > 6)
							: (game->client_state->choked_commands < max_new_cmds - 1)) &&
		!wpn->is_knife() && !peek_assistant.pos.has_value() && !aa.is_lag_hittable())
		return std::nullopt;

	// calculate hitchance.
	best_match->hc = calculate_hitchance(best_match);

	// abort if hitchance is not suitable.
	if (best_match->hc < target_hitchance && (!has_full_accuracy() || best_match->hc < min(target_hitchance, .85f)))
	{
		// scope the weapon if hitchance is not good enough.
		if (cfg.rage.hack.auto_scope.get() && wpn->is_scoped_weapon() && wpn->get_zoom_level() == 0 &&
			!attempted_to_jump && wpn->get_inaccuracy() > 0.f)
			cmd->buttons |= user_cmd::attack2;

		return std::nullopt;
	}

	return *best_match;
}

std::optional<rage_target> rage::scan_dormants(user_cmd *const cmd)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto data = wpn->get_cs_weapon_data();

	// abort here if we are using the knife.
	if (wpn->is_knife() || !cfg.rage.dormant.get())
		return std::nullopt;

	std::vector<rage_target> targets{};

	game->client_entity_list->for_each_player(
		[&targets, &data](cs_player_t *const player)
		{
			if (!player->is_valid(true) || !player->is_dormant() || !player->is_enemy())
				return;

			auto &player_entry = GET_PLAYER_ENTRY(player);
			if (!player_entry.visuals.has_matrix || player_entry.dormant_miss > 1 ||
				player_entry.visuals.last_update + TIME_TO_TICKS(5.f) <
					TIME_TO_TICKS(game->engine_client->get_last_timestamp()))
				return;

			const auto center =
				player_entry.visuals.pos.target + vec3(0, 0, game->rules->data->get_view_vectors()->duck_view.z + 1.f);
			player_entry.dormant_record = lag_record(player);
			player_entry.dormant_record.has_matrix = true;
			player_entry.dormant_record.dormant = true;
			memcpy(player_entry.dormant_record.mat, player_entry.visuals.matrix, sizeof(lag_record::mat));
			auto start = player_entry.visuals.abs_org;
			move_matrix(player_entry.dormant_record.mat, start, player_entry.visuals.pos.target);
			player_entry.dormant_record.res_direction = resolver_networked;
			memcpy(player_entry.dormant_record.res_mat[resolver_networked], player_entry.dormant_record.mat,
				   sizeof(lag_record::mat));
			rage_target target(center, &player_entry.dormant_record, false, center, cs_player_t::hitbox::body,
							   hitgroup_stomach, 0.f);
			target.pen = trace.wall_penetration(game->local_player->get_eye_position(), target.position,
												&player_entry.dormant_record);

			if (!target.pen.did_hit)
				return;

			const auto expected_damage =
				trace.scale_damage(player, target.pen.damage, data->flarmorratio, hitgroup_stomach);
			if (expected_damage > get_mindmg(player, cs_player_t::hitbox::body) ||
				expected_damage > get_adjusted_health(player))
			{
				target.pen.damage = expected_damage;
				targets.push_back(target);
			}
		});

	// select best target.
	std::optional<rage_target> best_match{};
	for (auto &target : targets)
	{
		if (cfg.rage.only_visible.get() && target.pen.impact_count > 1)
			continue;

		auto alternative = std::optional(target);

		if (should_prefer_record(best_match, alternative))
			best_match.emplace(*alternative);
	}

	if (best_match)
	{
		is_engaged = true;

		if (const auto info = wpn->get_cs_weapon_data();
			!pred.can_shoot &&
			(wpn->get_in_reload() || !wpn->get_clip1() ||
			 game->local_player->get_next_attack() > game->globals->curtime + .18f ||
			 (!info->bfullauto && wpn->get_next_primary_attack() >
									  game->globals->curtime + min(info->cycle_time - TICKS_TO_TIME(1), .18f)) ||
			 ((!info->bfullauto || peek_assistant.has_shot) && tickbase.to_shift > 0)))
			return std::nullopt;

		// stop the player, we have targets!
		if (cfg.rage.auto_engage.get())
			autostop(cmd, should_stop = true);

		if (!pred.can_shoot)
			return std::nullopt;

		// calculate hitchance.
		best_match->hc = has_full_accuracy() ? 100.f : calculate_hitchance(&*best_match);

		// abort if damage or hitchance is not suitable.
		const auto target_hitchance = aim_helper::get_hitchance(best_match->record->player);
		if (best_match->hc < target_hitchance)
		{
			// scope the weapon_t if hitchance is not good enough.
			if (cfg.rage.hack.auto_scope.get() && wpn->is_scoped_weapon() && wpn->get_zoom_level() == 0 &&
				!attempted_to_jump && wpn->get_inaccuracy() > 0.f)
				cmd->buttons |= user_cmd::attack2;

			return std::nullopt;
		}
	}

	return best_match;
}

std::vector<rage_target> rage::scan_target(user_cmd *const cmd, cs_player_t *const player)
{
	std::vector<rage_target> hitpoints{};

	if (!player || !player->is_enemy() || !player->is_valid())
		return hitpoints;

	auto &entry = GET_PLAYER_ENTRY(player);

	if (entry.records.empty())
		return hitpoints;

	// grab targets to scan.
	std::optional<rage_target> target;
	lag_record *latest = nullptr, *oldest = nullptr, *shot = nullptr, *angle_diff = nullptr, *uncrouched = nullptr,
			   *safety = nullptr;

	float first_time, last_time;
	entry.get_boundary_times(first_time, last_time);
	const auto lerp = calculate_lerp();
	auto max_angle_diff = 7.5f, max_head_safety_size = 0.f;
	auto max_head_safety = 0;

	if (last_time > 0.f)
	{
		for (auto tick = TIME_TO_TICKS(first_time); tick >= TIME_TO_TICKS(last_time); tick--)
		{
			const auto record = entry.get_record(TICKS_TO_TIME(tick) - lerp);
			if (!record)
				continue;

			if (!latest)
				latest = record;

			if (!uncrouched && record->duck < .1f)
				uncrouched = record;

			oldest = record;

			if (!shot && record->shot)
				shot = record;

			if (record->head_safety > max_head_safety || record->head_safety_size > max_head_safety_size)
			{
				max_head_safety = record->head_safety;
				max_head_safety_size = record->head_safety_size;
				safety = record;
			}

			const auto diff = fabsf(::angle_diff(record->abs_angle.y, latest->abs_angle.y));
			if (diff > max_angle_diff)
			{
				max_angle_diff = diff;
				angle_diff = record;
			}
		}
	}
	else if (first_time != 0.f)
	{
		latest = entry.get_record(first_time);
		if (latest && last_time == -1.f)
			latest->tick_count = TIME_TO_TICKS(first_time) - 1000;
	}

	// grab last target tick.
	const auto target_time = entry.aimbot.target_time;
	entry.aimbot.target_time = 0.f;

	// function to finalize and store target.
	const auto finalize = [&]()
	{
		entry.hittable = target.has_value();

		if (target.has_value())
		{
			if (target->pen.damage >= get_mindmg(target->record->player, target->pen.hitbox))
				entry.aimbot.target_time =
					target_time + TICKS_TO_TIME(game->client_state->choked_commands == 1 ? 2 : 1);

			prioritize_player(target->record->player, 2);
			hitpoints.push_back(std::move(*target));
		}
	};

	// scan latest.
	if (latest)
		target = scan_record(latest);

	// scan highest angle diff.
	if (angle_diff && angle_diff->recv_time != latest->recv_time &&
		(!shot || angle_diff->recv_time != shot->recv_time) &&
		(angle_diff->recv_time != oldest->recv_time || !angle_diff->is_moving()))
	{
		auto alternative = scan_record(angle_diff);

		if (should_prefer_record(target, alternative))
			target = std::move(alternative);
	}

	// scan highest head safety.
	if (safety && safety->recv_time != latest->recv_time && (!shot || safety->recv_time != shot->recv_time) &&
		safety->recv_time != oldest->recv_time && (!angle_diff || safety->recv_time != angle_diff->recv_time))
	{
		auto alternative = scan_record(safety);

		if (should_prefer_record(target, alternative))
			target = std::move(alternative);
	}

	// is there a shot that should be used instead?
	if (shot && shot->recv_time != latest->recv_time && (!angle_diff || shot->recv_time != angle_diff->recv_time))
	{
		auto alternative = scan_record(shot);

		if (should_prefer_record(target, alternative))
			target = std::move(alternative);
	}
	// are there two distinct records?
	// and if there are, is the last one moving?
	else if (oldest && latest->recv_time != oldest->recv_time && oldest->is_moving())
	{
		auto alternative = scan_record(oldest);

		if (should_prefer_record(target, alternative))
			target = std::move(alternative);
	}
	// is he standing and crouched?
	else if ((!target.has_value() || !target->record->is_moving()) && uncrouched && fabsf(latest->duck) > .1f)
	{
		auto alternative = scan_record(uncrouched);

		if (should_prefer_record(target, alternative))
			target = std::move(alternative);
	}

	finalize();
	return hitpoints;
}

std::optional<rage_target> rage::scan_record(lag_record *record, const bool minimal)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return std::nullopt;

	record->perform_matrix_setup();

	if (record->extrapolated)
	{
		lag_record *newest = nullptr;
		for (auto &rec : GET_PLAYER_ENTRY(record->player).records)
		{
			if (rec.extrapolated)
				continue;

			newest = &rec;
			break;
		}

		if (newest)
		{
			const auto org = newest->origin;
			newest->origin = record->origin;
			newest->perform_matrix_setup();
			newest->has_matrix = false;
			newest->origin = org;
			record->previous = newest;
		}
	}

	if (wpn->is_knife())
		return scan_record_knife(record);

	if (wpn->get_item_definition_index() == item_definition_index::weapon_shield)
		return scan_record_shield(record);

	return scan_record_gun(record, minimal);
}

std::optional<rage_target> rage::scan_record_knife(lag_record *record)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto info = wpn->get_cs_weapon_data();

	if (!record->player->get_studio_hdr())
		return std::nullopt;

	const auto studio_model = record->player->get_studio_hdr()->hdr;
	auto spot = get_hitbox_position(record->player, cs_player_t::hitbox::upper_chest, record->mat);
	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(cs_player_t::hitbox::upper_chest),
												 record->player->get_hitbox_set());

	if (!spot.has_value() || !hitbox)
		return std::nullopt;

	const auto start = game->local_player->get_eye_position();
	const auto calc = calc_angle(start, spot.value());
	vec3 forward;
	angle_vectors(calc, forward);
	trace_simple_filter filter(game->local_player);
	const auto first_swing = wpn->get_next_primary_attack() + .4f < game->globals->curtime;
	const auto swing_or_stab = [&](bool stab) -> rage_target
	{
		custom_tracing::wall_pen pen{};
		const auto range = stab ? 32.f : 48.f;
		const auto end = start + forward * range;
		game_trace tr{};
		ray r{};
		r.init(start, end);
		game->engine_trace->trace_ray(r, mask_solid, &filter, &tr);

		if (tr.fraction >= 1.f)
		{
			r.init(start, end, {-16, -16, -18}, {16, 16, 18});
			game->engine_trace->trace_ray(r, mask_solid, &filter, &tr);
		}

		const auto end_pos = tr.endpos;
		if (tr.fraction >= 1.f || tr.entity != record->player)
			return rage_target{end_pos,
							   record,
							   stab,
							   end_pos,
							   cs_player_t::hitbox::upper_chest,
							   0,
							   0.f,
							   0.f,
							   0.f,
							   custom_tracing::bullet_safety::none,
							   std::move(pen)};

		auto vec_los = record->origin - game->local_player->get_origin();
		vec_los.z = 0.0f;
		vec3 fwd;
		angle_vectors(record->player->get_abs_angle(), fwd);
		fwd.z = 0.0f;
		const auto back_stab = vec_los.normalize().dot(fwd) > .475f;

		auto damage = 0.f;
		if (stab)
			damage = back_stab ? 180.f : 65.f;
		else
			damage = back_stab ? 90.f : (first_swing ? 40.f : 25.f);

		pen.did_hit = true;
		pen.damage = trace.scale_damage(record->player, damage, info->flarmorratio,
										back_stab ? hitgroup_generic : hitgroup_chest);
		return rage_target{end_pos,
						   record,
						   stab,
						   end_pos,
						   cs_player_t::hitbox::upper_chest,
						   0,
						   0.f,
						   0.f,
						   0.f,
						   custom_tracing::bullet_safety::none,
						   std::move(pen)};
	};

	const auto backup_abs_origin = record->player->get_abs_origin();
	const auto backup_obb_mins = record->player->get_collideable()->obb_mins();
	const auto backup_obb_maxs = record->player->get_collideable()->obb_maxs();
	record->player->set_abs_origin(record->origin, true);
	record->player->get_collideable()->obb_mins() = record->obb_mins;
	record->player->get_collideable()->obb_maxs() = record->obb_maxs;
	const auto swing = swing_or_stab(false);
	const auto stab = swing_or_stab(true);
	record->player->set_abs_origin(backup_abs_origin, true);
	record->player->get_collideable()->obb_mins() = backup_obb_mins;
	record->player->get_collideable()->obb_maxs() = backup_obb_maxs;
	record->player->get_eflags() |= efl_dirty_abs_transform;

	if (get_adjusted_health(record->player) <= swing.pen.damage)
		return swing;

	if (get_adjusted_health(record->player) <= stab.pen.damage)
		return stab;

	if (swing.pen.damage > 0.f && !swing.pen.hitgroup)
		return swing;

	return std::nullopt;
}

std::optional<rage_target> rage::scan_record_shield(lag_record *record)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!record->player->get_studio_hdr())
		return std::nullopt;

	const auto studio_model = record->player->get_studio_hdr()->hdr;

	auto spot = get_hitbox_position(record->player, cs_player_t::hitbox::upper_chest, record->mat);
	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(cs_player_t::hitbox::upper_chest),
												 record->player->get_hitbox_set());

	if (!spot.has_value() || !hitbox)
		return std::nullopt;

	const auto start = game->local_player->get_eye_position();
	const auto calc = calc_angle(start, spot.value());
	vec3 forward;
	angle_vectors(calc, forward);
	trace_simple_filter filter(game->local_player);
	const auto end = start + forward * 64.f;

	const auto backup_abs_origin = record->player->get_abs_origin();
	const auto backup_obb_mins = record->player->get_collideable()->obb_mins();
	const auto backup_obb_maxs = record->player->get_collideable()->obb_maxs();
	record->player->set_abs_origin(record->origin);
	record->player->get_collideable()->obb_mins() = record->obb_mins;
	record->player->get_collideable()->obb_maxs() = record->obb_maxs;

	game_trace tr{};
	ray r{};
	r.init(start, end);
	game->engine_trace->trace_ray(r, mask_solid, &filter, &tr);

	if (tr.did_hit() && tr.entity && tr.entity == record->player)
	{
		custom_tracing::wall_pen pen{};
		pen.damage = trace.scale_damage(record->player, 90.f, wpn->get_cs_weapon_data()->flarmorratio, tr.hitgroup);
		pen.did_hit = true;
		record->player->set_abs_origin(backup_abs_origin);
		record->player->get_collideable()->obb_mins() = backup_obb_mins;
		record->player->get_collideable()->obb_maxs() = backup_obb_maxs;
		return rage_target{end,
						   record,
						   false,
						   end,
						   cs_player_t::hitbox::upper_chest,
						   0,
						   0.f,
						   0.f,
						   0.f,
						   custom_tracing::bullet_safety::none,
						   std::move(pen)};
	}

	record->player->set_abs_origin(backup_abs_origin);
	record->player->get_collideable()->obb_mins() = backup_obb_mins;
	record->player->get_collideable()->obb_maxs() = backup_obb_maxs;
	return std::nullopt;
}

std::optional<rage_target> rage::scan_record_gun(lag_record *record, const bool minimal)
{
	if (!record->player)
		return std::nullopt;

	auto &entry = GET_PLAYER_ENTRY(record->player);

	std::optional<rage_target> best_match, best_secure;
	const auto unreliable =
		entry.resolver.unreliable >= 2 && !cfg.rage.hack.secure_point->test(cfg_t::secure_point_disabled);
	const auto should_force =
		unreliable || cfg.rage.hack.secure_point.get().test(cfg_t::secure_point_force) || record->force_safepoint;
	auto targets = perform_hitscan(record, should_force ? custom_tracing::bullet_safety::full
														: custom_tracing::bullet_safety::none);
	auto was_hit = false;
	const auto health = get_adjusted_health(record->player);
	const auto is_lethal_in_body =
		GET_CONVAR_INT("mp_damage_headshot_only")
			? false
			: std::find_if(targets.begin(), targets.end(),
						   [&health](rage_target &target) {
							   return is_body_hitbox(target.hitbox) && target.pen.potential_damage >= health;
						   }) != targets.end();

	lag_record *matching_record = nullptr;
	for (auto &rec : entry.records)
		if (rec.valid && !rec.extrapolated && rec.server_tick == record->server_tick)
		{
			matching_record = &rec;
			break;
		}
	if (matching_record)
	{
		matching_record->head_safety = int32_t(custom_tracing::bullet_safety::none);
		matching_record->head_safety_size = 0.f;
	}

	// perform gun scan.
	for (auto &target : targets)
	{
		if (cfg.rage.only_visible.get() && target.pen.impact_count > 1)
			continue;

		if (target.pen.did_hit)
			was_hit = true;

		if (matching_record && target.pen.hitbox == cs_player_t::hitbox::head &&
			(int32_t(target.safety) > matching_record->head_safety ||
			 target.safety_size > matching_record->head_safety_size))
		{
			matching_record->head_safety = int32_t(target.safety);
			matching_record->head_safety_size = target.safety_size;
		}

		if (target.pen.safety != custom_tracing::bullet_safety::full && should_avoid_hitbox(target.pen.hitbox))
			continue;

		if (target.pen.safety != custom_tracing::bullet_safety::full &&
			cfg.rage.hack.lethal->test(cfg_t::lethal_secure_points) && is_lethal_in_body)
			continue;

		if (!is_body_hitbox(target.hitbox) && cfg.rage.hack.lethal->test(cfg_t::lethal_body_aim) && is_lethal_in_body)
			continue;

		if (is_limbs_hitbox(target.hitbox) && cfg.rage.hack.lethal->test(cfg_t::lethal_limbs) && is_lethal_in_body)
			continue;

		auto alternative = std::optional(target);

		if (should_prefer_record(best_match, alternative))
			best_match.emplace(*alternative);

		if (!minimal && target.pen.safety >= custom_tracing::bullet_safety::no_roll &&
			(!should_force || target.pen.safety == custom_tracing::bullet_safety::full) &&
			should_prefer_record(best_secure, alternative))
			best_secure.emplace(*alternative);
	}

	const auto should_prefer =
		best_secure.has_value() && cfg.rage.hack.secure_point.get().test(cfg_t::secure_point_prefer);

	if (was_hit)
		prioritize_player(record->player);

	if (!minimal)
	{
		// no secure aimpoint available.
		if (should_force && !best_secure.has_value())
			return std::nullopt;

		// swap to secure point.
		if (should_prefer || should_force)
			std::swap(best_match, best_secure);

		if (best_match.has_value() && best_match->record->has_visual_matrix && best_match->pen.did_hit)
		{
			const auto visual_pos =
				get_hitbox_position(record->player, best_match->hitbox, best_match->record->vis_mat);
			if (visual_pos.has_value())
			{
				entry.interpolated_target.update(*visual_pos - best_match->record->abs_origin, .05f);
				if (!entry.last_target)
					entry.interpolated_target.current = entry.interpolated_target.target;
			}
			entry.last_target = std::make_shared<rage_target>(*best_match);
		}
	}

	// make sure the match obeys config values.
	if (!best_match.has_value() || !best_match->pen.did_hit ||
		(best_match->pen.damage < get_mindmg(best_match->record->player, best_match->pen.hitbox) &&
		 best_match->pen.damage < health))
		return std::nullopt;

	// report result.
	return best_match;
}

void rage::autostop(sdk::user_cmd *const cmd, const bool should_stop, const bool force, bool full)
{
	if (!force)
	{
		if (!should_stop)
			has_stopped = false;

		if (should_stop || has_stopped)
		{
			has_stopped = true;
			return;
		}
	}

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	if ((wpn->get_item_definition_index() == item_definition_index::weapon_taser && !cfg.rage.slowwalk.get()) ||
		!game->local_player->is_on_ground() || cmd->buttons & user_cmd::jump || attempted_to_jump ||
		game->local_player->get_move_type() == movetype_ladder || GET_CONVAR_INT("weapon_accuracy_nospread"))
		return;

	// do not use shiftwalk.
	cmd->buttons &= ~user_cmd::speed;

	// determine if we can slide stop.
	const auto should_slide =
		(cfg.rage.slowwalk.get() &&
		 (cfg.rage.slowwalk_mode.get().test(cfg_t::slowwalk_speed) || aa.determine_maximum_lag() < 8 ||
		  game->client_state->choked_commands < aa.determine_maximum_lag() - 3)) ||
		!cfg.rage.slowwalk.get();

	// calculate new speed.
	const auto slide_speed =
		(wpn->get_item_definition_index() == item_definition_index::weapon_revolver ? 180.f : wpn->get_max_speed()) /
			3.f -
		1.f;
	slow_movement(cmd, (should_slide && !full) ? slide_speed : 0.f);
}

void rage::cancel_autostop() { should_stop = false; }
} // namespace hacks
