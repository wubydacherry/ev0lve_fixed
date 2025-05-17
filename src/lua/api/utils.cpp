#include <base/cfg.h>
#include <detail/custom_tracing.h>
#include <detail/player_list.h>
#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>
#include <sdk/client_state.h>
#include <sdk/debug_overlay.h>
#include <sdk/generated.h>
#include <sdk/math.h>
#include <sdk/weapon_system.h>
#include <util/fnv1a.h>

#ifdef CSGO_SECURE
#include <crypto/aes256.h>
#include <util/util.h>
#endif

#include <wininet.h>

using namespace lua;

int api_def::utils::random_int(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}, {ltd::number}});

	if (!r)
	{
		s.error(XOR("usage: random_int(min, max)"));
		return 0;
	}

	s.push(sdk::random_int(s.get_integer(1), s.get_integer(2)));
	return 1;
}

int api_def::utils::random_float(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}, {ltd::number}});

	if (!r)
	{
		s.error(XOR("usage: random_float(min, max)"));
		return 0;
	}

	s.push(sdk::random_float(s.get_float(1), s.get_float(2)));
	return 1;
}

int api_def::utils::flags(lua_State *l)
{
	runtime_state s(l);

	uint32_t result{};
	for (auto i = 1; i <= s.get_stack_top(); ++i)
	{
		if (s.is_number(i))
			result |= s.get_integer(i);
	}

	s.push(static_cast<int>(result));
	return 1;
}

int api_def::utils::find_interface(lua_State *l)
{
	runtime_state s(l);

	if (!cfg.lua.allow_insecure.get())
	{
		s.error(XOR_STR("find_interface is not available with Allow insecure disabled"));
		return 0;
	}

	const auto r = s.check_arguments({{ltd::string}, {ltd::string}});

	if (!r)
	{
		s.error(XOR("usage: find_interface(module, name)"));
		return 0;
	}

	try
	{
		s.push((int)helpers::get_interface(util::fnv1a(s.get_string(1)), util::fnv1a(s.get_string(2))));
	}
	catch (...)
	{
		s.push_nil();
	}

	return 1;
}

int api_def::utils::find_pattern(lua_State *l)
{
	runtime_state s(l);

	if (!cfg.lua.allow_insecure.get())
	{
		s.error(XOR_STR("find_pattern is not available with Allow insecure disabled"));
		return 0;
	}

	const auto r = s.check_arguments({{ltd::string}, {ltd::string}, {ltd::number, true}});

	if (!r)
	{
		s.error(XOR("usage: find_pattern(module, pattern, offset = 0)"));
		return 0;
	}

	const auto v = s.get_stack_top() >= 3 ? s.get_integer(3) : 0;

	try
	{
		const auto result =
			helpers::find_pattern(helpers::find_module(util::fnv1a(s.get_string(1))), s.get_string(2), v);
		if (result.error == helpers::scan_result::found)
		{
			s.push((int)result.address);
			return 1;
		}

		switch (result.error)
		{
		case helpers::scan_result::not_found:
			s.error(XOR_STR("pattern: not found"));
			break;
		case helpers::scan_result::too_large_deref:
			s.error(tfm::format(XOR_STR("pattern: too large address to get (max 4 bytes) at %d"), result.error_pos));
			break;
		case helpers::scan_result::too_small_deref:
			s.error(tfm::format(XOR_STR("pattern: too small address to get (min 1 byte) at %d"), result.error_pos));
			break;
		case helpers::scan_result::not_a_power:
			s.error(XOR_STR("pattern: offset is not a power of 2"));
			break;
		case helpers::scan_result::exception:
			s.error(XOR_STR("pattern: exception while reading memory"));
			break;
		case helpers::scan_result::parse_error:
			s.error(tfm::format(XOR_STR("pattern: parse error at %d"), result.error_pos));
			break;
		default:
			break;
		}
	}
	catch (...)
	{
		s.error(XOR_STR("pattern: unknown exception"));
	}

	return 0;
}

