
#include "player_list.h"
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/custom_tracing.h>
#include <detail/entity_rendering.h>
#include <detail/events.h>
#include <detail/player_list.h>
#include <detail/shot_tracker.h>
#include <hacks/antiaim.h>
#include <hacks/rage.h>
#include <hacks/skinchanger.h>
#include <hacks/tickbase.h>
#include <hooks/hooks.h>
#include <sdk/bitbuf.h>
#include <sdk/client_entity_list.h>
#include <sdk/client_state.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_client.h>
#include <sdk/game_rules.h>
#include <sdk/math.h>
#include <sdk/mdl_cache.h>
#include <sdk/weapon.h>
#include <sdk/weapon_system.h>

using namespace sdk;
using namespace detail::aim_helper;
using namespace hacks;
using namespace hacks::misc;

namespace detail
{
i_player_list player_list = i_player_list();
lag_record local_record = lag_record(), local_shot_record = lag_record(), local_fake_record = lag_record();

void i_player_list::reset()
{
	for (auto &entry : entries)
		entry = player_entry();
}

void i_player_list::send_esp_data(sdk::cs_player_t *player)
{
	// check stuff
	if (!game->local_player || (!cfg.player_esp.shared_esp_enemy && player->is_enemy()))
		return;
	if (!player->is_alive())
		return;

	bool sub{};

	// make sure weapon is not out of bounds
	const auto wep =
		(sdk::weapon_t *)game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon());
	auto wep_id = wep ? (int)wep->get_item_definition_index() : 0;
	if (wep_id > (int)item_definition_index::weapon_bumpmine)
	{
		sub = true;
		wep_id -= 400;
	}

	// clamp just to make sure
	wep_id = std::clamp(wep_id, 0, 255);

	// construct packet
	const auto pos = player->get_origin();
	short px{(short)pos.x}, py{(short)pos.y}, pz{(short)pos.z};

	uint8_t data[shared_esp_size];
	bf_write wr((char *)data, shared_esp_size);
	wr.write_word(sub ? XOR_16(0x7FFC) : XOR_16(0x7FFD));	  // packet id
	wr.write_byte((uint8_t)(player->index() & XOR_32(0xFF))); // entity
	wr.write_byte(wep_id);									  // their weapon id
	wr.write_word(*(uint16_t *)&px);						  // pos
	wr.write_word(*(uint16_t *)&py);
	wr.write_word(*(uint16_t *)&pz);
	wr.write_word(XOR_32(0));
	wr.write_dword(TIME_TO_TICKS(game->engine_client->get_last_timestamp()));

	// let's compose ZE KEY!
	const auto inf = game->engine_client->get_player_info(game->local_player->index());
	auto key = uint32_t((inf.xuid_high ^ game->local_player->index()) % inf.xuid_low);
	key |= (inf.xuid_high ^ inf.xuid_low) & XOR_32(0xFFFF);

	for (int i{}; i < XOR_32(shared_esp_size); ++i)
	{
		data[i] ^= ((uint8_t *)&key)[i % XOR_32(4)];
		if (i % XOR_32(4) == XOR_32(3))
		{
			// roll
			key <<= XOR_32(8);
			key |= i & XOR_32(0xFF);
		}
	}

	// create voicedata
	clc_msg_voicedata_t msg{};
	memset(&msg, 0, sizeof(msg));

	// init voicedata
	((int(__fastcall *)(void *, void *))game->engine.at(functions::clc_msg_voicedata))(&msg, nullptr);

	// apply packet
	msg.set(data);

	// do funky stuff
	struct
	{
		char data[16]{};
		uint32_t len{}, max_len{15};
	} vd{};

	msg.data = &vd;
	msg.format = 0;
	msg.flags = 63;

	hook_manager.send_netmsg->call(game->client_state->net_channel, 0, (net_message *)&msg, false, true);
}

void i_player_list::recv_esp_data(uint8_t *data, int from)
{
	if (!game->local_player)
		return;

	// first of all lets decrypt this shit
	const auto inf = game->engine_client->get_player_info(from);
	auto key = uint32_t((inf.xuid_high ^ from) % inf.xuid_low);
	key |= (inf.xuid_high ^ inf.xuid_low) & XOR_32(0xFFFF);

	for (int i{}; i < XOR_32(shared_esp_size); ++i)
	{
		data[i] ^= ((uint8_t *)&key)[i % XOR_32(4)];
		if (i % XOR_32(4) == XOR_32(3))
		{
			// roll
			key <<= XOR_32(8);
			key |= i & XOR_32(0xFF);
		}
	}

	// now read
	bf_read rd((char *)data, shared_esp_size);

	const auto id = rd.read_word();
	const auto ent_id = (uint32_t)rd.read_byte();
	rd.read_byte(); // wep id: skip this one - not used. only to stay comfy with fatality
	uint16_t px{rd.read_word()}, py{rd.read_word()}, pz{rd.read_word()};
	const vec3 ent_pos{float(*(short *)&px), float(*(short *)&py), float(*(short *)&pz)};
	// ignore pad
	// ignore last written (ticks) as well, we think different (c)

	// check packet validity
	const std::vector<uint16_t> ids{(uint16_t)XOR_16(0x7FFA), (uint16_t)XOR_16(0x7FFB), (uint16_t)XOR_16(0x7FFC),
									(uint16_t)XOR_16(0x7FFD)};
	if (std::find(ids.begin(), ids.end(), id) == ids.end())
		return;

	// check who's it from :)
	const auto sender = (sdk::cs_player_t *)game->client_entity_list->get_client_entity(from);
	if (sender)
	{
		auto &sender_entry = GET_PLAYER_ENTRY(sender);
		if (id == XOR_16(0x7FFC) || id == XOR_16(0x7FFD))
			sender_entry.hack = hack_kind_ev0lve;
		else
			sender_entry.hack = hack_kind_fatality;
	}

	// check player validity
	const auto player = (sdk::cs_player_t *)game->client_entity_list->get_client_entity(ent_id);
	if (!player || !player->is_dormant() || !player->is_enemy())
		return;

	// get entry and update shit
	auto &player_entry = GET_PLAYER_ENTRY(player);
	if (player_entry.visuals.last_update >= TIME_TO_TICKS(game->engine_client->get_last_timestamp()))
		return;

	if ((player_entry.visuals.pos.current - ent_pos).length() < 512.f)
		player_entry.visuals.pos.update(ent_pos, .05f);
	else
		player_entry.visuals.pos.target = player_entry.visuals.pos.current = ent_pos;

	player_entry.visuals.last_update = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
	player_entry.visuals.alpha = .3f;
}

void i_player_list::on_update_end(cs_player_t *const player)
{
	auto &player_entry = GET_PLAYER_ENTRY(player);

	if (cfg.player_esp.shared_esp &&
		player_entry.last_recv_data != TIME_TO_TICKS(game->engine_client->get_last_timestamp()))
	{
		send_esp_data(player);
		player_entry.last_recv_data = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
	}

	if (!player->is_valid() || player->index() == game->engine_client->get_local_player())
	{
		const auto local = player->index() == game->engine_client->get_local_player();

		if (local)
		{
			player_entry.records.clear();
			player_entry.compensation_time = 0.f;

			const auto &info = pred.info[game->client_state->command_ack % input_max];
			const auto wpn = reinterpret_cast<weapon_t *>(
				game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));

			if (wpn && info.sequence == game->client_state->command_ack && info.wpn == wpn->get_handle() &&
				fabsf(wpn->get_last_shot_time() - info.last_shot) > .001f)
			{
				const auto wpn_data = wpn->get_cs_weapon_data();
				wpn->get_next_primary_attack() = wpn->get_last_shot_time() + wpn_data->cycle_time;
				wpn->get_next_secondary_attack() = wpn->get_last_shot_time() + wpn_data->cycle_time_alt;
			}
		}
		else if (!player->is_alive())
			player_entry.records.clear();

		return;
	}

	// prepare records.
	if (!player->is_enemy())
	{
		player_entry.records.reserve(2);
		player_entry.records.clear_all_but_first();
		player_entry.scan_records.clear();
		player_entry.shot_records.clear();
	}
	else
	{
		player_entry.records.reserve(TIME_TO_TICKS(1.f));
		player_entry.scan_records.reserve(TIME_TO_TICKS(.5f));
		player_entry.shot_records.reserve(TIME_TO_TICKS(.2f));
	}

	// clear out old records.
	for (auto i = 0; i < player_entry.records.size(); i++)
	{
		auto &r = player_entry.records[i];
		if (r.sim_time < game->engine_client->get_last_timestamp() - 1.f)
		{
			const auto left = player_entry.records.size() - i;
			player_entry.records.current += left;
			player_entry.records.count -= left;
			break;
		}
	}

	if (player_entry.records.exhausted())
		player_entry.records.pop_back();

	// drop old extrapolation.
	while (!player_entry.records.empty() && player_entry.records.front().extrapolated)
		player_entry.records.pop_front();
	player_entry.bones.restore(player);

	// failsafe against respawn.
	if (!player->get_tick_base())
		player_entry.pvs = false;

	// move pvs indicator forwards.
	if (player_entry.pvs)
	{
		// update player with stale data.
		const auto state = player->get_anim_state();
		player->get_simulation_time() = state->last_update = TICKS_TO_TIME(player->get_tick_base() - 1);
		state->velocity = player->get_velocity();

		state->velocity.z = 0.f;
		state->feet_cycle = player_entry.server_layers[6].cycle;
		state->feet_weight = player_entry.server_layers[6].weight;
		state->strafe_sequence = player_entry.server_layers[7].sequence;
		state->strafe_change_cycle = player_entry.server_layers[7].cycle;
		state->strafe_change_weight = player_entry.server_layers[7].weight;
		state->acceleration_weight = player_entry.server_layers[12].weight;
		state->on_ground = !!(player->get_flags() & cs_player_t::on_ground);
		state->on_ladder = player->get_move_type() == movetype_ladder;
		if (!state->on_ground)
			state->air_time = player_entry.server_layers[4].cycle / player_entry.server_layers[4].playback_rate;

		player_entry.pvs = false;
		if (player_entry.records.exhausted())
			player_entry.records.pop_back();

		auto dormant = player_entry.records.push_front();
		*dormant = lag_record(player);
		dormant->layers = player_entry.server_layers;
		dormant->valid = dormant->dormant = false;

		player->get_eflags() |= efl_dirty_abs_transform | efl_dirty_abs_velocity;
		return;
	}

	// determine if simulation has occured.
	auto last_record = !player_entry.records.empty() ? &player_entry.records.front() : nullptr;
	auto matching = last_record != nullptr;
	if (last_record)
		for (auto i = 0; i < last_record->layers.size(); i++)
			if (last_record->layers[i] != player_entry.server_layers[i])
			{
				matching = false;
				break;
			}
	if (matching)
		return;

	if (last_record)
		player_entry.previous_angle = last_record->eye_ang;

	// detect simulation amount.
	lag_record record(player);
	record.layers = player_entry.server_layers;
	record.determine_simulation_ticks(last_record);

	// continue the interpolation if the update has been postponed.
	if (player->get_simulation_time() != record.sim_time)
	{
		player->on_simulation_time_changing(player->get_simulation_time(), record.sim_time);
		player->get_simulation_time() = record.sim_time;
	}

	// have we already seen this update?
	if (record.lag <= 0 || record.lag > TIME_TO_TICKS(1.f))
		return;

	// count jittering pitch for dumb reasons.
	if (player_entry.pitch_prev == XOR_32(0x42b1fa80) &&
		*reinterpret_cast<uint32_t *>(&player->get_eye_angles().x) == XOR_32(0x42b1fd50))
		player_entry.pitch_alt = max(player_entry.pitch_alt + 1, 4);
	else if (player_entry.pitch_prev != XOR_32(0x42b1fd50) ||
			 *reinterpret_cast<uint32_t *>(&player->get_eye_angles().x) != XOR_32(0x42b1fa80))
		player_entry.pitch_alt = 0;
	player_entry.pitch_prev = *reinterpret_cast<uint32_t *>(&player->get_eye_angles().x);
	if (player_entry.pitch_alt >= 4)
		record.do_not_set = true;

	// place the record in the list.
	game->mdl_cache->begin_lock();
	auto &rec = *player_entry.records.push_front();
	rec = std::move(record);

	// animate the record.
	if (true)
	{
		if (last_record && player->get_stamina() == last_record->stamina && last_record->stamina > 0.f)
			player->get_stamina() = rec.stamina = 0.f;

		const auto prev_change_time = player->get_bounds_change_time();
		perform_animations(&rec, last_record);
		player->get_eflags() |= efl_dirty_abs_transform | efl_dirty_abs_velocity;

		// update bounds.
		if (player_entry.old_maxs_z != FLT_MAX)
		{
			player_entry.view_delta = player->get_abs_origin().z + player_entry.old_maxs_z;

			if (player->get_bounds_change_time() == prev_change_time)
			{
				player->get_view_height() = player->get_origin().z + player_entry.old_maxs_z;
				player->get_bounds_change_time() = record.sim_time;
			}

			player_entry.old_maxs_z = FLT_MAX;
		}
	}

	// update the resolver.
	if (last_record && !last_record->extrapolated)
		update_resolver_state(&rec, last_record);

