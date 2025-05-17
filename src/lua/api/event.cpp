#include <lua/api_def.h>
#include <sdk/game_event_manager.h>
#include <util/fnv1a.h>

namespace lua::api_def::event
{
int index(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: event.NAME"));
		return 0;
	}

	switch (util::fnv1a(s.get_string(2)))
	{
	case FNV1A("get_name"):
		s.push(get_name);
		return 1;
	case FNV1A("get_bool"):
		s.push(get_bool);
		return 1;
	case FNV1A("get_int"):
		s.push(get_int);
		return 1;
	case FNV1A("get_float"):
		s.push(get_float);
		return 1;
	case FNV1A("get_string"):
		s.push(get_string);
		return 1;
	}

	const auto ev = s.user_data_ptr<sdk::game_event>(1);
	const auto key = ev->kv->find_key(s.get_string(2));
	if (!key)
		return 0;

	switch (key->data_type)
	{
	case sdk::keyvalues::TYPE_STRING:
		s.push(key->s_value);
		return 1;
	case sdk::keyvalues::TYPE_INT:
		s.push(key->i_value);
		return 1;
	case sdk::keyvalues::TYPE_FLOAT:
		s.push(key->f_value);
		return 1;
	}

	return 0;
}
} // namespace lua::api_def::event

int lua::api_def::event::get_name(lua_State *l)
{
	runtime_state s(l);

	if (!s.check_arguments({{ltd::user_data}}))
	{
		s.error(XOR_STR("usage: obj:get_name()"));
		return 0;
	}

	const auto ev = *reinterpret_cast<sdk::game_event **>(s.get_user_data(1));
	s.push(ev->get_name());
	return 1;
}

int lua::api_def::event::get_bool(lua_State *l)
{
	runtime_state s(l);

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: obj:get_bool(key_name)"));
		return 0;
	}

	const auto ev = *reinterpret_cast<sdk::game_event **>(s.get_user_data(1));
	s.push(ev->get_bool(s.get_string(2)));
	return 1;
}

int lua::api_def::event::get_int(lua_State *l)
{
	runtime_state s(l);

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: obj:get_int(key_name)"));
		return 0;
	}

	const auto ev = *reinterpret_cast<sdk::game_event **>(s.get_user_data(1));
	s.push(ev->get_int(s.get_string(2)));
	return 1;
}

int lua::api_def::event::get_float(lua_State *l)
{
	runtime_state s(l);

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: obj:get_float(key_name)"));
		return 0;
	}

	const auto ev = *reinterpret_cast<sdk::game_event **>(s.get_user_data(1));
	s.push(ev->get_float(s.get_string(2)));
	return 1;
}

int lua::api_def::event::get_string(lua_State *l)
{
	runtime_state s(l);

	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: obj:get_string(key_name)"));
		return 0;
	}

	const auto ev = *reinterpret_cast<sdk::game_event **>(s.get_user_data(1));
	s.push(ev->get_string(s.get_string(2)));
	return 1;
}