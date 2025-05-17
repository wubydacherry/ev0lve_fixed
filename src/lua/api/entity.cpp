#include <detail/aim_helper.h>
#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>
#include <sdk/client_entity_list.h>
#include <sdk/cs_player.h>

int lua::api_def::entities::index(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (s.is_number(2))
	{
		const auto key = s.get_integer(2);

		const auto ent = game->client_entity_list->get_client_entity(key);
		if (!ent)
			return 0;

		s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);
		return 1;
	}

	if (s.is_string(2) && util::fnv1a(s.get_string(2)) == FNV1A("local_player"))
	{
		const auto ent = game->client_entity_list->get_client_entity(game->engine_client->get_local_player());
		if (!ent)
			return 0;

		s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);
		return 1;
	}

	return 0;
}

int lua::api_def::entities::get_entity(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_number(1))
	{
		s.error(XOR_STR("usage: entities.get_entity(number)"));
		return 0;
	}

	auto ent = game->client_entity_list->get_client_entity(s.get_integer(1));

	if (!ent)
		return 0;

	s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);

	return 1;
}

int lua::api_def::entities::get_entity_from_handle(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_number(1))
	{
		s.error(XOR_STR("usage: entities.get_entity_from_handle(number)"));
		return 0;
	}

	auto ent = game->client_entity_list->get_client_entity_from_handle(s.get_integer(1));

	if (!ent)
		return 0;

	s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);

	return 1;
}

int lua::api_def::entities::for_each(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_function(1))
	{
		s.error(XOR_STR("usage: entities.for_each(function)"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	auto func_ref = s.registry_add();

	game->client_entity_list->for_each(
		[&](sdk::client_entity *ent)
		{
			s.registry_get(func_ref);
			s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);
			if (!s.call(1, 0))
			{
				me->did_error = true;
				lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
				if (const auto f = api.find_script_file(me->id); f.has_value())
					f->get().should_unload = true;

				return;
			}
		});

	s.registry_remove(func_ref);

	return 0;
}

int lua::api_def::entities::for_each_z(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_function(1))
	{
		s.error(XOR_STR("usage: entities.for_each_z(function)"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	auto func_ref = s.registry_add();

	game->client_entity_list->for_each_z(
		[&](sdk::client_entity *ent)
		{
			s.registry_get(func_ref);
			s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);
			if (!s.call(1, 0))
			{
				me->did_error = true;
				lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
				if (const auto f = api.find_script_file(me->id); f.has_value())
					f->get().should_unload = true;

				return;
			}
		});

	s.registry_remove(func_ref);

	return 0;
}

int lua::api_def::entities::for_each_player(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_function(1))
	{
		s.error(XOR_STR("usage: entities.for_each_player(function)"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	auto func_ref = s.registry_add();

	game->client_entity_list->for_each_player(
		[&](sdk::cs_player_t *ent)
		{
			s.registry_get(func_ref);
			s.create_user_object_ptr(XOR_STR("csgo.entity"), ent);

			if (!s.call(1, 0))
			{
				me->did_error = true;
				lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
				if (const auto f = api.find_script_file(me->id); f.has_value())
					f->get().should_unload = true;

				return;
			}
		});

	s.registry_remove(func_ref);

	return 0;
}

bool lua::api_def::entity::is_sane(sdk::entity_t *ent)
{
	return std::find(game->client_entity_list->begin(), game->client_entity_list->end(), ent) !=
		   game->client_entity_list->end();
}

int lua::api_def::entity::index(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_string(2))
		return 0;

	switch (util::fnv1a(s.get_string(2)))
	{
	case FNV1A("get_index"):
		s.push(get_index);
		return 1;
	case FNV1A("is_valid"):
		s.push(is_valid);
		return 1;
	case FNV1A("is_alive"):
		s.push(is_alive);
		return 1;
	case FNV1A("is_dormant"):
		s.push(is_dormant);
		return 1;
	case FNV1A("is_player"):
		s.push(is_player);
		return 1;
	case FNV1A("is_enemy"):
		s.push(is_enemy);
		return 1;
	case FNV1A("get_class"):
		s.push(get_class);
		return 1;
	case FNV1A("get_prop"):
		s.push(get_prop);
		return 1;
	case FNV1A("set_prop"):
		s.push(set_prop);
		return 1;
	case FNV1A("get_hitbox_position"):
		s.push(get_hitbox_position);
		return 1;
	case FNV1A("get_eye_position"):
		s.push(get_eye_position);
		return 1;
	case FNV1A("get_player_info"):
		s.push(get_player_info);
		return 1;
	case FNV1A("get_move_type"):
		s.push(get_move_type);
		return 1;
	case FNV1A("get_esp_alpha"):
		s.push(get_esp_alpha);
		return 1;
	case FNV1A("get_ptr"):
		s.push(get_ptr);
		return 1;
	}

	auto ent = *reinterpret_cast<sdk::entity_t **>(s.get_user_data(1));
	if (!ent || !is_sane(ent))
		return 0;

	const auto client_class = ent->get_client_class();
	if (FNV1A_CMP(client_class->network_name, "CCSGameRulesProxy"))
		ent = (sdk::entity_t *)game->rules->data;

	const auto prop_name = s.get_string(2);
	const auto var = helpers::get_netvar(prop_name, client_class);
	if (!var.offset)
		return 0;

	if (var.is_array)
	{
		helpers::lua_var v{ent, var};
		s.create_user_object(XOR_STR("csgo.netvar"), &v);
		return 1;
	}

	return helpers::state_get_prop({ent, var}, s);
}

int lua::api_def::entity::new_index(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.is_string(2))
		return 0;

	auto ent = *reinterpret_cast<sdk::entity_t **>(s.get_user_data(1));
	if (!ent || !is_sane(ent))
		return 0;

	const auto client_class = ent->get_client_class();
	if (FNV1A_CMP(client_class->network_name, "CCSGameRulesProxy"))
		ent = (sdk::entity_t *)game->rules->data;

	const auto prop_name = s.get_string(2);
	const auto var = helpers::get_netvar(prop_name, client_class);
	if (!var.offset || var.is_array)
		return 0;

	helpers::state_set_prop({ent, var}, s, 3);
	return 0;
}

int lua::api_def::entity::get_index(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::entity_t>(s, XOR_STR("usage: ent:get_index()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->index());
	return 1;
}

int lua::api_def::entity::is_alive(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:is_alive()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->is_alive());
	return 1;
}

int lua::api_def::entity::is_valid(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:is_valid()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->is_player() && ent->is_valid());

	return 1;
}

int lua::api_def::entity::is_dormant(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:is_dormant()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->is_dormant());

	return 1;
}

int lua::api_def::entity::is_player(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:is_player()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->is_player());

	return 1;
}

