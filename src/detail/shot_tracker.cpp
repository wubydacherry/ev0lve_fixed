
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/event_log.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/custom_tracing.h>
#include <detail/shot_tracker.h>
#include <sdk/client_entity_list.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_trace.h>

#ifdef CSGO_LUA
#include "player_list.h"
#include <lua/engine.h>

#endif

using namespace sdk;
using namespace hacks;
using namespace detail::aim_helper;

namespace detail
{
shot_tracker shot_track{};

void shot_tracker::register_shot(shot &&s) { shots.emplace_front(std::move(s)); }

int32_t shot_tracker::calculate_health_correction(cs_player_t *const player) const
{
	auto damage = 0.f;
	for (const auto &shot : shots)
		if (!shot.manual && shot.target->record->player == player)
			damage += shot.damage;

	return damage < player->get_health() ? damage : 0.f;
}

void shot_tracker::on_player_hurt(game_event *event)
{
	const auto attacker = event->get_int(XOR_STR("attacker"));
	const auto attacker_index = game->engine_client->get_player_for_user_id(attacker);

	if (attacker_index != game->engine_client->get_local_player())
		return;

	if (shots.empty())
		return;

	shot *last_confirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (it->confirmed && !it->skip)
		{
			last_confirmed = &*it;
			break;
		}
	}

	if (!last_confirmed)
		return;

	const auto userid = event->get_int(XOR_STR("userid"));
	const auto index = game->engine_client->get_player_for_user_id(userid);

	// perform reordering if fps dropped too low.
	if (!last_confirmed->manual && index != last_confirmed->record->index)
		for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
		{
			if (!it->manual && index == it->record->index)
			{
				it->impacted = last_confirmed->impacted;
				it->confirmed = last_confirmed->confirmed;
				it->skip = last_confirmed->skip;
				it->server_info.impacts = last_confirmed->server_info.impacts;
				last_confirmed->confirmed = last_confirmed->impacted = false;
				last_confirmed = &*it;
				break;
			}
		}

	// abort if still not found.
	if (!last_confirmed->manual && index != last_confirmed->record->index)
	{
		last_confirmed->server_info.index = -1;
		return;
	}

	last_confirmed->server_info.index = index;
	last_confirmed->server_info.damage = event->get_int(XOR_STR("dmg_health"));
	last_confirmed->server_info.health = event->get_int(XOR_STR("health"));
	last_confirmed->server_info.hitgroup = event->get_int(XOR_STR("hitgroup"));
}

void shot_tracker::on_bullet_impact(game_event *event)
{
	const auto userid = event->get_int(XOR_STR("userid"));
	const auto index = game->engine_client->get_player_for_user_id(userid);

	if (index != game->engine_client->get_local_player())
		return;

	if (shots.empty())
		return;

	shot *last_confirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (it->confirmed && !it->skip)
		{
			last_confirmed = &*it;
			break;
		}
	}

	if (!last_confirmed)
		return;

	last_confirmed->impacted = true;
	last_confirmed->server_info.impacts.emplace_back(event->get_float(XOR_STR("x")), event->get_float(XOR_STR("y")),
													 event->get_float(XOR_STR("z")));
}

void shot_tracker::on_weapon_fire(game_event *event)
{
	const auto userid = event->get_int(XOR_STR("userid"));
	const auto index = game->engine_client->get_player_for_user_id(userid);

	if (index != game->engine_client->get_local_player())
		return;

	if (shots.empty())
		return;

	shot *last_unconfirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (!it->confirmed)
		{
			last_unconfirmed = &*it;
			break;
		}

		it->skip = true;
	}

	if (!last_unconfirmed)
		return;

	last_unconfirmed->confirmed = true;
}

void shot_tracker::on_update_end()
{
	for (auto it = shots.begin(); it != shots.end();)
	{
		if (it->confirmed && it->impacted)
		{
			auto shot = *it;
			it = shots.erase(it);
			on_shot(shot);
		}
		else
			it = next(it);
	}

	for (auto it = shots.begin(); it != shots.end();)
	{
		if (it->time + 2.f < game->globals->curtime)
			it = shots.erase(it);
		else
			it = next(it);
	}
}