#ifndef NDEBUG
	if (game->dbg.visualized_matrices_index.get() == player->index())
	{
		const auto tickbase = TIME_TO_TICKS(rec.sim_time);
		auto &shared = game->dbg.shared_data[tickbase % input_max];
		if (shared.idx != tickbase)
		{
			shared.idx = tickbase;
			shared.flags = 0;
		}
		if (shared.flags == 2)
		{
			shared.lag = rec.lag - 1;
			rec.perform_matrix_setup();
			memcpy(shared.client_mat, rec.mat, sizeof(shared.client_mat));
			shared.flags |= 1;

			const auto time = TICKS_TO_TIME(2 + shared.lag);
			debug->draw_player(player, shared.server_mat, sdk::color(0, 0, 255, 130), time);
			debug->draw_player(player, shared.client_mat, sdk::color(0, 255, 0, 130), time);
		}
	}
#endif

	if (true)
	{
		// restore everything.
		player->get_lower_body_yaw_target() = rec.lower_body_yaw_target;
		*player->get_animation_layers() = rec.layers;
		*player->get_anim_state() = rec.state[rec.res_direction];
		player->get_pose_parameter() = rec.poses[rec.res_direction];
		hook_manager.set_abs_angles->call(player, 0, &rec.abs_ang[rec.res_direction]);
		hook_manager.on_latch_interpolated_variables->call(player, 0, latch_animation_var);
	}

	// tickbase drifting records are invalid.
	if (rec.sim_time <= player_entry.compensation_time)
	{
		if (game->cl_predict && game->cl_lagcompensation)
			rec.valid = false;
	}
	// set lagcompensation time.
	else
		player_entry.compensation_time = player->get_simulation_time();

	// fixing llama's dumb exploit.
	if (!player_entry.addt && last_record && TIME_TO_TICKS(last_record->delta_time) < 6)
		rec.valid = false;

	rec.breaking_lc = rec.is_breaking_lagcomp();

	player_entry.bones = bone_changes(player);
	player_entry.fill_current_real_lag();
	player_entry.dormant_miss = 0;
	game->mdl_cache->end_lock();
}

void i_player_list::on_shot_resolve(shot &s, const vec3 &end, const bool spread_miss)
{
	if (!cfg.rage.enable.get())
		return;

	auto &record = s.record;
	const auto player = record->player;
	auto &entry = GET_PLAYER_ENTRY(player);
	auto &resolver = entry.resolver;
	auto &resolver_state = resolver.get_resolver_state(*record);
	auto &positions = resolver_state.eliminated_positions;

	if (player->is_fake_player())
		return;

	// killed?
	if (!player->is_alive())
	{
		resolver_state.set_direction(record->res_direction);

		if (record->res_direction != resolver_state.direction)
		{
			resolver.map_resolver_state(*record, resolver_state);
			for (auto &other : entry.records)
			{
				other.res_direction = resolver.get_resolver_state(other).direction;
				if (other.has_matrix)
					memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
			}
		}

		return;
	}

	// missed on server.
	if (s.server_info.damage <= 0)
	{
		std::bitset<resolver_direction_max> alt_positions{};

		// eliminate resolver directions.
		for (auto i = 0; i < resolver_direction_max; i++)
		{
			const auto pen = trace.wall_penetration(s.start, end, s.record, custom_tracing::bullet_safety::none,
													static_cast<resolver_direction>(i));

			if (pen.did_hit)
			{
				positions.set(i);
				alt_positions.set(i);
			}
		}

		// did we go through all the values?
		if (positions.all())
			resolver.unreliable = min(resolver.unreliable + 1, 3);

		// update brute entries.
		if (positions.all() && !alt_positions.all())
			positions = alt_positions;
		else if (positions.all())
			positions.reset();

		// go to opposite.
		resolver_state.switch_to_opposite(*record, record->res_direction);

		// increase resolver misses.
		if (!spread_miss)
		{
			// stop the detection.
			resolver.stop_detection = true;
			entry.resolver_miss = std::clamp(entry.resolver_miss + 1, 0, 3);
		}
	}
	// did we hit the enemy?
	else
	{
		// alternative brute list.
		std::bitset<resolver_direction_max> alt_positions{};

		// setup distance vector.
		std::array<float, resolver_direction_max> distances;
		distances.fill(FLT_MAX);

		// perform reduction over all positions.
		for (auto i = 0; i < resolver_direction_max; i++)
		{
			const auto pen = trace.wall_penetration(s.start, end, s.record, custom_tracing::bullet_safety::none,
													static_cast<resolver_direction>(i));

			if (!pen.did_hit || pen.hitgroup != s.server_info.hitgroup)
			{
				positions.set(i);
				alt_positions.set(i);
				continue;
			}

			for (auto &impact : s.server_info.impacts)
			{
				const auto length_remaining = (impact - pen.end).length();

				if (length_remaining < distances[i])
					distances[i] = length_remaining;
			}
		}

		// update brute entries.
		if (positions.all() && !alt_positions.all())
		{
			positions = alt_positions;
			resolver.unreliable = max(resolver.unreliable - 1, 0);
		}
		else if (positions.all())
		{
			positions.reset();
			resolver.unreliable = min(resolver.unreliable + 1, 3);
		}
		else
			resolver.unreliable = max(resolver.unreliable - 1, 0);

		// try to minimize distance delta so that we increase the possibility of hitting next time.
		std::pair<resolver_direction, float> current_delta = {record->res_direction, FLT_MAX};
		if (distances[record->res_direction] > .1f)
			for (auto i = 0; i < resolver_direction_max; i++)
				if (distances[i] < current_delta.second)
					current_delta = {static_cast<resolver_direction>(i), distances[i]};

		resolver_state.set_direction(current_delta.first);

		if (record->res_direction != resolver_state.direction)
			resolver.stop_detection = true;
	}

	// did resolver state change?
	if (record->res_direction != resolver_state.direction)
	{
		resolver.map_resolver_state(*record, resolver_state);

		// find all records and update matrices.
		for (auto &other : entry.records)
		{
			other.res_direction = resolver.get_resolver_state(other).direction;
			if (other.has_matrix)
				memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
		}
	}
}

void i_player_list::on_target_player(cs_player_t *const player)
{
	if (!cfg.rage.enable.get() || player->is_fake_player())
		return;

	auto &entry = GET_PLAYER_ENTRY(player);
	auto &resolver = entry.resolver;
	lag_record *latest = nullptr;
	for (auto &rec : entry.records)
		if (rec.valid)
		{
			latest = &rec;
			break;
		}
	if (!latest)
		return;

	// run detection step.
	const auto from = player->get_eye_position();
	const auto to = game->local_player->get_eye_position();
	const auto at_target = calc_angle(from, to);
	resolver.detected_yaw = perform_freestanding(from, {to}, &resolver.previous_freestanding).first;
	resolver.right = std::remainderf(resolver.detected_yaw - at_target.y, yaw_bounds) > 0.f;

	const auto pitch = latest->eye_ang.x - floorf(latest->eye_ang.x / 360.f + .5f) * 360.f;
	if (fabsf(pitch) <= 50.f)
		return;

	if (latest->res_state == resolver_shot)
		return;

	if (entry.resolver.detected_layer && latest->is_moving())
		return;

	// perform reduction over all open positions.
	if (!entry.resolver.stop_detection)
	{
		lag_record copy = *latest;
		auto working_copy = &copy;
		working_copy->res_right = resolver.right;
		auto &resolver_state = resolver.get_resolver_state(*working_copy);

		std::pair<resolver_direction, float> current_delta = {resolver_networked, FLT_MAX};
		for (auto i = 0; i < resolver_direction_max; i++)
		{
			const auto pos = get_hitbox_position(player, cs_player_t::hitbox::head, working_copy->res_mat[i]);
			if (!pos.has_value())
				continue;

			const auto ang = calc_angle(working_copy->origin, *pos);
			const auto delta = fabsf(std::remainderf(ang.y - resolver.detected_yaw, yaw_bounds));

			if (!resolver_state.eliminated_positions.test(i) && delta < current_delta.second)
				current_delta = {static_cast<resolver_direction>(i), delta};
		}

		if (current_delta.first == resolver_networked)
			return;

		// did resolver state change?
		resolver_state.direction = current_delta.first;
		if (working_copy->res_direction != resolver_state.direction)
		{
			resolver.map_resolver_state(*working_copy, resolver_state);

			// find all records and update matrices.
			for (auto &other : entry.records)
			{
				other.res_direction = resolver.get_resolver_state(other).direction;
				if (other.has_matrix)
					memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
			}
		}

		// apply also to the other side.
		working_copy->res_right = !working_copy->res_right;
		auto &other_state = resolver.get_resolver_state(*working_copy);
		other_state.set_direction(current_delta.first);
		resolver.map_resolver_state(*working_copy, resolver_state);

		// find all records and update matrices.
		for (auto &other : entry.records)
		{
			other.res_direction = resolver.get_resolver_state(other).direction;
			if (other.has_matrix)
				memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
		}
	}
}

void i_player_list::refresh_local()
{
	const auto old = game->local_player;

	if (!game->engine_client->is_ingame() || !game->engine_client->is_connected() ||
		(game->local_player &&
		 (game->local_player != reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(
									game->engine_client->get_local_player())) ||
		  !game->local_player->is_alive())))
		game->local_player = nullptr;

	const auto out = !game->local_player && old;
	if (out)
	{
		skin_changer->remove_glove();
		skin_changer->clear_sequence_table();
		local_record = local_shot_record = local_fake_record = lag_record();
		evnt.is_game_end = false;
		for (auto &p : pred.info)
			p.post_pone_time = FLT_MAX;
		rag.reset();
	}

	const auto reset = game->local_player && game->local_player->get_spawn_time() != local_record.recv_time;
	if (reset)
	{
		local_record = local_shot_record = local_fake_record = lag_record(game->local_player);
		local_record.recv_time = game->local_player->get_spawn_time();
		evnt.is_game_end = false;
		for (auto &p : pred.info)
			p.post_pone_time = FLT_MAX;

		auto death_notice = find_hud_element(XOR_STR("CCSGO_HudDeathNotice"));

		if (death_notice - 0x14)
			reinterpret_cast<void(__thiscall *)(uintptr_t)>(game->client.at(functions::clear_death_notices))(
				death_notice - 0x14);
	}

	if (evnt.is_freezetime || reset || out)
		for (auto &entry : player_list.entries)
		{
			entry.visuals.last_update = 0;
			entry.visuals.has_matrix = false;
			entry.visuals.alpha = 0.f;
			entry.dormant_miss = entry.spread_miss = entry.resolver_miss = 0;
		}
}

void i_player_list::merge_local_animation(cs_player_t *const player, user_cmd *cmd)
{
	game->mdl_cache->begin_lock();
	local_record.player = player;

	auto &crc = game->input->verified_commands[cmd->command_number % input_max].crc;
	const auto initial = crc == *reinterpret_cast<int32_t *>(&game->globals->interval_per_tick);

	if (initial && cmd->command_number == game->client_state->last_command + game->client_state->choked_commands + 1)
	{
		tickbase.recent = std::make_pair(player->get_tick_base(), cmd->command_number);
		if (player->get_tick_base() > tickbase.lag.first)
			tickbase.lag = tickbase.recent;
	}

	auto &current = pred.info[cmd->command_number % input_max].animation;
	auto &before = pred.info[(cmd->command_number - 1) % input_max].animation;

	if (current.sequence != cmd->command_number || crc != current.checksum)
	{
		const auto backup = animation_copy(0, player);
		const auto in = &game->input->commands[cmd->command_number % input_max];

		if (in->command_number == cmd->command_number && before.sequence == cmd->command_number - 1 &&
			player->get_spawn_time() == before.spawn_time)
		{
			before.restore(player);
			local_record.update_animations(in);
		}
		else if (current.sequence == cmd->command_number && player->get_spawn_time() == current.spawn_time)
		{
			current.restore(player);
			player->get_strafing() = current.get_strafing;
		}
		else if (game->last_full_update_frame != -1 &&
				 cmd->command_number - game->last_full_update_frame < max(TIME_TO_TICKS(1.25f), input_max))
		{
			// lost connection, attempt to catch up...
			pred.info[game->last_full_update_frame % input_max].animation.restore(player);
			for (auto i = game->last_full_update_frame + 1; i <= cmd->command_number; i++)
				local_record.update_animations(&game->input->commands[i % input_max]);
		}
		else
		{
			player->get_lower_body_yaw_target() = 0.f;
			for (auto &layer : *player->get_animation_layers())
				layer = animation_layer();
			player->get_anim_state()->player = player;
			player->get_anim_state()->reset();
			local_record = lag_record();
			local_record.player = player;
			local_record.update_animations(in);
			local_record.addon.vm = XOR_32(183);
			before = animation_copy(cmd->command_number - 1, player);
			local_record.update_animations(in);
		}

		current = animation_copy(cmd->command_number, player);

		if (!initial)
			backup.restore(player);
		else
			local_record.has_visual_matrix = false;
	}
	else
		player->get_strafing() = current.get_strafing;

	game->mdl_cache->end_lock();
}