int api_def::utils::get_weapon_info(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}});

	if (!r)
	{
		s.error(XOR("usage: get_weapon_info(item_definition_index)"));
		return 0;
	}

	const auto n = s.get_integer(1);
	if (!n)
		return 0;

	const auto weapon_data = game->weapon_system->get_weapon_data((sdk::item_definition_index)n);
	if (!weapon_data)
		return 0;

	s.create_table();
	{
		s.set_field(XOR("console_name"), weapon_data->consolename);
		s.set_field(XOR("max_clip1"), weapon_data->maxclip1);
		s.set_field(XOR("max_clip2"), weapon_data->imaxclip2);
		s.set_field(XOR("world_model"), weapon_data->szworldmodel);
		s.set_field(XOR("view_model"), weapon_data->szviewmodel);
		s.set_field(XOR("weapon_type"), weapon_data->weapontype);
		s.set_field(XOR("weapon_price"), weapon_data->iweaponprice);
		s.set_field(XOR("kill_reward"), weapon_data->ikillaward);
		s.set_field(XOR("cycle_time"), weapon_data->cycle_time);
		s.set_field(XOR("is_full_auto"), weapon_data->bfullauto);
		s.set_field(XOR("damage"), weapon_data->idamage);
		s.set_field(XOR("range"), weapon_data->range);
		s.set_field(XOR("range_modifier"), weapon_data->flrangemodifier);
		s.set_field(XOR("throw_velocity"), weapon_data->flthrowvelocity);
		s.set_field(XOR("has_silencer"), weapon_data->bhassilencer);
		s.set_field(XOR("max_player_speed"), weapon_data->flmaxplayerspeed);
		s.set_field(XOR("max_player_speed_alt"), weapon_data->flmaxplayerspeedalt);
		s.set_field(XOR("zoom_fov1"), weapon_data->zoom_fov1);
		s.set_field(XOR("zoom_fov2"), weapon_data->zoom_fov2);
		s.set_field(XOR("zoom_levels"), weapon_data->zoom_levels);
	}

	return 1;
}

int api_def::utils::create_timer(lua_State *l, bool run)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}, {ltd::function}});

	if (!r)
	{
		s.error(XOR("usage: new_timer(delay, function)"));
		return 0;
	}

	const auto &me = lua::api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	const auto ref = s.registry_add();
	const auto tmr = std::make_shared<helpers::timed_callback>(
		s.get_float(1),
		[ref, l, me]()
		{
			runtime_state s(l);
			s.registry_get(ref);

			if (!s.is_nil(-1))
			{
				if (!s.call(0, 0))
				{
					me->did_error = true;
					lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
					if (const auto f = api.find_script_file(me->id); f.has_value())
						f->get().should_unload = true;

					return;
				}
			}
			else
				s.pop(1);
		});

	helpers::timers[me->id].emplace_back(tmr);

	if (run)
	{
		tmr->run_once();
		return 0;
	}

	// create lua usertype
	std::weak_ptr<helpers::timed_callback> obj{tmr};
	s.create_user_object<decltype(obj)>(XOR_STR("utils.timer"), &obj);

	return 1;
}

int api_def::utils::new_timer(lua_State *l) { return create_timer(l); }

int api_def::utils::run_delayed(lua_State *l) { return create_timer(l, true); }

