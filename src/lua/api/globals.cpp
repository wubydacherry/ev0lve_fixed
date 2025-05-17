#include <base/debug_overlay.h>
#include <detail/player_list.h>
#include <gui/gui.h>
#include <hacks/antiaim.h>
#include <hacks/tickbase.h>
#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>
#include <sdk/global_vars_base.h>

#ifdef CSGO_SECURE
#include <VirtualizerSDK.h>
#include <network/helpers.h>
#endif

using namespace hacks;
using namespace detail;

int lua::api_def::panic(lua_State *l)
{
	runtime_state s(l);

	MessageBoxA(nullptr, s.get_string(-1), XOR_STR("Critical error"), MB_OK | MB_ICONERROR);
	return 0;
}

int lua::api_def::print(lua_State *l)
{
	runtime_state s(l);
	if (!s.get_stack_top())
	{
		s.error(XOR_STR("print() requires at least one argument"));
		return 0;
	}

	std::string output{};

	auto n = 1;
	do
	{
		if (s.is_boolean(n))
			output += s.get_boolean(n) ? XOR_STR("true") : XOR_STR("false");
		else if (s.is_number(n))
			output += std::to_string(s.get_number(n));
		else if (s.is_string(n))
			output += s.get_string(n);
	} while (++n <= s.get_stack_top());

	size_t last_pos{};
	while ((last_pos = output.find(XOR_STR("%"), last_pos)) != std::string::npos)
	{
		output = output.replace(last_pos, 1, XOR_STR("%%"));
		last_pos += 2;
	}

	lua::helpers::print(output.c_str());
	return 0;
}

int lua::api_def::require(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: require(lib)"));
		return 0;
	}

	// ignore ffi require
	if (FNV1A_CMP(s.get_string(1), "ffi"))
	{
		s.get_global(XOR_STR("ffi"));
		return 1;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("sandbox problem"));
		return 0;
	}

	// attempt loading lib
	script_file file{st_library, s.get_string(1)};

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	s.load_file(file.get_file_path().c_str());
	if (!s.call(0, 1))
		s.error(s.get_last_error_description());

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif

	return 1;
}

int lua::api_def::loadfile(lua_State *l)
{
	runtime_state s(l);

	if (!cfg.lua.allow_dynamic_load.get())
	{
		s.error(XOR_STR("dynamic loading is disabled"));
		return 0;
	}

	const auto r = s.check_arguments({{ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: loadfile(path)"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("sandbox problem"));
		return 0;
	}

	// check if it is a proprietary script
	std::string content;
#ifdef CSGO_SECURE
	if (me->remote.is_proprietary)
		content = network::decrypt_script(s.get_string(1), me->remote.id, me->remote.last_update);
	else
#endif
	{
		// read from file
		std::ifstream file(s.get_string(1));
		if (!file.is_open())
		{
			s.error(XOR_STR("file not found"));
			return 0;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		content = buffer.str();
	}

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	if (!s.load_string(content))
	{
		s.error(s.get_last_error_description());
		return 0;
	}

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif

	return 1;
}

int lua::api_def::stub_new_index(lua_State *l)
{
	runtime_state s(l);
	s.error(XOR_STR("access violation: overriding fields is forbidden"));

	return 0;
}

int lua::api_def::stub_index(lua_State *l)
{
	runtime_state s(l);
	return 0;
}

int lua::api_def::server_index(lua_State *l)
{
	if (!game->engine_client->is_ingame())
		return 0;

	runtime_state s(l);

	const auto key = util::fnv1a(s.get_as_string(2).c_str());
	switch (key)
	{
	case FNV1A("map_name"):
		s.push(game->client_state->level_name);
		return 1;
	case FNV1A("address"):
		s.push(game->client_state->net_channel->get_address());
		return 1;
	case FNV1A("max_players"):
		s.push(game->globals->max_clients);
		return 1;
	default:
		return 0;
	}
}

int lua::api_def::ev0lve_index(lua_State *l)
{
	runtime_state s(l);

	const auto key = util::fnv1a(s.get_as_string(2).c_str());
	switch (key)
	{
	case FNV1A("username"):
		s.push(evo::gui::ctx->user.username.c_str());
		return 1;
	case FNV1A("build"):
#ifndef CSGO_SECURE
		s.push(XOR_STR("dev"));
#else
		s.push(network::get_build().c_str());
#endif
		return 1;
	case FNV1A("lag_ticks"):
		s.push(game->client_state ? game->client_state->choked_commands : 0);
		return 1;
	case FNV1A("can_fastfire"):
		s.push(cfg.rage.enable_fast_fire && tickbase.compute_current_limit() > 0);
		return 1;
	case FNV1A("charged"):
		s.push(game->engine_client->is_ingame()
				   ? ((float)hacks::tickbase.compute_current_limit() / ((float)sv_maxusrcmdprocessticks - 2.f))
				   : 0.f);
		return 1;
	case FNV1A("in_fakeduck"):
		s.push(cfg.rage.fake_duck);
		return 1;
	case FNV1A("in_slowwalk"):
		s.push(cfg.rage.slowwalk);
		return 1;
	case FNV1A("allow_dynamic_load"):
		s.push(cfg.lua.allow_dynamic_load);
		return 1;
	case FNV1A("allow_insecure"):
		s.push(cfg.lua.allow_insecure);
		return 1;
	case FNV1A("desync"):
		s.push(std::clamp(hacks::aa.previous_yaw_modifier, 0.f, 1.f));
		return 1;
	default:
		return 0;
	}
}

int lua::api_def::global_vars_index(lua_State *l)
{
	runtime_state s(l);

	const auto key = util::fnv1a(s.get_as_string(2).c_str());
	switch (key)
	{
	case FNV1A("realtime"):
		s.push(game->globals->realtime);
		return 1;
	case FNV1A("framecount"):
		s.push(game->globals->framecount);
		return 1;
	case FNV1A("curtime"):
		s.push(game->globals->curtime);
		return 1;
	case FNV1A("frametime"):
		s.push(game->globals->frametime);
		return 1;
	case FNV1A("tickcount"):
		s.push(game->globals->tickcount);
		return 1;
	case FNV1A("interval_per_tick"):
		s.push(game->globals->interval_per_tick);
		return 1;
	default:
		return 0;
	}
}

int lua::api_def::game_rules_index(lua_State *l)
{
	runtime_state s(l);

	if (!game->rules->data)
		return 0;

	const auto key = util::fnv1a(s.get_as_string(2).c_str());
	switch (key)
	{
	case FNV1A("is_valve_server"):
		s.push(game->rules->data->get_is_valve_ds());
		return 1;
	case FNV1A("is_freeze_period"):
		s.push(game->rules->data->get_freeze_period());
		return 1;
	case FNV1A("is_warmup_period"):
		s.push(game->rules->data->get_warmup_period());
		return 1;
	case FNV1A("is_paused"):
		s.push(game->rules->data->get_match_waiting_for_resume());
		return 1;
	default:
		return 0;
	}
}