void i_player_list::update_addon_bits(cs_player_t *const player)
{
	static const auto wpn_c4 = XOR_STR_STORE("weapon_c4");
	static const auto wpn_shield = XOR_STR_STORE("weapon_shield");
	XOR_STR_STACK(c4, wpn_c4);
	XOR_STR_STACK(shield, wpn_shield);

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	const auto item_definition = wpn ? wpn->get_item_definition_index() : item_definition_index(-1);
	const auto world_model =
		wpn ? reinterpret_cast<entity_t *const>(
				  game->client_entity_list->get_client_entity_from_handle(wpn->get_weapon_world_model()))
			: nullptr;

	int32_t new_bits = 0;
	auto weapon_visible = world_model ? !(world_model->get_effects() & ef_nodraw) : true;

	auto flash_bang = player->get_ammo_count(FNV1A("AMMO_TYPE_FLASHBANG"));
	if (weapon_visible && item_definition == item_definition_index::weapon_flashbang)
		flash_bang--;

	if (flash_bang >= 1)
		new_bits |= 0x1;

	if (flash_bang >= 2)
		new_bits |= 0x2;

	if (player->get_ammo_count(FNV1A("AMMO_TYPE_HEGRENADE")) &&
		(item_definition != item_definition_index::weapon_hegrenade || !weapon_visible))
		new_bits |= 0x4;

	if (player->get_ammo_count(FNV1A("AMMO_TYPE_SMOKEGRENADE")) &&
		(item_definition != item_definition_index::weapon_smokegrenade || !weapon_visible))
		new_bits |= 0x8;

	if (player->get_ammo_count(FNV1A("AMMO_TYPE_DECOY")) &&
		(item_definition != item_definition_index::weapon_decoy || !weapon_visible))
		new_bits |= 0x200;

	if (player->get_ammo_count(FNV1A("AMMO_TYPE_TAGRENADE")) &&
		(item_definition != item_definition_index::weapon_tagrenade || !weapon_visible))
		new_bits |= 0x1000;

	if (player->owns_this_type(c4) && (item_definition != item_definition_index::weapon_c4 || !weapon_visible))
		new_bits |= 0x10;

	if (player->get_player_defuser())
		new_bits |= 0x20;

	const auto rifle = player->get_slot(0);
	if (rifle && (rifle != wpn || !weapon_visible))
	{
		new_bits |= 0x40;
		player->get_primary_addon() = int32_t(rifle->get_item_definition_index());
	}
	else
		player->get_primary_addon() = 0;

	const auto pistol = player->get_slot(1);
	if (pistol && (pistol != wpn || !weapon_visible))
	{
		new_bits |= 0x80;
		if (pistol->get_item_definition_index() == item_definition_index::weapon_elite)
			new_bits |= 0x100;
		player->get_secondary_addon() = int32_t(pistol->get_item_definition_index());
	}
	else if (pistol && pistol->get_item_definition_index() == item_definition_index::weapon_elite)
	{
		new_bits |= 0x100;
		player->get_secondary_addon() = int32_t(pistol->get_item_definition_index());
	}
	else
		player->get_secondary_addon() = 0;

	const auto knife = player->get_slot(2);
	if (knife && (knife != wpn || !weapon_visible))
		new_bits |= 0x400;

	if (player->owns_this_type(shield) && (item_definition != item_definition_index::weapon_shield || !weapon_visible))
		new_bits |= 0x2000;

	if (player->index() == game->engine_client->get_local_player())
		skin_changer->on_addon_bits(player, new_bits);

	player->get_addon_bits() = new_bits;
}

void i_player_list::build_fake_animation(cs_player_t *const player, user_cmd *const cmd)
{
	// store off real animation.
	game->mdl_cache->begin_lock();
	pred.repredict(cmd);
	const auto backup = animation_copy(0, player);
	auto &log = pred.info[tickbase.lag.second % input_max].animation;
	if (log.sequence == tickbase.lag.second)
		log.restore(player);
	const auto real_layers = *player->get_animation_layers();
	const auto buttons = cmd->buttons;

	// restore buttons.
	for (auto i = 0; i < game->client_state->choked_commands; i++)
		cmd->buttons |= game->input->commands[(cmd->command_number - i - 1) % input_max].buttons & user_cmd::jump;

	// emplace last fake.
	if (local_fake_record.has_state)
	{
		const auto bak = *player->get_anim_state();
		*player->get_anim_state() = local_fake_record.state[resolver_networked];
		bak.copy_meta(player->get_anim_state());
		*player->get_animation_layers() = local_fake_record.custom_layers[resolver_networked];
		player->get_pose_parameter() = local_fake_record.poses[resolver_networked];
		player->get_anim_state()->feet_cycle = player->get_animation_layers()->at(6).cycle;
		player->get_anim_state()->feet_weight = player->get_animation_layers()->at(6).weight;
	}

	// animate fake.
	local_fake_record.update_animations(cmd);

	// selectively merge in the server layers now.
	const auto &entry = GET_PLAYER_ENTRY(game->local_player);
	for (auto i = 0u; i < game->local_player->get_animation_layers()->size(); i++)
	{
		auto &client = game->local_player->get_animation_layers()->at(i);
		auto &real = real_layers[i];

		client.order = real.order;

		if (i == 1 || (i > 7 && i < 12))
			client = real;
	}

	// store fake record.
	cmd->buttons = buttons;
	local_fake_record.has_state = true;
	local_fake_record.state[resolver_networked] = *player->get_anim_state();
	local_fake_record.layers = local_fake_record.custom_layers[resolver_networked] = *player->get_animation_layers();
	local_fake_record.poses[resolver_networked] = player->get_pose_parameter();
	local_fake_record.abs_ang[resolver_networked] = {0.f, player->get_anim_state()->foot_yaw, 0.f};
	local_fake_record.origin = player->get_origin();
	local_fake_record.abs_origin = player->get_abs_origin();
	local_fake_record.eye_ang = *player->eye_angles();
	local_fake_record.sim_time = game->globals->curtime;
	local_fake_record.obb_maxs = player->get_collideable()->obb_maxs();
	local_fake_record.has_matrix = false;
	if (game->input->camera_in_third_person && !cfg.local_visuals.fake_chams.mode.get().test(cfg_t::chams_disabled))
	{
		const auto view = *game->local_player->eye_angles();
		game->prediction->set_local_view_angles({cmd->viewangles.x, cmd->viewangles.y, 0.f});
		local_fake_record.perform_matrix_setup();
		game->prediction->set_local_view_angles(view);
	}

	backup.restore(player);
	game->mdl_cache->end_lock();
}