int api_def::utils::world_to_screen(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({
		{ltd::number},
		{ltd::number},
		{ltd::number},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: world_to_screen(x, y, z): x, y | nil"));
		return 0;
	}

	sdk::vec3 in{s.get_float(1), s.get_float(2), s.get_float(3)}, out{};
	if (!world_to_screen(in, out))
		return 0;

	s.push(out.x);
	s.push(out.y);
	return 2;
}

int api_def::utils::get_rtt(lua_State *l)
{
	runtime_state s(l);

	if (!game->engine_client->is_ingame())
	{
		s.push(0.f);
		return 0;
	}

	s.push(game->client_state->net_channel->get_avg_latency(sdk::flow_incoming) +
		   game->client_state->net_channel->get_avg_latency(sdk::flow_outgoing));
	return 1;
}

int api_def::utils::get_time(lua_State *l)
{
	runtime_state s(l);

	const auto time = std::time(nullptr);
	const auto tm = std::localtime(&time);

	s.create_table();
	s.set_field(XOR("sec"), tm->tm_sec);
	s.set_field(XOR("min"), tm->tm_min);
	s.set_field(XOR("hour"), tm->tm_hour);
	s.set_field(XOR("month_day"), tm->tm_mday);
	s.set_field(XOR("month"), tm->tm_mon + 1);
	s.set_field(XOR("year"), tm->tm_year + 1900);
	s.set_field(XOR("week_day"), tm->tm_wday + 1);
	s.set_field(XOR("year_day"), tm->tm_yday + 1);

	return 1;
}

int api_def::utils::http_get(lua_State *l)
{
	helpers::print(XOR_STR("warning: utils.http_get is deprecated. Use net.http_get instead!"));

#ifdef CSGO_SECURE
	return net::http_get(l);
#else
	return 0;
#endif
}

int api_def::utils::http_post(lua_State *l)
{
	helpers::print(XOR_STR("warning: utils.http_post is deprecated. Use net.http_post instead!"));
#ifdef CSGO_SECURE
	return net::http_post(l);
#else
	return 0;
#endif
}

int api_def::utils::json_encode(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({{ltd::table}});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.json_encode(table)"));
		return 0;
	}

	const auto res = helpers::parse_table(l, 1);
	s.push(res.dump().c_str());
	return 1;
}

