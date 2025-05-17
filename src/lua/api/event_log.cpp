#include <base/event_log.h>
#include <lua/api_def.h>

using namespace lua;

int api_def::event_log::add(lua_State *l)
{
	runtime_state s(l);

	const auto r = s.check_arguments({{ltd::number}, {ltd::string}});

	if (!r)
	{
		s.error(XOR_STR("usage: add(color: integer, text: string)"));
		return 0;
	}

	eventlog.add(s.get_integer(1), s.get_string(2));
	return 0;
}

int api_def::event_log::output(lua_State *l)
{
	eventlog.output();
	return 0;
}