void i_player_list::perform_animations(lag_record *const current, lag_record *const last, bool long_path)
{
	const auto player = current->player;
	const auto state = player->get_anim_state();
	auto &entry = GET_PLAYER_ENTRY(player);
	auto target_record = last ? last : current;
	const auto is_onground = current->flags & cs_player_t::on_ground;
	const auto is_fake = player->is_fake_player();
	const auto full_setup = player->is_enemy();

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));

	// did the player shoot in this frame chunk?
	current->shot = last && !current->extrapolated && wpn && wpn->is_shootable_weapon() &&
					wpn->get_last_shot_time() > last->sim_time && wpn->get_last_shot_time() <= current->sim_time;

	// load last addon.
	if (last)
		current->addon = last->addon;

	// reset spread misses, if possible.
	if (!current->extrapolated && current->velocity.length() > 200.f && current->flags & cs_player_t::flags::on_ground)
		entry.spread_miss = 0;

	// correct velocity when standing.
	if (!current->is_moving_on_server() && is_onground)
		current->velocity = vec3();

	// steal the velocity from the alive loop.
	if (last && !current->extrapolated && current->layers[11].playback_rate == last->layers[11].playback_rate &&
		wpn == state->weapon && current->lag > 1 && is_onground)
	{
		const auto speed = min(current->velocity.length2d(), max_cs_player_move_speed);
		const auto max_movement_speed = wpn ? max(wpn->get_max_speed(), .001f) : max_cs_player_move_speed;
		const auto portion = std::clamp(1.f - current->layers[11].weight, 0.f, 1.f);
		const auto vel = (portion * .35f + .55f) * max_movement_speed;
		if ((portion > 0.f && portion < 1.f) || (portion == 0.f && speed > vel) || (portion == 1.f && speed < vel))
		{
			auto v1 = current->velocity;
			v1.x = v1.x / speed * vel;
			v1.y = v1.y / speed * vel;
			if (v1.is_valid())
				current->velocity = v1;
		}
	}

	// backup stuff.
	const auto backup_poses = player->get_pose_parameter();
	const auto backup_layers = *player->get_animation_layers();
	const auto backup_state = *state;
	const auto backup_abs_origin = player->get_abs_origin();
	const auto backup_abs_angle = player->get_abs_angle();
	const auto backup_lby = player->get_lower_body_yaw_target();
	const auto backup_strafing = player->get_strafing();
	const auto backup_velocity = player->get_velocity();
	const auto backup_eye_angles = player->get_eye_angles();
	const auto backup_curtime = game->globals->curtime;
	const auto backup_frametime = game->globals->frametime;
	auto backup_move = move_delta_t();
	backup_move.store(player);

	// set simulation start.
	player->set_abs_origin(target_record->origin);
	player->get_flags() = target_record->flags;
	player->get_velocity_modifier() = target_record->velocity_modifier;
	if (!target_record->final_velocity.is_valid())
		target_record->final_velocity = {0.f, 0.f, 0.f};
	player->get_velocity() = target_record->final_velocity;
	player->get_duck_amount() = target_record->duck;
	player->get_ground_entity() = (target_record->flags & cs_player_t::on_ground)
									  ? game->client_entity_list->get_client_entity(0)->get_handle()
									  : -1;
	player->get_move_state() = target_record->move_state;
	player->get_stamina() = target_record->stamina;
	player->get_step_size() = GET_CONVAR_FLOAT("sv_stepsize");
	player->get_last_position_at_full_crouch_speed() = {FLT_MAX, FLT_MAX};

	// run simulation.
	user_cmd cmd{};
	auto prev_buttons = cmd.buttons;
	const auto target_simtime = current->sim_time;
	float lower_body_yaw[resolver_direction_max];
	for (auto j = 0; j < current->lag; j++)
	{
		const auto first_frame = j == 0;
		const auto last_frame = j == current->lag - 1;

		// setup animation time.
		game->globals->curtime = current->sim_time = target_simtime - TICKS_TO_TIME(current->lag - j - 1);
		cmd.command_number = TIME_TO_TICKS(game->globals->curtime);
		game->globals->frametime = game->globals->interval_per_tick;

		// intermediate movement.
		const auto speed = player->get_velocity().length2d();
		if (current->lag > 1 && last && !current->extrapolated && full_setup && game->rules->data)
		{
			const auto duck_delta = current->duck - player->get_duck_amount();
			auto unduck_frame = current->lag, duck_frame = current->lag;
			if (duck_delta < 0.f)
			{
				const auto amnt_per_frame = max(1.5f, player->get_duck_speed()) * TICKS_TO_TIME(1);
				const auto frames_to_unduck = static_cast<int32_t>(ceilf(-duck_delta / amnt_per_frame));
				unduck_frame = max(0, current->lag - 1 - frames_to_unduck);
			}
			else if (duck_delta > 0.f)
			{
				const auto amnt_per_frame = player->get_duck_speed() * .8f * TICKS_TO_TIME(1);
				const auto frames_to_duck = static_cast<int32_t>(ceilf(duck_delta / amnt_per_frame));
				duck_frame = max(0, current->lag - 1 - frames_to_duck);
			}

			if (j >= duck_frame || current->duck == 1.f)
				cmd.buttons |= user_cmd::duck;

			if (j >= unduck_frame)
				cmd.buttons &= ~user_cmd::duck;

			if (first_frame)
			{
				if (target_record->duck > 0.f)
					player->get_ducking() = true;
				else
					player->get_ducking() = false;

				if (target_record->duck == 1.f)
				{
					player->get_ducked() = true;
					player->get_ducking() = false;
				}
				else
					player->get_ducked() = false;

				prev_buttons = cmd.buttons;

				if (target_record->flags & cs_player_t::on_ground && !(current->flags & cs_player_t::on_ground))
					prev_buttons |= user_cmd::jump;
			}

			const auto next_duck = approach(cmd.buttons & user_cmd::duck ? 1.f : 0.f, player->get_duck_amount(),
											player->get_duck_speed() * TICKS_TO_TIME(1));
			const auto factor = next_duck * .34f + 1.f - next_duck;

			cmd.viewangles.y = calc_angle(player->get_abs_origin(), current->origin).y;
			cmd.forwardmove = current->velocity.length2d();
			if (speed <= .1f && current->is_moving_on_server() && current->flags & cs_player_t::on_ground)
				cmd.forwardmove = cmd.command_number % 2 ? -1.01f : 1.01f;
			cmd.sidemove = 0.f;

			if (speed < target_record->final_velocity.length2d())
			{
				auto data = game->cs_game_movement->setup_move(player, &cmd);
				slow_movement(&data, speed);
				std::tie(cmd.forwardmove, cmd.sidemove) = std::tie(data.forward_move, data.side_move);
			}

			cmd.forwardmove /= factor;
			cmd.sidemove /= factor;

			auto data = game->cs_game_movement->setup_move(player, &cmd);
			data.old_buttons = prev_buttons | user_cmd::bull_rush;
			data.buttons |= user_cmd::bull_rush;
			const auto previous_duck = player->get_duck_amount();
			const auto delta = game->cs_game_movement->process_movement(player, &data);
			prev_buttons = data.buttons;
			delta.restore(player);

			if (entry.old_maxs_z != FLT_MAX)
			{
				if (player->get_duck_amount() == 1.f && previous_duck < 1.f)
				{
					player->get_view_height() = data.abs_origin.z + game->rules->data->get_view_vectors()->hull_max.z;
					player->get_bounds_change_time() = game->globals->curtime;
				}
				else if (player->get_duck_amount() < 1.f && previous_duck == 1.f)
				{
					player->get_view_height() =
						data.abs_origin.z + game->rules->data->get_view_vectors()->duck_hull_max.z;
					player->get_bounds_change_time() = game->globals->curtime;
				}
			}

			if (!(target_record->flags & cs_player_t::on_ground) && !(current->flags & cs_player_t::on_ground) &&
				player->get_flags() & cs_player_t::on_ground)
				cmd.buttons |= user_cmd::jump;
			else
				cmd.buttons &= ~user_cmd::jump;

			player->set_abs_origin(data.abs_origin);

			const auto lerp = static_cast<float>(j + 1) / static_cast<float>(current->lag);
			const auto current_len =
				interpolate(target_record->velocity.length2d(), current->velocity.length2d(), lerp);
			const auto current_vel =
				interpolate(vec3{target_record->velocity.x, target_record->velocity.y, 0.f}.normalize(),
							vec3{current->velocity.x, current->velocity.y, 0.f}.normalize(), lerp + .5f)
					.normalize();
			player->get_velocity().x = current_vel.x * current_len;
			player->get_velocity().y = current_vel.y * current_len;
			player->get_velocity().z = data.velocity.z;

			// steal the velocity from the alive loop.
			if (last_frame && current->layers[11].playback_rate == last->layers[11].playback_rate &&
				wpn == state->weapon && is_onground)
			{
				const auto speed = min(player->get_velocity().length2d(), max_cs_player_move_speed);
				const auto max_movement_speed = wpn ? max(wpn->get_max_speed(), .001f) : max_cs_player_move_speed;
				const auto portion = std::clamp(1.f - current->layers[11].weight, 0.f, 1.f);
				const auto vel = (portion * .35f + .55f) * max_movement_speed;
				if ((portion > 0.f && portion < 1.f) || (portion == 0.f && speed > vel) ||
					(portion == 1.f && speed < vel))
				{
					auto v1 = player->get_velocity();
					v1.x = v1.x / speed * vel;
					v1.y = v1.y / speed * vel;
					if (v1.is_valid())
						player->get_velocity() = v1;
				}
			}
		}
		else if (current->extrapolated && game->rules->data)
		{
			const auto p1 = entry.records.size() > 2 ? &entry.records[2] : nullptr;
			const auto p2 = entry.records.size() > 3 ? &entry.records[3] : nullptr;

			vec3 pchange, vchange;
			if (p1 && p2)
			{
				const auto delta = (p1->velocity - p2->velocity) / p1->lag;
				vchange = (last->velocity - p1->velocity) / last->lag;
				pchange = vchange - delta;
			}

			if (current->duck > 0.f)
				cmd.buttons |= user_cmd::duck;
			else
				cmd.buttons &= ~user_cmd::duck;

			if (first_frame)
			{
				if (player->get_duck_amount() > 0.f)
					player->get_ducking() = true;
				else
					player->get_ducking() = false;

				if (player->get_duck_amount() == 1.f)
				{
					player->get_ducked() = true;
					player->get_ducking() = false;
				}
				else
					player->get_ducked() = false;

				prev_buttons = cmd.buttons;
			}

			const auto next_duck = approach(cmd.buttons & user_cmd::duck ? 1.f : 0.f, player->get_duck_amount(),
											player->get_duck_speed() * TICKS_TO_TIME(1));
			const auto factor = next_duck * .34f + 1.f - next_duck;

			angle achange;
			const auto pvel = player->get_velocity() + pchange;
			vector_angles(vec3(pvel).normalize(), achange);
			cmd.viewangles.y = achange.y;
			cmd.forwardmove = speed > 5.f ? forward_bounds : (j % 2 ? 1.01f : -1.01f);
			cmd.forwardmove /= factor;
			cmd.sidemove = 0.f;

			const auto target_vel = p1 ? p1->velocity.length2d() : 0.f;
			if (!(player->get_flags() & cs_player_t::flags::on_ground))
			{
				angle vel_ang;
				vector_angles(vec3(player->get_velocity()).normalize(), vel_ang);
				const auto vel_ang_diff = angle_diff(vel_ang.y, achange.y);

				if (fabsf(vel_ang_diff) > 20.f)
				{
					cmd.forwardmove = 0.f;
					cmd.sidemove = vel_ang_diff > 0.f ? side_bounds : -side_bounds;
					cmd.sidemove /= factor;
				}
			}
			else if (p1 && p1->flags & cs_player_t::on_ground && (!p2 || p2->flags & cs_player_t::on_ground) &&
						 speed < target_vel - 5.f * last->lag && player->get_stamina() == 0.f &&
						 player->get_velocity_modifier() == 1.f && !state->in_hit_ground ||
					 speed < 2.f)
			{
				auto data = game->cs_game_movement->setup_move(player, &cmd);
				slow_movement(&data, 1.01f);
				cmd.forwardmove = data.forward_move / factor;
				cmd.sidemove = data.side_move / factor;
			}
			else if (p1 && p1->flags & cs_player_t::on_ground && (!p2 || p2->flags & cs_player_t::on_ground) &&
					 speed < target_vel + 2.f * last->lag && speed > 5.f &&
					 (!wpn || p1->velocity.length2d() < wpn->get_max_speed() - 2.f * last->lag))
			{
				auto data = game->cs_game_movement->setup_move(player, &cmd);
				slow_movement(&data, pvel.length2d());
				cmd.forwardmove = data.forward_move / factor;
				cmd.sidemove = data.side_move / factor;
			}

			auto data = game->cs_game_movement->setup_move(player, &cmd);
			data.old_buttons = prev_buttons | user_cmd::bull_rush;
			data.buttons |= user_cmd::bull_rush;
			const auto previous_duck = player->get_duck_amount();
			const auto delta = game->cs_game_movement->process_movement(player, &data);
			prev_buttons = data.buttons;
			delta.restore(player);

			if (player->get_duck_amount() == 1.f && previous_duck < 1.f)
			{
				player->get_view_height() = data.abs_origin.z + game->rules->data->get_view_vectors()->hull_max.z;
				player->get_bounds_change_time() = game->globals->curtime;
				player->get_view_offset().z = game->rules->data->get_view_vectors()->duck_view.z;
				player->get_collideable()->obb_maxs().z = game->rules->data->get_view_vectors()->duck_hull_max.z;
			}
			else if (player->get_duck_amount() < 1.f && previous_duck == 1.f)
			{
				player->get_view_height() = data.abs_origin.z + game->rules->data->get_view_vectors()->duck_hull_max.z;
				player->get_bounds_change_time() = game->globals->curtime;
				player->get_view_offset().z = game->rules->data->get_view_vectors()->view.z;
				player->get_collideable()->obb_maxs().z = game->rules->data->get_view_vectors()->hull_max.z;
			}

			player->set_abs_origin(data.abs_origin);
			current->velocity = player->get_velocity() = data.velocity;

			if (last_frame)
				current->origin = data.abs_origin;

			player->get_eye_angles() = current->eye_ang;
		}
		else
			player->get_velocity() = current->velocity;

		// set correct velocity for the hook.
		if (last_frame && current->layers[6].playback_rate <= 0.f && is_onground)
			player->get_velocity() = vec3();

		// check if game movement screwed up.
		if (!player->get_velocity().is_valid())
			player->get_velocity() = current->velocity;

		// make sure we don't drop below our margin.
		if (player->get_velocity().length2d_sqr() < 1.1f * 1.1f && current->is_moving_on_server() &&
			current->flags & cs_player_t::on_ground)
			player->get_velocity() = {cmd.command_number % 2 ? -1.01f : 1.01f, 0.f, 0.f};

		// set final values.
		if (last_frame)
		{
			if (!current->extrapolated)
			{
				player->get_duck_amount() = current->duck;
				player->get_flags() = current->flags;
				player->set_abs_origin(current->origin);
				player->get_velocity_modifier() = current->velocity_modifier;
				player->get_stamina() = current->stamina;
				player->get_move_state() = current->move_state;
			}
			else
			{
				current->stamina = player->get_stamina();
				current->velocity_modifier = player->get_velocity_modifier();
				current->flags = player->get_flags();
			}
		}

		if (!long_path)
			continue;

		// perform all possible animations.
		anim_state pred{};
		for (auto i = 0; i < resolver_direction_max; i++)
		{
			// bots and teammates only get the networked angle animated.
			if ((!full_setup || is_fake) && i != resolver_networked)
				continue;

			if (i == resolver_max_max || i == resolver_min_min || i == resolver_min_extra || i == resolver_max_extra)
				continue;

			// copy back pre-animation data.
			if (first_frame)
			{
				player->get_pose_parameter() = target_record->poses[i];
				const auto &layer = current->extrapolated ? target_record->custom_layers[i] : target_record->layers;
				*player->get_animation_layers() = layer;
				player->get_strafing() = target_record->strafing;
				*state = target_record->state[i];
				backup_state.copy_meta(state);
				state->feet_cycle = layer[6].cycle;
				state->feet_weight = layer[6].weight;
				state->strafe_sequence = layer[7].sequence;
				state->strafe_change_cycle = layer[7].cycle;
				state->strafe_change_weight = layer[7].weight;
				state->acceleration_weight = layer[12].weight;
				player->get_lower_body_yaw_target() = target_record->lower_body_yaw_target;
				player->set_abs_angle(target_record->abs_ang[i]);
			}
			else
			{
				*player->get_animation_layers() = current->custom_layers[i];
				*state = current->state[i];
				backup_state.copy_meta(state);
				player->get_pose_parameter() = current->poses[i];
				player->get_lower_body_yaw_target() = lower_body_yaw[i];
				player->set_abs_angle(current->abs_ang[i]);
			}

			// resolve intermediate animations.
			if (!is_fake)
			{
				switch (i)
				{
				case resolver_max:
				case resolver_min:
				{
					constexpr auto half_bounds = yaw_bounds * .5f;
					const auto approach_epsilon =
						pred.last_increment * (30.f + 20.f * state->ground_fraction) * 2.f + 1.f;

					auto foot_yaw = i == resolver_max ? half_bounds : -half_bounds;
					foot_yaw = std::clamp(foot_yaw, -half_bounds + approach_epsilon, half_bounds - approach_epsilon);
					player->get_eye_angles().y = std::remainderf(current->eye_ang.y + foot_yaw, yaw_bounds);
					break;
				}
				default:
					player->get_eye_angles().y = std::remainderf(current->eye_ang.y, yaw_bounds);
					break;
				}

				if (!last_frame && i != resolver_networked)
					state->foot_yaw = player->get_eye_angles().y;
			}

			// already shot?
			const auto has_shot = current->shot && TIME_TO_TICKS(wpn->get_last_shot_time()) <= cmd.command_number;
			if (has_shot)
				player->get_eye_angles() = current->eye_ang;

			// set final values.
			if (last_frame)
			{
				player->get_eye_angles() = current->eye_ang;

				if (!current->extrapolated)
					player->get_strafing() = current->strafing;
			}

			if (i == resolver_networked)
				pred = current->predict_animation_state();

			// estimate strafing.
			if (!last_frame)
			{
				if (current->strafing || !current->extrapolated && player->get_flags() & cs_player_t::on_ground &&
											 player->get_velocity().length2d() > .1f &&
											 current->layers[7].weight > 0.f &&
											 (current->layers[7].weight >= .999f - pred.last_increment * 5.f ||
											  last && current->layers[7].weight >=
														  last->layers[7].weight - pred.last_increment * 5.f - .001f))
				{
					if (state->strafe_sequence != -1 || state->strafe_change_weight <= 0.f)
						player->get_strafing() = true;
				}
			}

			// perform animation.
			const auto last_ground_fraction = state->ground_fraction;
			current->update_animations(current->extrapolated ? &cmd : nullptr);

			// set lby.
			if (!current->extrapolated)
				player->get_lower_body_yaw_target() = current->lower_body_yaw_target;
			else if (i == resolver_networked)
				current->lower_body_yaw_target = player->get_lower_body_yaw_target();

			// TODO: sync lby here.

			// store current pose parameters.
			current->abs_ang[i] = {0.f, state->foot_yaw, 0.f};
			current->poses[i] = player->get_pose_parameter();
			current->state[i] = *player->get_anim_state();
			current->custom_layers[i] = *player->get_animation_layers();
			lower_body_yaw[i] = player->get_lower_body_yaw_target();
		}
	}

	// store final predicted velocity.
	current->final_velocity = player->get_velocity();

	// keep stuff we want for extrapolation.
	const auto ground_entity = player->get_ground_entity();
	const auto view_offset = player->get_view_offset();

	// restore stuff.
	player->get_pose_parameter() = backup_poses;
	*player->get_animation_layers() = backup_layers;
	*state = backup_state;
	player->set_abs_origin(backup_abs_origin);
	player->set_abs_angle(backup_abs_angle);
	player->get_lower_body_yaw_target() = backup_lby;
	player->get_strafing() = backup_strafing;
	player->get_velocity() = backup_velocity;
	player->get_eye_angles() = backup_eye_angles;
	game->globals->curtime = backup_curtime;
	game->globals->frametime = backup_frametime;
	backup_move.restore(player);

	if (current->extrapolated)
	{
		player->get_ground_entity() = ground_entity;
		player->get_view_offset() = view_offset;
	}

	// set roll resolver directions.
	current->abs_ang[resolver_max_max] = current->abs_ang[resolver_max];
	current->abs_ang[resolver_max_extra] = current->abs_ang[resolver_max];
	current->abs_ang[resolver_min_min] = current->abs_ang[resolver_min];
	current->abs_ang[resolver_min_extra] = current->abs_ang[resolver_min];
	current->poses[resolver_max_max] = current->poses[resolver_max];
	current->poses[resolver_max_extra] = current->poses[resolver_max];
	current->poses[resolver_min_min] = current->poses[resolver_min];
	current->poses[resolver_min_extra] = current->poses[resolver_min];
	current->state[resolver_max_max] = current->state[resolver_max];
	current->state[resolver_max_extra] = current->state[resolver_max];
	current->state[resolver_min_min] = current->state[resolver_min];
	current->state[resolver_min_extra] = current->state[resolver_min];
	current->custom_layers[resolver_max_max] = current->custom_layers[resolver_max];
	current->custom_layers[resolver_max_extra] = current->custom_layers[resolver_max];
	current->custom_layers[resolver_min_min] = current->custom_layers[resolver_min];
	current->custom_layers[resolver_min_extra] = current->custom_layers[resolver_min];
}