int api_def::utils::json_decode(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({{ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.json_decode(json)"));
		return 0;
	}

	try
	{
		auto data = nlohmann::json::parse(s.get_string(1));
		return helpers::load_table(l, data);
	}
	catch (...)
	{
		s.error(XOR_STR("invalid json string"));
		return 0;
	}
}

int api_def::utils::set_clan_tag(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({{ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.set_clan_tag(tag)"));
		return 0;
	}

	const auto set_clantag =
		reinterpret_cast<void(__fastcall *)(const char *, const char *)>(game->engine.at(sdk::functions::set_clantag));
	set_clantag(s.get_string(1), s.get_string(1));
	return 0;
}

int api_def::utils::scale_damage(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({
		{ltd::user_data},
		{ltd::number},
		{ltd::number},
		{ltd::number},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.scale_damage(entity, hitgroup, damage, weapon)"));
		return 0;
	}

	const auto ent = extract_type<sdk::entity_t>(s, XOR_STR("invalid entity"));
	if (!ent || !entity::is_sane(ent) || !ent->is_player())
		return 0;

	const auto weapon_data = game->weapon_system->get_weapon_data((sdk::item_definition_index)s.get_integer(3));
	if (!weapon_data)
		return 0;

	s.push(detail::trace.scale_damage((sdk::cs_player_t *)ent, s.get_float(2), weapon_data->flarmorratio,
									  s.get_integer(4)));
	return 1;
}

int api_def::utils::trace(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({
		{ltd::user_data},
		{ltd::user_data},
		{ltd::number},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.trace(start, end, skip_index)"));
		return 0;
	}

	if (!game->engine_client->is_ingame())
		return 0;

	const auto from = s.user_data<sdk::vec3>(1);
	const auto to = s.user_data<sdk::vec3>(2);
	const auto skip_index = s.get_integer(3);

	sdk::trace_simple_filter simple_filter((sdk::entity_t *)game->client_entity_list->get_client_entity(skip_index));
	sdk::trace_no_player_filter no_player_filter{};

	sdk::game_trace tr;
	sdk::ray ray;
	ray.init(from, to);

	game->engine_trace->trace_ray(
		ray, mask_shot, skip_index == -1 ? (sdk::trace_filter *)&no_player_filter : (sdk::trace_filter *)&simple_filter,
		&tr);

	s.create_table();
	{
		s.create_user_object<decltype(tr.endpos)>(XOR_STR("vec3"), &tr.endpos);
		s.set_field(XOR("endpos"));
		s.set_field(XOR("fraction"), tr.fraction);
		s.set_field(XOR("allsolid"), tr.allsolid);
		s.set_field(XOR("startsolid"), tr.startsolid);
		s.set_field(XOR("fractionleftsolid"), tr.fractionleftsolid);
		s.create_user_object<decltype(tr.plane.normal)>(XOR_STR("vec3"), &tr.plane.normal);
		s.set_field(XOR("plane_normal"));
		s.set_field(XOR("plane_dist"), tr.plane.dist);
		s.set_field(XOR("contents"), tr.contents);
		s.set_field(XOR("disp_flags"), tr.disp_flags);
		s.set_field(XOR("surface_name"), tr.surface.name);
		s.set_field(XOR("surface_props"), tr.surface.surface_props);
		s.set_field(XOR("surface_flags"), tr.surface.flags);
		s.set_field(XOR("ent_index"), tr.entity ? tr.entity->index() : -1);
		s.set_field(XOR("hitbox"), tr.hitbox);
		s.set_field(XOR("hitgroup"), tr.hitgroup);
	}

	return 1;
}

int api_def::utils::trace_bullet(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({
		{ltd::user_data},
		{ltd::user_data},
		{ltd::user_data},
		{ltd::number},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.trace_bullet(target, from, to, item_definition_index)"));
		return 0;
	}

	const auto ent = extract_type<sdk::entity_t>(s, XOR_STR("invalid entity"));
	if (!ent || !entity::is_sane(ent) || !ent->is_player())
		return 0;

	const auto from = s.user_data<sdk::vec3>(2);
	const auto to = s.user_data<sdk::vec3>(3);

	const auto weapon_data = game->weapon_system->get_weapon_data((sdk::item_definition_index)s.get_integer(4));
	if (!weapon_data)
		return 0;

	auto player = GET_PLAYER_ENTRY((sdk::cs_player_t *)ent);

	float first{}, second{};
	player.get_boundary_times(first, second);
	const auto lag_record = player.get_record(first);
	const auto ret = detail::trace.wall_penetration(from, to, lag_record, detail::custom_tracing::bullet_safety::none,
													std::nullopt, nullptr, false, weapon_data);
	s.push(ret.damage);
	s.create_table();
	{
		s.set_field(XOR_STR("hitbox"), (int)ret.hitbox);
		s.set_field(XOR_STR("hitgroup"), ret.hitgroup);
		s.set_field(XOR_STR("did_hit"), ret.did_hit);
	}

	return 2;
}

#ifdef CSGO_SECURE
namespace lua::api_def::utils
{
int aes256_encrypt(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: utils.aes256_encrypt(key, data)"));
		return 0;
	}

	std::string lua_key{s.get_string(1)}, lua_data{s.get_string(2)};
	std::vector<uint8_t> key, data, enc;
	key.resize(lua_key.size());
	data.resize(lua_data.size());
	std::copy(lua_key.begin(), lua_key.end(), key.begin());
	std::copy(lua_data.begin(), lua_data.end(), data.begin());

	Aes256::encrypt(key, data, enc);

	std::string enc_str;
	enc_str.resize(enc.size());
	std::copy(enc.begin(), enc.end(), enc_str.begin());

	s.push(enc_str);
	return 1;
}

int aes256_decrypt(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: utils.aes256_decrypt(key, data)"));
		return 0;
	}

	std::string lua_key{s.get_string(1)}, lua_data{s.get_string(2)};
	std::vector<uint8_t> key, data, dec;
	key.resize(lua_key.size());
	data.resize(lua_data.size());
	std::copy(lua_key.begin(), lua_key.end(), key.begin());
	std::copy(lua_data.begin(), lua_data.end(), data.begin());

	Aes256::decrypt(key, data, dec);

	std::string dec_str;
	dec_str.resize(dec.size());
	std::copy(dec.begin(), dec.end(), dec_str.begin());

	s.push(dec_str);
	return 1;
}

int base64_encode(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}}))
	{
		s.error(XOR_STR("usage: utils.base64_encode(data)"));
		return 0;
	}

	s.push(evo::util::base64_encode(s.get_string(1)));
	return 1;
}

int base64_decode(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}}))
	{
		s.error(XOR_STR("usage: utils.base64_decode(data)"));
		return 0;
	}

	try
	{
		s.push(evo::util::base64_decode(s.get_string(1)));
		return 1;
	}
	catch (const std::exception &e)
	{
		s.error(e.what());
		return 0;
	}
}

int get_unix_time(lua_State *l)
{
	runtime_state s(l);
	s.push((double)evo::util::get_unix_time());
	return 1;
}
} // namespace lua::api_def::utils
#endif