int lua::api_def::entity::is_enemy(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:is_enemy()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->is_player() && ent->is_enemy());

	return 1;
}

int lua::api_def::entity::get_class(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_class()"));

	if (!ent || !is_sane(ent))
		return 0;

	s.push(ent->get_client_class()->network_name);

	return 1;
}

int lua::api_def::entity::get_hitbox_position(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: ent:get_hitbox_position(number)"));
		return 0;
	}

	const auto ent = *reinterpret_cast<sdk::cs_player_t **>(s.get_user_data(1));

	if (!ent || !is_sane(ent))
		return 0;

	sdk::mat3x4 *mtx{};
	if (ent->index() == game->engine_client->get_local_player())
	{
		if (!detail::local_record.has_visual_matrix)
			return 0;

		mtx = detail::local_record.vis_mat;
		// fixup position
		sdk::move_matrix(mtx, detail::local_record.abs_origin, ent->get_abs_origin());
	}
	else
	{
		auto pl = GET_PLAYER_ENTRY(ent);
		if (pl.visuals.has_matrix)
			return 0;

		mtx = pl.visuals.matrix;
	}

	const auto hitbox_pos =
		detail::aim_helper::get_hitbox_position(ent, static_cast<sdk::cs_player_t::hitbox>(s.get_integer(2)), mtx);
	if (hitbox_pos == std::nullopt)
		return 0;

	s.push(hitbox_pos->x);
	s.push(hitbox_pos->y);
	s.push(hitbox_pos->z);

	return 3;
}

int lua::api_def::entity::get_eye_position(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_eye_position()"));

	if (!ent || !is_sane(ent))
		return 0;

	const auto eye_pos = ent->get_eye_position();

	s.push(eye_pos.x);
	s.push(eye_pos.y);
	s.push(eye_pos.z);

	return 3;
}

int lua::api_def::entity::get_player_info(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_player_info()"));

	if (!ent || !is_sane(ent) || !ent->is_player())
		return 0;

	const auto player_info = game->engine_client->get_player_info(ent->index());

	s.create_table();
	{
		s.set_field(XOR("name"), player_info.name);
		s.set_field(XOR("user_id"), player_info.user_id);
		s.set_field(XOR("steam_id"), player_info.steam_id);
		s.set_field(XOR("steam_id64"), std::to_string(player_info.steam_id64).c_str());
		s.set_field(XOR("steam_id64_low"), player_info.xuid_low);
		s.set_field(XOR("steam_id64_high"), player_info.xuid_high);
		s.set_field(XOR("is_bot"), player_info.fakeplayer);
		s.set_field(XOR("is_hltv"), player_info.ishltv);
	}

	return 1;
}