void i_player_list::update_resolver_state(lag_record *const current, lag_record *const last)
{
	if (!cfg.rage.enable.get() || current->player->is_fake_player() || !current->player->is_enemy() ||
		!game->local_player || !game->local_player->is_alive())
	{
		current->res_state = resolver_default;
		current->res_direction = resolver_networked;
		return;
	}

	const auto player = current->player;
	auto &entry = GET_PLAYER_ENTRY(player);
	auto &resolver = entry.resolver;

#ifndef NDEBUG
	if (game->dbg.force_resolver_direction.get() != -1)
	{
		current->res_state = resolver_default;
		current->res_direction = resolver_direction(uint32_t(game->dbg.force_resolver_direction.get()));
		for (auto i = 0; i < 3; i++)
			resolver.states[0][i].direction = resolver.states[1][i].direction = current->res_direction;
		return;
	}
#endif

	// on shot?
	if (current->shot)
	{
		auto target = resolver.states[resolver.right][resolver_default].direction;

		switch (target)
		{
		case resolver_max_max:
			target = resolver_min_min;
			break;
		case resolver_max:
		case resolver_max_extra:
			target = resolver_min;
			break;
		case resolver_min:
		case resolver_min_extra:
			target = resolver_max;
			break;
		case resolver_min_min:
			target = resolver_max_max;
			break;
		default:
			target = resolver_networked;
			break;
		}

		if (!resolver.states[resolver.right][resolver_shot].eliminated_positions.test(target))
			resolver.states[resolver.right][resolver_shot].direction = target;

		current->res_state = resolver_shot;
		current->res_right = resolver.right;
		current->res_direction = resolver.get_resolver_state(*current).direction;
		return;
	}

	// start pitch switch timer on pitch switch.
	const auto pitch = current->eye_ang.x - floorf(current->eye_ang.x / 360.f + .5f) * 360.f;
	if (fabsf(pitch) < 50.f)
	{
		if (resolver.pitch_timer <= 0.f)
			resolver.pitch_timer = game->globals->curtime;
	}
	else
		resolver.pitch_timer = 0.f;

	// determine rotation momentum and antiaim characteristics.
	const auto delta = std::remainderf(current->eye_ang.y - last->eye_ang.y, yaw_bounds);
	resolver.continuous_momentum += delta;
	resolver.steadiness += fabsf(delta) > flip_margin ? .1f : -.1f;
	resolver.steadiness = std::clamp(resolver.steadiness, 0.f, 1.f);

	// force safe points until pitch is steady.
	const auto force_safepoint = resolver.pitch_timer > 0.f && resolver.pitch_timer >= game->globals->curtime - 1.f;
	if (force_safepoint && !cfg.rage.hack.secure_point->test(cfg_t::secure_point_disabled))
		current->force_safepoint = true;

	// we don't have any clue where in the cycle he shot, just ignore them.
	if (!current->shot && last->shot)
	{
		current->res_state = resolver_shot;
		current->res_right = resolver.right;
		current->res_direction = resolver.get_resolver_state(*current).direction;
		return;
	}

	// should we switch to or from the secondary mode?
	if (resolver.current_state == resolver_flip && resolver.continuous_momentum < -flip_margin)
	{
		resolver.current_state = resolver_default;
		resolver.continuous_momentum = 0.f;
	}
	else if (resolver.current_state == resolver_default && resolver.continuous_momentum > flip_margin)
	{
		resolver.current_state = resolver_flip;
		resolver.continuous_momentum = 0.f;
	}
	// account for flip from -180 to 180 deg.
	else if (fabsf(delta) >= 179.f)
	{
		resolver.current_state = resolver.current_state == resolver_default ? resolver_flip : resolver_default;
		resolver.continuous_momentum = 0.f;
	}

	// write resolver data to record.
	current->res_state = resolver.current_state;
	current->res_right = resolver.right;

	auto &resolver_state = resolver.get_resolver_state(*current);
	current->res_direction = resolver_state.direction;

	if (!current->valid || !last->valid || current->lag == 1 || force_safepoint)
		return;

	angle move, last_move;
	vector_angles(current->velocity, move);

	// perform layer 6 matching.
	if (current->is_moving() && last->is_moving() // both moving.
		&& current->layers[6].weight >= last->layers[6].weight &&
		current->layers[6].playback_rate >= last->layers[6].playback_rate // accelerating.
		&& current->flags & cs_player_t::flags::on_ground &&
		last->flags & cs_player_t::flags::on_ground // is player grounded?
		&& !resolver.stop_detection)				// default resolve mode.
	{
		auto best_match = std::make_pair(resolver_networked, FLT_MAX);

		for (auto i = 0; i < resolver_direction_max; i++)
		{
			if (i != resolver_max && i != resolver_min)
				continue;

			if (current->layers[6].sequence != current->custom_layers[i][6].sequence)
				continue;

			const auto delta_weight = fabsf(current->custom_layers[i][6].weight - current->layers[6].weight);
			const auto delta_cycle = fabsf(current->custom_layers[i][6].cycle - current->layers[6].cycle);
			const auto delta_rate =
				fabsf(current->custom_layers[i][6].playback_rate - current->layers[6].playback_rate);
			const auto delta_total = delta_weight + delta_cycle + delta_rate;

			if (delta_total < best_match.second)
				best_match = {static_cast<resolver_direction>(i), delta_total};

			if (delta_weight < .000001 || delta_cycle < .000001f || delta_rate < .000001f)
				best_match = {static_cast<resolver_direction>(i), 0.f};
		}

		// update resolver.
		if (best_match.second < FLT_MAX)
		{
			resolver.states[false][resolver_default].set_direction(best_match.first);
			resolver.states[false][resolver_flip].set_direction(best_match.first);
			resolver.states[true][resolver_default].set_direction(best_match.first);
			resolver.states[true][resolver_flip].set_direction(best_match.first);

			current->res_direction = resolver_state.direction;
			current->resolved = resolver.detected_layer = true;
		}
	}

	// update matrices on all records.
	// find all records and update matrices.
	for (auto &other : entry.records)
	{
		other.res_direction = resolver.get_resolver_state(other).direction;
		if (other.has_matrix)
			memcpy(other.mat, other.res_mat[other.res_direction], sizeof(other.mat));
	}
}

void i_player_list::build_activity_modifiers(cs_player_t *const player, std::array<uint16_t, 3> &modifiers,
											 bool uh) const
{
	static const auto moving = XOR_STR_STORE("moving");
	static const auto crouch = XOR_STR_STORE("crouch");
	static const auto underhand = XOR_STR_STORE("underhand");

	modifiers.fill(0xFFFF);
	auto j = 0;

	const auto add_modifier = [&modifiers, &j](const char *name)
	{
		char lookup[32];
		strcpy_s(lookup, sizeof(lookup), name);
		modifiers[j++] = game->modifier_table->find_or_insert(lookup);
	};

	if (!player || !player->is_alive())
		return;

	const auto state = player->get_anim_state();
	add_modifier(ent_ren.is_updating_animstate ? XOR_STR("sniper") : state->get_active_weapon_prefix());

	if (state->running_speed > .25f)
	{
		XOR_STR_STACK(m, moving);
		add_modifier(m);
	}

	if (state->duck_amount > .55f)
	{
		XOR_STR_STACK(c, crouch);
		add_modifier(c);
	}

	if (uh)
	{
		XOR_STR_STACK(u, underhand);
		add_modifier(u);
	}
}

float i_player_list::calculate_yaw_modifier(cs_player_t *const player, const vec3 &velocity, float duck,
											float ground_fraction)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));

	const auto speed = min(velocity.length2d(), max_cs_player_move_speed);
	const auto max_movement_speed = wpn ? max(wpn->get_max_speed(), .001f) : max_cs_player_move_speed;
	const auto movement_speed = std::clamp(speed / (max_movement_speed * .520f), 0.f, 1.f);
	const auto ducking_speed = std::clamp(speed / (max_movement_speed * .340f), 0.f, 1.f);

	auto yaw_modifier = (((ground_fraction * -.3f) - .2f) * movement_speed) + 1.f;

	if (duck > 0.f)
	{
		const auto ducking_modifier = std::clamp(ducking_speed, 0.f, 1.f);
		yaw_modifier += (duck * ducking_modifier) * (.5f - yaw_modifier);
	}

	return yaw_modifier;
}

