
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/event_log.h>
#include <detail/events.h>
#include <detail/shot_tracker.h>
#include <hacks/skinchanger.h>
#include <hacks/visuals.h>
#include <sdk/surface.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

using namespace sdk;
using namespace hacks;

namespace detail
{
events evnt{};

void events::fire_game_event(game_event *event)
{
	switch (util::fnv1a(event->get_name()))
	{
	case FNV1A("player_hurt"):
		shot_track.on_player_hurt(event);
		vis->on_player_hurt(event);
		eventlog.on_player_hurt(event);
		break;
	case FNV1A("player_death"):
	    skin_changer->on_player_death(event);
		break;
	case FNV1A("item_purchase"):
		eventlog.on_purchase(event);
		break;
	case FNV1A("weapon_fire"):
		shot_track.on_weapon_fire(event);
		break;
	case FNV1A("bullet_impact"):
		shot_track.on_bullet_impact(event);
		break;
	case FNV1A("round_end"):
		is_round_end = true;
		skin_changer->save_skins();
		vis->abort_bomb();
		break;
	case FNV1A("round_start"):
		is_round_end = is_game_end = false;
		is_freezetime = true;
		vis->abort_bomb();
		misc::on_round_start();
		print_to_console(XOR_STR("====================> ROUND START <====================\n"));
		break;
	case FNV1A("bomb_beginplant"):
		vis->on_bomb_beginplant(event);
		break;
	case FNV1A("bomb_planted"):
		vis->on_bomb_planted();
		break;
	case FNV1A("bomb_abortplant"):
	case FNV1A("bomb_defused"):
	case FNV1A("bomb_exploded"):
		vis->abort_bomb();
		return;
	case FNV1A("round_freeze_end"):
		is_freezetime = is_game_end = false;
		break;
	case FNV1A("cs_intermission"):
	case FNV1A("announce_phase_end"):
	case FNV1A("cs_win_panel_match"):
		is_game_end = true;
		break;
	case FNV1A("grenade_thrown"):
		game->nade_throw_times[game->engine_client->get_player_for_user_id(event->get_int(XOR_STR("userid")))] =
			game->globals->curtime;
		break;
	default:
		break;
	}

#ifdef CSGO_LUA
	const auto name = XOR_STR("on_") + std::string(event->get_name());
	lua::api.create_callback(name.c_str());

	const auto cbk = [event](lua::state &s) -> int
	{
		s.create_user_object_ptr(XOR_STR("csgo.event"), event);
		return 1;
	};

	lua::api.callback(FNV1A("on_game_event"), cbk);
	lua::api.callback(util::fnv1a(name.c_str()), cbk);
#endif
}

int events::get_event_debug_id() { return XOR_32(42); }
} // namespace detail
