#include <base/cfg.h>
#include <lua/api_def.h>
#include <lua/helpers.h>
#include <sdk/convar.h>
#include <sdk/engine_client.h>
#include <util/cvar_lexer.h>

int lua::api_def::engine::is_in_game(lua_State *l)
{
	runtime_state s(l);
	s.push(game->engine_client->is_ingame());

	return 1;
}

int lua::api_def::engine::get_local_player(lua_State *l)
{
	runtime_state s(l);
	s.push(static_cast<int>(game->engine_client->get_local_player()));

	return 1;
}

int lua::api_def::engine::exec(lua_State *l)
{
	using namespace helpers;
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: exec(cmd)"));
		return 0;
	}

	// check if any poor commands were used
	for (const auto &cv : util::parse_cvars(s.get_string(1)))
	{
		sdk::c_command cmd(cv.c_str());

		const auto hash = util::fnv1a(cmd.m_pArgvBuffer);
		if (std::find(blocked_vars.begin(), blocked_vars.end(), hash) != blocked_vars.end())
		{
			s.error(XOR_STR("cannot execute ") + cv + XOR_STR(" because it is restricted"));
			return 0;
		}

		if (std::find(risky_vars.begin(), risky_vars.end(), hash) != risky_vars.end())
		{
			if (!cfg.lua.allow_insecure)
			{
				s.error(XOR_STR("cannot execute ") + cv +
						XOR_STR(" because it is restricted. Enable insecure mode to allow this command"));
				return 0;
			}
		}

		game->engine_client->clientcmd_unrestricted(cv.c_str(), nullptr);
	}

	return 0;
}

int lua::api_def::engine::get_view_angles(lua_State *l)
{
	runtime_state s(l);

	const auto va = game->engine_client->get_view_angles();
	s.push(va.x);
	s.push(va.y);
	s.push(va.z);

	return 3;
}

int lua::api_def::engine::get_player_for_user_id(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}});

	if (!r)
	{
		s.error(XOR_STR("usage: get_player_for_user_id(user_id)"));
		return 0;
	}

	s.push(game->engine_client->get_player_for_user_id(s.get_integer(1)));
	return 1;
}

int lua::api_def::engine::get_player_info(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}});

	if (!r)
	{
		s.error(XOR_STR("usage: get_player_info(id)"));
		return 0;
	}

	const auto info = game->engine_client->get_player_info(s.get_integer(1));

	s.create_table();
	{
		s.set_field(XOR("name"), info.name);
		s.set_field(XOR("user_id"), info.user_id);
		s.set_field(XOR("steam_id"), info.steam_id);
		s.set_field(XOR("steam_id64"), std::to_string(info.steam_id64).c_str());
		s.set_field(XOR("steam_id64_low"), info.xuid_low);
		s.set_field(XOR("steam_id64_high"), info.xuid_high);
		s.set_field(XOR("is_bot"), info.fakeplayer);
		s.set_field(XOR("is_hltv"), info.ishltv);
	}

	return 1;
}

int lua::api_def::engine::set_view_angles(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments(
		{
			{ltd::number},
			{ltd::number},
		},
		true); // can be relaxed so possible to do something like cmd:get_view_angles() as argument

	if (!r)
	{
		s.error(XOR_STR("usage: set_view_angles(x, y)"));
		return 0;
	}

	sdk::angle ang{s.get_float(1), s.get_float(2), 0.f};
	game->engine_client->set_view_angles(ang);
	return 0;
}