i_player_list::player_entry::player_entry(cs_player_t *const player)
{
	if (player == nullptr)
		INVALID_ARGUMENT("Tried to construct entry from null player.");

	for (auto &shot : shot_track.shots)
		if (shot.record && shot.record->index == player->index())
		{
			shot.record = nullptr;
			shot.manual = true;
		}

	this->player = player;
	spawntime = player->get_spawn_time();
	handle = player->get_handle();
	bones = bone_changes(player);

	visuals.previous_health = std::clamp(player->get_health(), 0, 100) / 100.f;
	visuals.previous_armor = std::clamp(player->get_armor(), 0, 100) / 100.f;
	visuals.previous_ammo = 1.f;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	if (wpn && wpn->get_clip1() > 0)
	{
		const auto wpn_info = wpn->get_cs_weapon_data();
		if (wpn_info && wpn_info->maxclip1 > 0)
			visuals.previous_ammo =
				std::clamp(wpn->get_clip1(), 0, wpn_info->maxclip1) / static_cast<float>(wpn_info->maxclip1);
	}

	for (auto i = 0; i < 2; i++)
		for (auto j = 0; j < resolver_state_max; j++)
		{
			resolver.states[i][j].eliminated_positions.set(resolver_max_max);
			resolver.states[i][j].eliminated_positions.set(resolver_max_extra);
			resolver.states[i][j].eliminated_positions.set(resolver_min_min);
			resolver.states[i][j].eliminated_positions.set(resolver_min_extra);
		}
}

lag_record *i_player_list::player_entry::get_record(float target_time, bool visual)
{
	const auto latency = max(0, TIME_TO_TICKS(game->client_state->net_channel->get_latency(flow_outgoing)));
	const auto dead_time =
		static_cast<int32_t>(game->engine_client->get_last_timestamp() + TICKS_TO_TIME(latency) - sv_max_unlag);

	lag_record *record = nullptr, *previous = nullptr;
	for (auto &rec : records)
		if (rec.valid && (!visual || !rec.extrapolated))
		{
			record = &rec;
			break;
		}
	if (!record)
		return nullptr;

	auto origin = record->origin;
	for (auto &rec : records)
	{
		if (!rec.valid || (visual && rec.extrapolated))
			continue;

		if (rec.sim_time < static_cast<float>(dead_time))
			break;

		previous = record;
		record = &rec;

		const auto delta = record->origin - origin;
		if (delta.length2d_sqr() > 64 * 64)
			return nullptr;

		if (record->sim_time <= target_time)
			break;

		origin = record->origin;
	}

	if (!previous)
		return nullptr;

	if (scan_records.exhausted())
		scan_records.pop_back();

	if (scan_records.exhausted())
		return nullptr;

	auto target_record = scan_records.push_front();
	*target_record = *record;

	if (record->sim_time < target_time && record->sim_time < previous->sim_time)
	{
		const auto frac = (target_time - record->sim_time) / (previous->sim_time - record->sim_time);
		for (auto i = 0; i < resolver_direction_max; i++)
			target_record->abs_ang[i].y = interpolate(record->abs_ang[i].y, previous->abs_ang[i].y, frac);
		target_record->origin = interpolate(record->origin, previous->origin, frac);
		target_record->obb_mins = interpolate(record->obb_mins, previous->obb_mins, frac);
		target_record->obb_maxs = interpolate(record->obb_maxs, previous->obb_maxs, frac);

		if (visual && record->has_visual_matrix && previous->has_visual_matrix)
		{
			const auto lerp_org = interpolate(record->abs_origin, previous->abs_origin, frac);
			const auto lerp_ang_diff = angle_diff(previous->abs_angle.y, record->abs_angle.y);
			const auto lerp_ang_diff_interpolated = interpolate(0.f, lerp_ang_diff, frac);
			const auto lerp_ang =
				angle(0.f, std::remainderf(record->abs_angle.y + lerp_ang_diff_interpolated, yaw_bounds), 0.f);

			const auto inv = matrix_invert(angle_matrix(record->abs_angle, record->abs_origin));
			const auto transform = concat_transforms(angle_matrix(lerp_ang, lerp_org), inv);
			const auto bones =
				min(player->get_studio_hdr() ? player->get_studio_hdr()->numbones() : max_bones, max_bones);
			for (auto i = 0; i < bones; i++)
				target_record->vis_mat[i] = concat_transforms(transform, record->vis_mat[i]);
			target_record->abs_origin = lerp_org;
			target_record->has_visual_matrix = true;
		}
	}
	else if (visual && record->has_visual_matrix)
	{
		target_record->has_visual_matrix = true;
		memcpy(target_record->vis_mat, record->vis_mat, sizeof(target_record->vis_mat));
	}

	if (!visual && target_record->obb_maxs != player->get_collideable()->obb_maxs())
	{
		target_record->override_bounds_change_time = game->globals->curtime;
		target_record->override_view_height = player->get_collideable()->obb_maxs().z + records.front().origin.z;
	}

	target_record->tick_count = TIME_TO_TICKS(target_time);
	return target_record;
}

void i_player_list::player_entry::get_boundary_times(float &first, float &second, bool visual)
{
	if (records.empty())
	{
		first = 0.f;
		second = 0.f;
		return;
	}

	auto time = game->globals->curtime;
	if (visual)
		time += 2.f * calculate_lerp() - TICKS_TO_TIME(hacks::tickbase.compute_current_limit() - 1);

	const auto lerp = calculate_lerp();
	const auto correct = std::clamp(calculate_rtt() + lerp, 0.f, sv_max_unlag);

	const auto max_unlag = max(0.f, sv_max_unlag - game->globals->interval_per_tick);
	const auto target_first = time - correct + lerp + max_unlag;
	const auto target_second = time - correct + lerp - max_unlag;

	const auto first_tick = static_cast<int32_t>(floorf(target_first / game->globals->interval_per_tick));
	const auto second_tick = static_cast<int32_t>(ceilf(target_second / game->globals->interval_per_tick));

	auto first_record_time = 0.f;
	std::optional<vec3> origin;
	auto broke_lc = false;
	for (auto &rec : records)
	{
		if (!game->cl_lagcompensation || !game->cl_predict)
		{
			broke_lc = true;
			break;
		}

		if (!rec.valid || rec.extrapolated)
			continue;

		if (!origin.has_value())
			origin = rec.origin;

		const auto delta = rec.origin - *origin;
		if (delta.length2d_sqr() > 64 * 64)
		{
			broke_lc = true;
			first_record_time = 0.f;
			break;
		}

		if (first_record_time != 0.f)
			break;

		first_record_time = rec.sim_time;
		origin = rec.origin;
	}

	if (broke_lc)
	{
		first = records.front().sim_time;
		second = 0.f;
	}
	else
	{
		if (!visual)
		{
			first = min(first_record_time + lerp, TICKS_TO_TIME(first_tick));
			second = TICKS_TO_TIME(second_tick);
		}
		else
		{
			first = min(first_record_time + lerp, target_first);
			second = target_second;
		}

		if (first < second)
		{
			first = game->globals->curtime - correct;
			second = -1.f;
		}
	}
}

void i_player_list::player_entry::run_extrapolation(bool full_setup)
{
	if (records.empty())
		return;

	auto previous = &records.front();
	auto first_lagcomp = previous;
	if (!previous->valid || previous->extrapolated)
		for (auto &rec : records)
		{
			if (!rec.valid || rec.extrapolated)
				continue;

			first_lagcomp = &rec;
			break;
		}

	const auto lag = max(1, first_lagcomp->real_lag);
	const auto latency = max(0, TIME_TO_TICKS(game->client_state->net_channel->get_latency(flow_outgoing)));
	const auto possible_future_tick = TIME_TO_TICKS(game->engine_client->get_last_timestamp()) + 1 + latency + 8;
	const auto behind =
		std::clamp(TIME_TO_TICKS(game->engine_client->get_last_timestamp()) - first_lagcomp->server_tick, 0,
				   sv_maxusrcmdprocessticks);

	const auto first_lagcomp_sim_tick = TIME_TO_TICKS(first_lagcomp->sim_time);
	const auto target_tick = first_lagcomp_sim_tick + behind + latency;

	for (auto i = first_lagcomp_sim_tick + lag; i <= target_tick && i <= possible_future_tick; i += lag)
	{
		const auto delta_to_first = i - TIME_TO_TICKS(previous->sim_time);
		if (delta_to_first <= 0)
			continue;

		const auto ticks_remaining = min(lag, delta_to_first);

		if (records.exhausted())
			records.pop_back();

		const auto rec = records.push_front();
		*rec = *previous;
		rec->valid = true;
		rec->sim_time = TICKS_TO_TIME(i);
		rec->lag = ticks_remaining;
		rec->real_lag = first_lagcomp->real_lag;
		rec->extrapolated = true;

		if (rec->shot)
		{
			if (records.size() > 2 && !records[2].shot)
				rec->eye_ang = records[2].eye_ang;
			rec->shot = false;
		}

		player_list.perform_animations(rec, previous, full_setup);

		previous = rec;
	}
}

void i_player_list::player_entry::fill_current_real_lag()
{
	if (records.size() < 1)
		return;

	auto &current = records.front();
	if (records.size() < 2)
	{
		current.real_lag = current.lag;
		return;
	}

	auto use_server_tick = false;
	for (auto it = ++records.begin(); it != records.end(); ++it)
	{
		if (!it->valid)
		{
			use_server_tick = true;
			continue;
		}

		if (use_server_tick)
		{
			current.real_lag = current.server_tick - it->server_tick;
			return;
		}

		break;
	}

	current.real_lag = current.lag;
}

bool i_player_list::player_entry::is_visually_fakeducking()
{
	if (records.size() < 2)
		return false;

	const auto &first = records[0];
	const auto &second = records[1];

	return (first.lag >= 10 || second.lag >= 10) && fabsf(first.duck - second.duck) < .25f && first.duck > 0.f &&
		   second.duck < .85f;
}

bool i_player_list::player_entry::is_charged()
{
	return !records.empty() && records.front().delta_time > TICKS_TO_TIME(7) && records.front().lag < 7;
}

bool i_player_list::player_entry::is_exploiting()
{
	return !records.empty() && records.front().sim_time < compensation_time;
}

bool i_player_list::player_entry::is_peeking()
{
	if (records.empty())
		return false;

	const auto first_velocity = records.front().velocity.length();

	if (first_velocity >= 50.f)
		return true;

	if (records.size() < 2)
		return false;

	return fabsf(first_velocity - records[1].velocity.length()) >= 10.f;
}

lag_record::lag_record(cs_player_t *const player)
{
	this->player = player;
	index = player->index();
	dormant = player->is_dormant();
	origin = player->get_origin();
	abs_origin = player->get_abs_origin();
	obb_mins = player->get_collideable()->obb_mins();
	obb_maxs = player->get_collideable()->obb_maxs();
	layers = *player->get_animation_layers();
	has_state = player->get_anim_state();
	sim_time = player->get_simulation_time();
	recv_time = game->globals->curtime;
	delta_time = game->engine_client->get_last_timestamp() - sim_time;
	server_tick = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
	strafing = player->get_strafing();
	duck = player->get_duck_amount();
	lower_body_yaw_target = player->get_lower_body_yaw_target();
	eye_ang = player->get_eye_angles();
	flags = player->get_flags();
	eflags = player->get_eflags();
	effects = player->get_effects();
	move_state = player->get_move_state();
	lag = TIME_TO_TICKS(player->get_simulation_time() - player->get_old_simulation_time());
	velocity_modifier = player->get_velocity_modifier();
	stamina = player->get_stamina();

	// initially populate resolver data.
	for (auto i = 0; i < resolver_direction_max; i++)
	{
		poses[i] = player->get_pose_parameter();
		abs_ang[i] = player->get_abs_angle();

		if (has_state)
			state[i] = *player->get_anim_state();
	}

	velocity = (player->get_origin() - *player->get_old_origin()) * (1.f / TICKS_TO_TIME(lag));
	if (flags & cs_player_t::on_ground)
		velocity.z = 0.f;

	if (!velocity.is_valid())
		velocity = {0.f, 0.f, 0.f};
}