int lua::api_def::entity::get_prop(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}, {ltd::number, true}}))
	{
		s.error(XOR_STR("usage: ent:get_prop(name, index (optional))"));
		return 0;
	}

	auto index = 0;

	// 3 IS 3!
	if (s.is_number(3))
		index = s.get_integer(3);

	auto ent = *reinterpret_cast<sdk::entity_t **>(s.get_user_data(1));
	if (!ent || !is_sane(ent))
		return 0;

	const auto client_class = ent->get_client_class();
	if (FNV1A_CMP(client_class->network_name, "CCSGameRulesProxy"))
		ent = (sdk::entity_t *)game->rules->data;

	const auto prop_name = s.get_string(2);
	const auto var = helpers::get_netvar(prop_name, client_class);
	if (!var.offset)
		return 0;

	const auto addr = uint32_t(ent) + var.offset;
	switch (var.type)
	{
	default:
	case sdk::dpt_int:
		if (var.is_bool)
			s.push(((bool *)addr)[index]);
		else if (var.is_short)
			s.push(((short *)addr)[index]);
		else
			s.push(((int *)addr)[index]);

		return 1;
	case sdk::dpt_string:
		s.push((const char *)addr);
		return 1;
	case sdk::dpt_vector:
		s.push(((float *)addr)[index]);
		s.push(((float *)addr)[index + 1]);
		s.push(((float *)addr)[index + 2]);
		return 3;
	case sdk::dpt_float:
		s.push(((float *)addr)[index]);
		return 1;
	case sdk::dpt_int64:
		s.push(std::to_string(((int64_t *)addr)[index]).c_str());
		return 1;
	case sdk::dpt_vectorxy:
		s.push(((float *)addr)[index]);
		s.push(((float *)addr)[index + 1]);
		return 2;
	}
}

int lua::api_def::entity::set_prop(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}, {ltd::number}}, true))
	{
		s.error(XOR_STR("usage: ent:set_prop(name, index, value(s))"));
		return 0;
	}

	auto index = 0;

	// 3 IS 3!
	if (s.is_number(3))
		index = s.get_integer(3);

	const auto ent = *reinterpret_cast<sdk::entity_t **>(s.get_user_data(1));

	if (!ent || !is_sane(ent))
		return 0;

	const auto client_class = ent->get_client_class();
	const auto prop_name = s.get_string(2);
	const auto var = helpers::get_netvar(prop_name, client_class);
	if (!var.offset)
		return 0;

	const auto addr = uint32_t(ent) + var.offset;
	switch (var.type)
	{
	default:
	case sdk::dpt_int:
		if (var.is_bool)
			((bool *)addr)[index] = s.get_boolean(4);
		else if (var.is_short)
			((short *)addr)[index] = (short)s.get_integer(4);
		else
			((int *)addr)[index] = s.get_integer(4);
		break;
	case sdk::dpt_string:
		break;
	case sdk::dpt_vector:
		((float *)addr)[index] = s.get_float(4);
		((float *)addr)[index + 1] = s.get_float(5);
		((float *)addr)[index + 2] = s.get_float(6);
		break;
	case sdk::dpt_float:
		((float *)addr)[index] = s.get_float(4);
		break;
	case sdk::dpt_int64:
		break;
	case sdk::dpt_vectorxy:
		((float *)addr)[index] = s.get_float(4);
		((float *)addr)[index + 1] = s.get_float(5);
		break;
	}

	return 0;
}

int lua::api_def::entity::get_move_type(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_move_type()"));

	if (!ent || !is_sane(ent) || !ent->is_player())
		return 0;

	s.push(ent->get_move_type());

	return 1;
}

int lua::api_def::entity::get_esp_alpha(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_esp_alpha()"));
	if (!ent || !is_sane(ent) || !ent->is_player())
		return 0;

	s.push(GET_PLAYER_ENTRY(ent).visuals.alpha);
	return 1;
}

int lua::api_def::entity::get_ptr(lua_State *l)
{
	runtime_state s(l);
	if (!game->engine_client->is_ingame())
		return 0;

	const auto ent = extract_type<sdk::cs_player_t>(s, XOR_STR("usage: ent:get_esp_alpha()"));
	if (!ent || !is_sane(ent) || !ent->is_player())
		return 0;

	s.push((int)ent);
	return 1;
}