void shot_tracker::on_shot(shot &s)
{
	const auto player =
		s.record != nullptr
			? reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(s.record->index))
			: nullptr;

	if (!game->local_player)
		return;

	if (player)
		s.record->player = player;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	const auto &info = pred.info[s.cmd_num % input_max];
	const auto org_start = s.start;
	auto hit_start = false;
	if (info.sequence == s.cmd_num)
		std::tie(s.start.x, s.start.y) = std::make_pair(info.origin.x, info.origin.y);

	const auto angle = calc_angle(s.start, s.server_info.impacts.back());
	vec3 forward;
	angle_vectors(angle, forward);
	const auto end = s.server_info.impacts.back() + forward * wpn->get_cs_weapon_data()->range;

	// check deviation from server.
	custom_tracing::wall_pen pen{}, org_pen{};
	if (!s.manual && player)
	{
		auto &entry = GET_PLAYER_ENTRY(player);
		if (s.record->extrapolated && !entry.records.empty())
		{
			lag_record *latest = nullptr;
			for (auto &rec : entry.records)
				if (rec.valid)
				{
					latest = &rec;
					break;
				}
			if (latest)
			{
				org_pen = trace.wall_penetration(s.start, end, s.record, custom_tracing::bullet_safety::none,
												 s.target_direction);
				s.record = latest;
				s.record->perform_matrix_setup();
			}
		}

		pen = trace.wall_penetration(s.start, end, s.record, custom_tracing::bullet_safety::none, s.target_direction);

		if ((org_start - s.start).length2d() > .01f)
			hit_start =
				trace
					.wall_penetration(org_start, end, s.record, custom_tracing::bullet_safety::none, s.target_direction)
					.did_hit;

		if (s.server_info.damage > 0)
		{
			entry.visuals.last_update = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
			entry.dormant_miss = 0;
		}
	}

	const auto spread_miss = !pen.did_hit && s.server_info.damage <= 0 && s.server_info.index != -1 &&
							 wpn->get_inaccuracy() > 0.f && !s.manual;
	const auto dormancy_miss = s.server_info.damage <= 0 && !s.manual && s.record->dormant;

	auto miss_reason = XOR_STR_STORE("");

	if (s.server_info.damage <= 0 && !s.manual && s.record->dormant)
	{
		miss_reason = XOR_STR_STORE("dormancy");
		++GET_PLAYER_ENTRY(player).dormant_miss;
	}
	else if (spread_miss)
	{
		s.spread_miss = true;

		if (!org_pen.did_hit)
		{
			if (hit_start)
				miss_reason = XOR_STR_STORE("unforeseen circumstances");
			else
				miss_reason = XOR_STR_STORE("spread");
		}
		else
			miss_reason = XOR_STR_STORE("player movement");

		if (player)
			++GET_PLAYER_ENTRY(player).spread_miss;
	}

	if (cfg.misc.event_triggers.get().test(cfg_t::event_shot_info) && (dormancy_miss || spread_miss))
	{
		eventlog.add(0x06, XOR_STR_STORE("Missed shot due to "));
		eventlog.add(0x09, miss_reason);
		eventlog.add(0x06, XOR_STR_STORE("."));
		eventlog.output();
	}

#ifdef CSGO_LUA
	const auto cbk = [&](lua::state &state)
	{
		XOR_STR_STACK(miss_reason_dec, miss_reason);

		state.create_table();
		state.set_field(XOR_STR("manual"), s.manual);
		state.set_field(XOR_STR("secure_point"), (int)pen.safety);
		state.set_field(XOR_STR("result"), s.server_info.damage > 0 ? XOR_STR("hit") : miss_reason_dec);
		state.set_field(XOR_STR("target"), s.server_info.index);
		state.set_field(XOR_STR("tick"), s.server_tick);
		state.set_field(XOR_STR("backtrack"), s.cmd_tick - s.target_tick);
		state.set_field(XOR_STR("hitchance"), s.target ? s.target->hc : -1);
		state.set_field(XOR_STR("client_hitgroup"), (int)s.hitgroup);
		state.set_field(XOR_STR("client_damage"), s.damage);
		state.set_field(XOR_STR("server_hitgroup"), (int)s.server_info.hitgroup);
		state.set_field(XOR_STR("server_damage"), s.server_info.damage);

		return 1;
	};

	lua::api.callback(FNV1A("on_shot_fired"), cbk);
	lua::api.callback(FNV1A("on_shot_registered"), cbk);
#endif

	if (player && s.server_info.damage > 0 && !s.record->dormant)
	{
		if (cfg.local_visuals.hit_indicator.get().test(cfg_t::hit_on_kill) && !player->is_alive())
			debug->draw_player(player, s.record->mat, cfg.local_visuals.hit_color.get(), 4.f);
		else if (cfg.local_visuals.hit_indicator.get().test(cfg_t::hit_on_hit))
			debug->draw_player(player, s.record->mat, cfg.local_visuals.hit_color.get(), 4.f);
	}

	if (cfg.local_visuals.impacts.get())
		for (const auto &impact : s.server_info.impacts)
			game->debug_overlay->box_overlay_2(
				impact, {-1.25f, -1.25f, -1.25f}, {1.25f, 1.25f, 1.25f}, sdk::angle(),
				cfg.local_visuals.server_impacts.get(),
				sdk::color(250, 250, 250,
						   static_cast<uint8_t>(.25f * cfg.local_visuals.server_impacts.get().value.a * 255.f)),
				4.f);

	if ((cfg.legit.enable.get() && !cfg.rage.enable.get()) || !player || s.manual || s.record->dormant ||
		!game->local_player->is_alive())
		return;

	player_list.on_shot_resolve(s, end, spread_miss);

	if (!s.manual && player)
	{
		auto &entry = GET_PLAYER_ENTRY(player);
		if (s.record->extrapolated && !entry.records.empty() && !pen.did_hit && org_pen.did_hit)
			entry.resolver.unreliable_extrapolation = true;
	}
}
} // namespace detail