void lag_record::determine_simulation_ticks(lag_record *const last)
{
	auto &player_entry = GET_PLAYER_ENTRY(player);

	if (!valid || !last)
		return;

	const auto &start = last->layers[11];
	const auto &end = layers[11];
	auto cycle = start.cycle;
	auto playback_rate = start.playback_rate;
	auto reached = false;

	for (auto i = 1; i <= TIME_TO_TICKS(1.f); i++)
	{
		const auto cycle_delta = playback_rate * TICKS_TO_TIME(1);
		cycle += cycle_delta;

		if (cycle >= 1.f)
			playback_rate = end.playback_rate;

		cycle -= (float)(int)cycle;

		if (cycle < 0.f)
			cycle += 1.f;

		if (cycle > 1.f)
			cycle -= 1.f;

		if (fabsf(cycle - end.cycle) <= .5f * cycle_delta)
		{
			reached = true;

			if ((TIME_TO_TICKS(sim_time) < TIME_TO_TICKS(last->sim_time) ||
				 (player_entry.addt && TIME_TO_TICKS(sim_time) == TIME_TO_TICKS(last->sim_time))) &&
				!last->dormant)
				lag = i + 1;
			else
				lag = i;
		}
	}

	if (!reached)
		lag = 1;

	if (!player_entry.addt && TIME_TO_TICKS(sim_time) == TIME_TO_TICKS(last->sim_time))
	{
		if (player_entry.ground_accel_linear_frac_last_time_stamp == game->client_state->command_ack &&
			fabsf(player_entry.ground_accel_linear_frac_last_time - sim_time - TICKS_TO_TIME(lag)) < 1.f)
			sim_time = player_entry.ground_accel_linear_frac_last_time;
		else
			sim_time += TICKS_TO_TIME(lag);
	}

	velocity = (player->get_origin() - *player->get_old_origin()) * (1.f / TICKS_TO_TIME(lag));
	if (flags & cs_player_t::on_ground || last->flags & cs_player_t::on_ground)
		velocity.z = 0.f;

	if (!velocity.is_valid())
		velocity = {0.f, 0.f, 0.f};
}

void lag_record::build_matrix(const resolver_direction direction)
{
	const auto hdr = player->get_studio_hdr();
	const auto layers = player->get_animation_layers();
	const auto non_local = !player->get_predictable();

	if (!hdr || !layers || layers->size() < 13)
		return;

	// keep track of old values.
	game->mdl_cache->begin_lock();
	const auto backup_effects = player->get_effects();
	const auto backup_flags = player->get_lod_data().flags;
	const auto backup_ik = player->get_ik();
	const auto backup_layer = layers->at(12).weight;
	const auto backup_eye = *player->eye_angles();
	const auto backup_entire_body = player->get_snapshot_entire_body();
	const auto backup_upper_body = player->get_snapshot_upper_body();

	// stop interpolation, snapshots and other shit.
	player->get_eflags() |= efl_setting_up_bones;
	player->get_effects() |= cs_player_t::ef_nointerp;
	player->get_lod_data().flags = lod_empty;
	player->get_snapshot_entire_body().player = nullptr;
	player->get_snapshot_upper_body().player = nullptr;
	if (this != &local_fake_record)
		layers->at(12).weight = 0.f;

	// setup temporary storage for inverse kinematics.
	ik_context ik;
	uint8_t computed[0x100]{};
	auto angle = abs_ang[direction];
	auto &out = res_mat[direction];
	ik.init(hdr, &angle, &origin, sim_time, game->globals->framecount,
			XOR_32(bone_used_by_hitbox | bone_used_by_attachment));

	// perform bone blending.
	alignas(16) vec3 pos[max_bones] = {};
	alignas(16) quaternion_aligned q[max_bones] = {};
	player->get_ik() = &ik;
	const auto mask = XOR_32(bone_used_by_anything);
	player->standard_blending_rules(hdr, pos, q, sim_time, mask);
	ik.update_targets(pos, q, out, computed);
	ik.solve_dependencies(pos, q, out, computed);

	// build chain.
	int32_t chain[max_bones] = {};
	const auto chain_length = hdr->numbones();
	for (auto i = 0; i < chain_length; i++)
		chain[chain_length - i - 1] = i;

	// build transformations.
	const auto rotation = angle_matrix(angle, origin);
	for (auto j = chain_length - 1; j >= 0; j--)
	{
		const auto i = chain[j];
		const auto parent = hdr->bone_parent.count() > i ? &hdr->bone_parent[i] : nullptr;

		if (!parent)
			continue;

		const auto qua = quaternion_matrix(q[i], pos[i]);

		if (*parent == -1)
			out[i] = concat_transforms(rotation, qua);
		else
			out[i] = concat_transforms(out[*parent], qua);
	}

	auto networked_eye = *player->eye_angles();
	networked_eye.x = std::clamp(std::remainderf(networked_eye.x, yaw_bounds), -pitch_bounds, pitch_bounds);
	auto last_eye = GET_PLAYER_ENTRY(player).previous_angle;

	if (non_local)
	{
		// TODO: consider handling jittering pitch here.

		has_previous_matrix =
			fabsf(std::remainderf(last_eye.y - std::remainderf(networked_eye.y, yaw_bounds), yaw_bounds)) > 2.f;

		if (override_bounds_change_time != 0.f)
		{
			player->get_bounds_change_time() = override_bounds_change_time;
			player->get_view_height() = override_view_height;
		}
	}

	memcpy(unprocessed_mat, out, sizeof(out));

	if (direction == resolver_max)
	{
		player->eye_angles()->z = get_resolver_roll(resolver_max_max);
		memcpy(res_mat[resolver_max_max], out, sizeof(out));
		player->post_build_transformations(res_mat[resolver_max_max], mask);
		player->eye_angles()->z = get_resolver_roll(resolver_max_extra);
		memcpy(res_mat[resolver_max_extra], out, sizeof(out));
		player->post_build_transformations(res_mat[resolver_max_extra], mask);

		if (has_previous_matrix)
		{
			*player->eye_angles() = last_eye;
			player->eye_angles()->z = get_resolver_roll(resolver_max_max);
			memcpy(previous_res_mat[resolver_max_max], out, sizeof(out));
			player->post_build_transformations(previous_res_mat[resolver_max_max], mask);
			player->eye_angles()->z = get_resolver_roll(resolver_max_extra);
			memcpy(previous_res_mat[resolver_max_extra], out, sizeof(out));
			player->post_build_transformations(previous_res_mat[resolver_max_extra], mask);
		}
	}
	else if (direction == resolver_min)
	{
		player->eye_angles()->z = get_resolver_roll(resolver_min_min);
		memcpy(res_mat[resolver_min_min], out, sizeof(out));
		player->post_build_transformations(res_mat[resolver_min_min], mask);
		player->eye_angles()->z = get_resolver_roll(resolver_min_extra);
		memcpy(res_mat[resolver_min_extra], out, sizeof(out));
		player->post_build_transformations(res_mat[resolver_min_extra], mask);

		if (has_previous_matrix)
		{
			*player->eye_angles() = last_eye;
			player->eye_angles()->z = get_resolver_roll(resolver_min_min);
			memcpy(previous_res_mat[resolver_min_min], out, sizeof(out));
			player->post_build_transformations(previous_res_mat[resolver_min_min], mask);
			player->eye_angles()->z = get_resolver_roll(resolver_min_extra);
			memcpy(previous_res_mat[resolver_min_extra], out, sizeof(out));
			player->post_build_transformations(previous_res_mat[resolver_min_extra], mask);
		}
	}

	if (has_previous_matrix)
	{
		*player->eye_angles() = last_eye;
		player->eye_angles()->z = get_resolver_roll(direction);
		memcpy(previous_res_mat[direction], out, sizeof(out));
		player->post_build_transformations(previous_res_mat[direction], mask);
	}

	if (non_local)
	{
		*player->eye_angles() = networked_eye;
		player->eye_angles()->z = get_resolver_roll(direction);
	}

	player->post_build_transformations(out, mask);

	// restore original values.
	player->get_eflags() &= ~efl_setting_up_bones;
	player->get_effects() = backup_effects;
	player->get_lod_data().flags = backup_flags;
	player->get_ik() = backup_ik;
	layers->at(12).weight = backup_layer;
	*player->eye_angles() = backup_eye;
	player->get_snapshot_entire_body() = backup_entire_body;
	player->get_snapshot_upper_body() = backup_upper_body;
	game->mdl_cache->end_lock();
}

void lag_record::perform_matrix_setup()
{
	game->mdl_cache->begin_lock();
	const auto is_local = player->index() == game->engine_client->get_local_player();
	const auto is_fake = player->is_fake_player() || !player->is_enemy() || is_local || !true;
	if (is_fake)
		res_direction = resolver_networked;

	const auto backup_layers = *player->get_animation_layers();
	const auto backup_anim_state = *player->get_anim_state();
	const auto backup_poses = player->get_pose_parameter();
	const auto backup_abs_origin = player->get_abs_origin();
	const auto backup_abs_angle = player->get_abs_angle();
	const auto backup_bones = bone_changes(player);

	player->get_collideable()->obb_maxs() = obb_maxs;

	for (auto i = 0; i < resolver_direction_max; i++)
	{
		if (is_fake && i != resolver_networked)
			continue;

		if (i == resolver_max_max || i == resolver_min_min || i == resolver_min_extra || i == resolver_max_extra)
			continue;

		// restore animation data.
		*player->get_animation_layers() = extrapolated ? custom_layers[i] : layers;
		player->get_pose_parameter() = poses[i];
		player->set_abs_origin(origin);
		player->set_abs_angle(abs_ang[i]);

		// perform bone setup.
		build_matrix(static_cast<resolver_direction>(i));
	}

	// restore everything.
	*player->get_animation_layers() = backup_layers;
	*player->get_anim_state() = backup_anim_state;
	player->get_pose_parameter() = backup_poses;
	player->set_abs_origin(backup_abs_origin);
	player->set_abs_angle(backup_abs_angle);
	backup_bones.restore(player);

	// write out final matrix.
	memcpy(mat, res_mat[res_direction], sizeof(mat));

	// cache off data.
	has_matrix = true;
	game->mdl_cache->end_lock();
}

bool lag_record::is_breaking_lagcomp() const
{
	if (!game->cl_predict || !game->cl_lagcompensation)
		return true;

	auto &entry = GET_PLAYER_ENTRY(player);

	if (entry.records.size() < 2 || dormant)
		return false;

	lag_record *latest = nullptr;
	for (auto &rec : entry.records)
		if (rec.valid && !rec.extrapolated)
		{
			latest = &rec;
			break;
		}

	if (!latest || latest->dormant)
		return true;

	auto prev_org = latest->origin;
	auto skip_first = true;

	// walk context looking for any invalidating event.
	for (auto &cmp : entry.records)
	{
		if (!cmp.valid || cmp.extrapolated)
			continue;

		if (skip_first)
		{
			skip_first = false;
			continue;
		}

		if (cmp.dormant)
			break;

		auto delta = cmp.origin - prev_org;
		if (delta.length2d_sqr() > teleport_dist)
			return true;

		if (cmp.sim_time <= sim_time)
			break;

		prev_org = cmp.origin;
	}

	return false;
}

void lag_record::update_animations(user_cmd *const cmd)
{
	game->mdl_cache->begin_lock();
	const auto local = player->index() == game->engine_client->get_local_player();

	// make a backup of globals.
	const auto backup_frametime = game->globals->frametime;
	const auto backup_curtime = game->globals->curtime;

	// fix animating twice per frame.
	if (player->get_anim_state()->last_framecount >= game->globals->framecount)
		player->get_anim_state()->last_framecount = game->globals->framecount - 1;

	// fixes for networked players.
	if (!local && !ent_ren.is_updating_animstate)
	{
		game->globals->frametime = game->globals->interval_per_tick;
		game->globals->curtime = sim_time;

		if (player->get_anim_state()->last_update == game->globals->curtime)
			player->get_anim_state()->last_update = game->globals->curtime + game->globals->interval_per_tick;
	}

	// update animations.
	play_additional_animations(cmd, predict_animation_state());
	const auto angles = *player->eye_angles();
	const auto state = player->get_anim_state();
	state->update(angles.y, angles.x);

	if (cmd)
	{
		// update lower body yaw target, simtime and prevent adjust from blending too far into the future.
		if (state->speed > .1f || state->speed_z > 100.f)
		{
			player->get_lower_body_yaw_target() = state->eye_yaw;
			addon.next_lby_update = game->globals->curtime + .22f;
		}
		else if (game->globals->curtime > addon.next_lby_update &&
				 fabsf(angle_diff(state->foot_yaw, state->eye_yaw)) > 35.f)
		{
			player->get_lower_body_yaw_target() = state->eye_yaw;
			addon.next_lby_update = game->globals->curtime + 1.1f;
		}

		player->get_animation_layers()->at(3).weight = addon.adjust_weight;
	}

	if (local)
		sim_time = game->globals->curtime;

	// restore globals.
	game->globals->frametime = backup_frametime;
	game->globals->curtime = backup_curtime;
	game->mdl_cache->end_lock();
}

