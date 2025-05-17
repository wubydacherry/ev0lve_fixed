#include <lua/api_def.h>
#include <lua/helpers.h>

namespace lua::api_def::net_prop
{
int index(lua_State *l)
{
	runtime_state s(l);

	const auto v = s.user_data<helpers::lua_var>(1);
	if (!entity::is_sane(v.ent) || !v.v.is_array)
		return 0;

	helpers::lua_var vr{v};
	vr.v.is_array = false;
	vr.v.offset += 4 * s.get_integer(2);
	return helpers::state_get_prop(vr, s);
}

int new_index(lua_State *l)
{
	runtime_state s(l);

	const auto v = s.user_data<helpers::lua_var>(1);
	if (!entity::is_sane(v.ent) || !v.v.is_array)
		return 0;

	helpers::lua_var vr{v};
	vr.v.is_array = false;
	vr.v.offset += 4 * s.get_integer(2);
	helpers::state_set_prop(vr, s, 3);
	return 0;
}
} // namespace lua::api_def::net_prop
