#include <base/game.h>
#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>
#include <sdk/model_render.h>

namespace lua::api_def::mat
{
int gc(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	const auto mat = s.user_data_ptr<sdk::material>(1);
	mat->decrement_reference_count();

	return 0;
}

int index(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
		return 0;

	switch (util::fnv1a(s.get_string(2)))
	{
	case FNV1A("modulate"):
		s.push(modulate);
		return 1;
	case FNV1A("set_flag"):
		s.push(set_flag);
		return 1;
	case FNV1A("get_flag"):
		s.push(get_flag);
		return 1;
	case FNV1A("find_var"):
		s.push(find_var);
		return 1;
	case FNV1A("get_name"):
		s.push(get_name);
		return 1;
	case FNV1A("get_group"):
		s.push(get_group);
		return 1;
	}

	uint32_t tok;
	const auto var = s.user_data_ptr<sdk::material>(1)->find_var_fast(s.get_string(2), &tok);
	if (!var)
		return 0;

	s.create_user_object_ptr(XOR_STR("csgo.material_var"), var);
	return 1;
}

int get_float(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	s.push(s.user_data_ptr<sdk::material_var>(1)->get_float());
	return 1;
}

int set_float(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: material_var:set_float(val)"));
		return 0;
	}

	s.user_data_ptr<sdk::material_var>(1)->set_float(s.get_float(2));
	return 0;
}

int get_int(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	s.push(s.user_data_ptr<sdk::material_var>(1)->get_int());
	return 1;
}

int set_int(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: material_var:set_int(val)"));
		return 0;
	}

	s.user_data_ptr<sdk::material_var>(1)->set_int(s.get_integer(2));
	return 0;
}

int get_string(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	s.push(s.user_data_ptr<sdk::material_var>(1)->get_string());
	return 1;
}

int set_string(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: material_var:set_string(val)"));
		return 0;
	}

	s.user_data_ptr<sdk::material_var>(1)->set_string(s.get_string(2));
	return 0;
}

int get_vector(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	const auto var = s.user_data_ptr<sdk::material_var>(1);
	const auto vec = var->get_vector();
	const auto vec_sz = var->get_vector_size();

	for (int i{}; i < vec_sz; ++i)
		s.push(vec[i]);

	return vec_sz;
}

int set_vector(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}, {ltd::number}}, true))
	{
		s.error(XOR_STR("usage: material_var:set_vector(x, y, z?, w?)"));
		return 0;
	}

	const auto var = s.user_data_ptr<sdk::material_var>(1);
	var->set_vector_component(s.get_float(2), 0);
	var->set_vector_component(s.get_float(3), 1);
	if (s.get_stack_top() >= 4)
		var->set_vector_component(s.get_float(4), 2);
	if (s.get_stack_top() >= 5)
		var->set_vector_component(s.get_float(5), 3);

	return 0;
}

int modulate(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::table}}))
	{
		s.error(XOR_STR("usage: material:modulate(color)"));
		return 0;
	}

	const auto col = extract_color(s, 2);
	const auto mat = s.user_data_ptr<sdk::material>(1);
	mat->alpha_modulate(col.value.a);
	mat->color_modulate(col.value.r, col.value.g, col.value.b);
	return 0;
}

int set_flag(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}, {ltd::boolean}}))
	{
		s.error(XOR_STR("usage: material:set_flag(flag, state)"));
		return 0;
	}

	s.user_data_ptr<sdk::material>(1)->set_material_var_flag((sdk::material_var_flags)s.get_integer(2),
															 s.get_boolean(3));
	return 0;
}

int get_flag(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::number}}))
	{
		s.error(XOR_STR("usage: material:get_flag(flag)"));
		return 0;
	}

	s.push(s.user_data_ptr<sdk::material>(1)->get_material_var_flag((sdk::material_var_flags)s.get_integer(2)));
	return 1;
}

int find_var(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: material:find_var(name)"));
		return 0;
	}

	uint32_t tok;
	const auto var = s.user_data_ptr<sdk::material>(1)->find_var_fast(s.get_string(2), &tok);
	if (!var)
		return 0;

	s.create_user_object_ptr(XOR_STR("csgo.material_var"), var);
	return 1;
}

int get_name(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	s.push(s.user_data_ptr<sdk::material>(1)->get_name());
	return 1;
}

int get_group(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}))
		return 0;

	s.push(s.user_data_ptr<sdk::material>(1)->get_group());
	return 1;
}

int create(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::string}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: mat.create(name, type, kv)"));
		return 0;
	}

	const auto kv = sdk::keyvalues::produce(s.get_string(1), s.get_string(2), s.get_string(3));
	const auto mat = game->material_system->create_material(s.get_string(1), kv);
	if (!mat)
	{
		kv->~keyvalues();
		s.error(XOR_STR("failed to create material"));
		return 0;
	}

	mat->increment_reference_count();
	s.create_user_object_ptr(XOR_STR("csgo.material"), mat);
	return 1;
}

int find(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::string}}))
	{
		s.error(XOR_STR("usage: mat.find(name, texture)"));
		return 0;
	}

	const auto mat = game->material_system->find_material(s.get_string(1), s.get_string(2));
	if (!mat)
		return 0;

	mat->increment_reference_count();
	s.create_user_object_ptr(XOR_STR("csgo.material"), mat);
	return 1;
}

int for_each_material(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::function}}))
	{
		s.error(XOR_STR("usage: mat.for_each_material(callback)"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	const auto fn = s.registry_add();
	for (auto i = game->material_system->first_material(); i != game->material_system->invalid_material();
		 i = game->material_system->next_material(i))
	{
		const auto mat = game->material_system->get_material(i);
		if (!mat)
			continue;

		mat->increment_reference_count();

		s.registry_get(fn);
		s.create_user_object_ptr(XOR_STR("csgo.material"), mat);
		if (!s.call(1, 0))
		{
			me->did_error = true;
			helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
			if (const auto f = api.find_script_file(me->id); f.has_value())
				f->get().should_unload = true;

			return 0;
		}
	}

	s.registry_remove(fn);
	return 0;
}

int override_material(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::user_data}}) && !s.check_arguments({{ltd::nil}}))
	{
		s.error(XOR_STR("usage: mat.override_material(material)"));
		return 0;
	}

	game->model_render->forced_material_override(s.is_nil(1) ? nullptr : s.user_data_ptr<sdk::material>(1));
	return 0;
}
} // namespace lua::api_def::mat