anim_state lag_record::predict_animation_state(user_cmd *cmd)
{
	game->mdl_cache->begin_lock();
	const auto backup_state = *player->get_anim_state();
	const auto backup_layers = *player->get_animation_layers();
	const auto backup_poses = player->get_pose_parameter();

	// fix animating twice per frame.
	if (player->get_anim_state()->last_framecount >= game->globals->framecount)
		player->get_anim_state()->last_framecount = game->globals->framecount - 1;

	// update animations and write out final state.
	const auto angles = *player->eye_angles();
	player->get_anim_state()->update(angles.y, angles.x);
	auto pred = *player->get_anim_state();
	if (cmd)
		play_additional_animations(cmd, predict_animation_state());

	*player->get_anim_state() = backup_state;
	*player->get_animation_layers() = backup_layers;
	player->get_pose_parameter() = backup_poses;
	game->mdl_cache->end_lock();
	return pred;
}

// note to myself: there are still a few minor bugs in this implementation, some situations where you move very slowly
// and ducking. none of these situations have any influence on the weapon_t shoot position, so we are going to clean up
// later (TM).
void lag_record::play_additional_animations(sdk::user_cmd *const cmd, const sdk::anim_state &pred_state)
{
	if (!player->get_anim_state() || !player->get_anim_state()->player || !player->get_studio_hdr() || !cmd)
		return;

	static const auto recrouch_generic = XOR_STR_STORE("recrouch_generic");
	static const auto crouch = XOR_STR_STORE("crouch");

	const auto s = player->get_anim_state();
	const auto p = &pred_state;
	const auto update_time = fmaxf(0.f, p->last_update - s->last_update);
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	const auto world_model =
		wpn ? reinterpret_cast<entity_t *const>(
				  game->client_entity_list->get_client_entity_from_handle(wpn->get_weapon_world_model()))
			: nullptr;
	const auto hdr = player->get_studio_hdr();
	const auto seed = cmd->command_number;

	if (wpn && wpn->get_weapon_type() == weapontype_knife &&
		wpn->get_next_primary_attack() + .4f < game->globals->curtime)
		addon.swing_left = true;

	player_list.build_activity_modifiers(player, addon.activity_modifiers);

	// CCSGOPlayerAnimState::DoAnimationEvent
	if ((cmd->buttons & user_cmd::jump) && !(player->get_flags() & cs_player_t::on_ground) && s->on_ground)
	{
		player->try_initiate_animation(4, XOR_32(985), addon.activity_modifiers, seed);
		addon.in_jump = true;
	}

	auto &action = player->get_animation_layers()->at(1);
	switch (addon.vm)
	{
	case 219:
	case 190:
	case 192:
	case 477:
		if (wpn && wpn->get_item_definition_index() != item_definition_index::weapon_c4)
		{
			if (addon.vm == 219)
			{
				std::array<uint16_t, 3> tmp;
				player_list.build_activity_modifiers(player, tmp, true);
				player->try_initiate_animation(1, XOR_32(961), tmp, seed);
			}
			else
				player->try_initiate_animation(1, XOR_32(961), addon.activity_modifiers, seed);
		}
		break;
	// ACT_VM_HITCENTER, ACT_VM_MISSCENTER
	case 200:
	case 206:
		player->try_initiate_animation(1, !addon.swing_left ? XOR_32(961) : XOR_32(962), addon.activity_modifiers,
									   seed);
		addon.swing_left = !addon.swing_left;
		break;
	// ACT_VM_SWINGHIT
	case 211:
		player->try_initiate_animation(1, XOR_32(963), addon.activity_modifiers, seed);
		addon.swing_left = !addon.swing_left;
		break;
	// ACT_VM_SECONDARYATTACK
	case 193:
		player->try_initiate_animation(1, XOR_32(964), addon.activity_modifiers, seed);
		break;
	// ACT_VM_HITCENTER2, ACT_VM_MISSCENTER2
	case 201:
	case 207:
		player->try_initiate_animation(1, XOR_32(964), addon.activity_modifiers, seed);
		addon.swing_left = true;
		break;
	// ACT_VM_SWINGHARD
	case 209:
		player->try_initiate_animation(1, XOR_32(965), addon.activity_modifiers, seed);
		addon.swing_left = true;
		break;
	// ACT_VM_RELOAD, ACT_SECONDARY_VM_RELOAD
	case 194:
	case 831:
		if (wpn && wpn->get_weapon_type() == weapontype_shotgun &&
			wpn->get_item_definition_index() != item_definition_index::weapon_mag7)
			player->try_initiate_animation(1, XOR_32(969), addon.activity_modifiers, seed);
		else
			player->try_initiate_animation(1, XOR_32(967), addon.activity_modifiers, seed);
		break;
	// ACT_SHOTGUN_RELOAD_START
	case 264:
		player->try_initiate_animation(1, XOR_32(968), addon.activity_modifiers, seed);
		break;
	// ACT_SHOTGUN_RELOAD_FINISH
	case 265:
		player->try_initiate_animation(1, XOR_32(970), addon.activity_modifiers, seed);
		break;
	// ACT_VM_PULLPIN
	case 191:
		player->try_initiate_animation(1, XOR_32(971), addon.activity_modifiers, seed);
		break;
	// ACT_VM_DRAW, ACT_VM_EMPTY_DRAW, ACT_VM_DRAW_SILENCED
	case 183:
	case 224:
	case 481:
		if (wpn && player->get_sequence_activity(action.sequence) == XOR_32(972) && action.cycle < .15f)
			addon.in_rate_limit = true;
		else
			player->try_initiate_animation(1, XOR_32(972), addon.activity_modifiers, seed);
		break;
	}

	// CCSGOPlayerAnimState::SetupVelocity
	auto &layer3 = player->get_animation_layers()->at(3);

	if (!s->adjust_started && p->speed <= 0.f && s->standing_time <= 0.f && s->on_ground && !s->on_ladder &&
		!s->in_hit_ground && s->stutter_step < 50.f)
	{
		player->try_initiate_animation(3, XOR_32(980), addon.activity_modifiers, seed);
		s->adjust_started = true;
	}

	const auto layer3_act = player->get_sequence_activity(layer3.sequence);
	if (layer3_act == XOR_32(980) || layer3_act == XOR_32(979))
	{
		if (s->adjust_started && p->ducking_speed <= .25f)
		{
			layer3.increment_layer_cycle_weight_rate_generic(update_time, hdr);
			s->adjust_started = !layer3.has_sequence_completed(update_time);
		}
		else
		{
			const auto weight = layer3.weight;
			layer3.weight = approach(0.f, weight, update_time * 5.f);
			layer3.set_layer_weight_rate(update_time, weight);
			s->adjust_started = false;
		}
	}

	if (p->speed <= 1.f && s->on_ground && !s->on_ladder && !s->in_hit_ground &&
		fabsf(angle_diff(s->foot_yaw, p->foot_yaw)) / update_time > 120.f)
	{
		player->try_initiate_animation(3, XOR_32(979), addon.activity_modifiers, seed);
		s->adjust_started = true;
	}

	addon.adjust_weight = layer3.weight;
	layer3.weight = 0.f;

	// CCSGOPlayerAnimState::SetUpWeaponAction
	auto increment = true;
	if (wpn && addon.in_rate_limit && player->get_sequence_activity(action.sequence) == XOR_32(972))
	{
		if (world_model)
			world_model->get_effects() |= ef_nodraw;

		if (action.cycle >= .15f)
		{
			addon.in_rate_limit = false;
			player->try_initiate_animation(1, XOR_32(972), addon.activity_modifiers, seed);
			increment = false;
		}
	}

	auto &recrouch = player->get_animation_layers()->at(2);
	auto recrouch_weight = 0.f;
	if (action.weight > 0.f)
	{
		if (recrouch.sequence <= 0)
		{
			XOR_STR_STACK(r, recrouch_generic);

			const auto seq = player->lookup_sequence(r);
			recrouch.playback_rate = player->get_layer_sequence_cycle_rate(&recrouch, seq);
			recrouch.sequence = seq;
			recrouch.cycle = recrouch.weight = 0.f;
		}

		auto has_modifier = false;
		XOR_STR_STACK(c, crouch);
		const auto &seqdesc = player->get_studio_hdr()->get_seq_desc(action.sequence);
		for (auto i = 0; i < seqdesc->numactivitymodifiers; i++)
			if (!strcmp(seqdesc->get_activity_modifier(i)->get_name(), c))
			{
				has_modifier = true;
				break;
			}

		if (has_modifier)
		{
			if (p->duck_amount < 1.f)
				recrouch_weight = action.weight * (1.f - p->duck_amount);
		}
		else if (p->duck_amount > 0.f)
			recrouch_weight = action.weight * p->duck_amount;
	}
	else if (recrouch.weight > 0.f)
		recrouch_weight = approach(0.f, recrouch.weight, 4.f * update_time);

	recrouch.weight = std::clamp(recrouch_weight, 0.f, 1.f);

	if (increment)
	{
		action.increment_layer_cycle(update_time, false);
		const auto prev_weight = action.weight;
		action.set_layer_ideal_weight_from_sequence_cycle(hdr);
		action.set_layer_weight_rate(p->last_increment, action.weight);
		action.cycle = std::clamp(action.cycle - p->last_increment * action.playback_rate, 0.f, 1.f);
		action.weight = std::clamp(action.weight - p->last_increment * action.weight_delta_rate, .0000001f, 1.f);
	}

	// CCSGOPlayerAnimState::SetupMovement
	if (player->get_predictable())
	{
		vec3 forward, right, up;
		angle_vectors(angle(0.f, p->foot_yaw, 0.f), forward, right, up);
		right.normalize();
		const auto to_forward_dot = p->last_accel_speed.dot(forward);
		const auto to_right_dot = p->last_accel_speed.dot(right);
		const auto move_right = (cmd->buttons & user_cmd::move_right) != 0;
		const auto move_left = (cmd->buttons & user_cmd::move_left) != 0;
		const auto move_forward = (cmd->buttons & user_cmd::forward) != 0;
		const auto move_backwards = (cmd->buttons & user_cmd::back) != 0;
		const auto strafe_forward =
			p->running_speed >= .65f && move_forward && !move_backwards && to_forward_dot < -.55f;
		const auto strafe_backwards =
			p->running_speed >= .65f && move_backwards && !move_forward && to_forward_dot > .55f;
		const auto strafe_right = p->running_speed >= .73f && move_right && !move_left && to_right_dot < -.63f;
		const auto strafe_left = p->running_speed >= .73f && move_left && !move_right && to_right_dot > .63f;
		player->get_strafing() = strafe_forward || strafe_backwards || strafe_right || strafe_left;
	}

	const auto swapped_ground = s->on_ground != p->on_ground || s->on_ladder != p->on_ladder;

	if (!s->on_ladder && p->on_ladder)
		player->try_initiate_animation(5, XOR_32(987), addon.activity_modifiers, seed);

	if (p->on_ground)
	{
		auto &layer5 = player->get_animation_layers()->at(5);

		if (!s->in_hit_ground && swapped_ground)
			player->try_initiate_animation(5, s->air_time > 1.f ? XOR_32(989) : XOR_32(988), addon.activity_modifiers,
										   seed);

		if (p->in_hit_ground && player->get_sequence_activity(layer5.sequence) != XOR_32(987))
			addon.in_jump = false;

		if (!p->in_hit_ground && !addon.in_jump && p->ladder_speed <= 0.f)
			layer5.weight = 0.f;
	}
	else if (swapped_ground && !addon.in_jump)
		player->try_initiate_animation(4, XOR_32(986), addon.activity_modifiers, seed);

	// CCSGOPlayerAnimState::SetupAliveLoop
	auto &alive = player->get_animation_layers()->at(11);
	if (player->get_sequence_activity(alive.sequence) == XOR_32(981))
	{
		if (p->weapon && p->weapon != p->last_weapon)
		{
			const auto cycle = alive.cycle;
			player->try_initiate_animation(11, XOR_32(981), addon.activity_modifiers, seed);
			alive.cycle = cycle;
		}
		else if (!alive.has_sequence_completed(update_time))
			alive.weight = 1.f - std::clamp((p->walk_speed - .55f) / .35f, 0.f, 1.f);
	}

	addon.vm = 0;
}

float lag_record::get_resolver_roll(std::optional<resolver_direction> direction_override)
{
	const auto state = player->get_anim_state();

	switch (direction_override.value_or(res_direction))
	{
	case resolver_max_max:
	case resolver_min_extra:
		return -signum(state->aim_yaw_max) * roll_bounds;
	case resolver_min_min:
	case resolver_max_extra:
		return -signum(state->aim_yaw_min) * roll_bounds;
	default:
		break;
	}

	return 0.f;
}
} // namespace detail
