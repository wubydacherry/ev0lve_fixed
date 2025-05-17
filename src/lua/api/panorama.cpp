#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>

#include <base/cfg.h>
#include <base/game.h>
#include <sdk/panorama.h>

namespace lua::api_def::panorama
{
bool value_to_lua(runtime_state &s, sdk::v8::value_t *val)
{
	int _unk;
	if (val->is_number_object() || val->is_number())
		s.push(val->number_value());
	else if (val->is_boolean() || val->is_boolean_object())
		s.push(val->boolean_value());
	else if (val->is_string() || val->is_string_object())
	{
		sdk::v8::utf8_string_t str;
		str.ctor(val);
		s.push(std::string(str.value, str.length));
		str.dtor();
	}
	else if (val->is_object())
	{
		// not going to do this yet.
		if (val->is_function())
			return false;

		const auto obj = (sdk::v8::object_t *)val->to_object(&_unk)->value;
		if (!obj)
			return false;

		if (val->is_array())
		{
			const auto arr = (sdk::v8::array_t *)obj;

			s.create_table();
			for (int i{}; i < arr->length(); ++i)
			{
				const auto v = arr->get(&_unk, i)->value;
				if (!v)
					continue;

				// push the value to the stack
				if (!value_to_lua(s, v))
					continue;

				s.set_i(i + 1);
			}
		}
		else
		{
			const auto obj_names = (sdk::v8::array_t *)obj->get_property_names(&_unk)->value;
			if (!obj_names)
				return false;

			s.create_table();
			for (int i{}; i < obj_names->length(); ++i)
			{
				const auto obj_name = obj_names->get(&_unk, i)->value;
				if (!obj_name || !obj_name->is_string() && !obj_name->is_string_object())
					continue;

				const auto v = obj->get(&_unk, obj_name)->value;
				if (!v)
					continue;

				// push the value
				if (!value_to_lua(s, v))
					continue;

				sdk::v8::utf8_string_t str;
				str.ctor(obj_name);
				s.set_field(str.value);
				str.dtor();
			}
		}
	}

	return true;
}

int eval(lua_State *l)
{
	runtime_state s(l);
	if (!cfg.lua.allow_insecure.get())
	{
		s.error(XOR_STR("eval is not available with Allow insecure disabled"));
		return 0;
	}

	const auto r = s.check_arguments({{ltd::string}}, true);

	if (!r)
	{
		s.error(XOR_STR("usage: panorama.run_script(script)"));
		return 0;
	}

	const auto ui_engine = game->panorama_ui_engine->get_ui_engine();
	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("sandbox problem"));
		return 0;
	}

	const auto find_panel = [ui_engine](uint32_t id) -> sdk::ui_panel_t *
	{
		for (const auto &panel : ui_engine->panels)
		{
			if (FNV1A_CMP_IM(panel.panel->get_id(), id))
				return panel.panel;
		}

		return nullptr;
	};

	// get js context
	sdk::ui_panel_t *js_context{};
	if (s.get_stack_top() == 2 && s.is_string(2))
	{
		js_context = find_panel(util::fnv1a(s.get_string(2)));
		if (!js_context)
		{
			s.error(XOR_STR("panorama: specified panel not found"));
			return 0;
		}
	}

	if (!js_context)
		js_context = find_panel(FNV1A("CSGOJsRegistration"));

	if (!js_context || !ui_engine->is_valid_panel_ptr(js_context))
	{
		s.error(XOR_STR("panorama: unable to find panel"));
		return 0;
	}

	// run script
	sdk::v8::handle_scope_t scope;
	sdk::v8::try_catch_t try_catch;

	const auto isolate = ui_engine->get_isolate();
	isolate->enter();
	scope.enter(isolate);

	const auto ctx = sdk::v8::handle_scope_t::create_handle(
		isolate, **ui_engine->get_panel_context(js_context->get_js_context_parent()));
	ctx->enter();

	// compile in try/catch.
	try_catch.enter(isolate);
	const auto compiled =
		ui_engine->compile(js_context, tfm::format(XOR_STR("(function(){%s})()"), s.get_string(1)).c_str());
	try_catch.exit();

	if (!compiled->value)
	{
		ctx->exit();
		scope.exit();
		isolate->exit();
		return 0;
	}

	// run script.
	int ret_count{}, _unk;

	const auto val = ui_engine->run_compiled_script(&_unk, js_context, compiled->value)->value;
	if (!val)
	{
		ctx->exit();
		scope.exit();
		isolate->exit();
		return 0;
	}

	// deduct return params.
	if (value_to_lua(s, val))
		ret_count = 1;

	ctx->exit();
	scope.exit();
	isolate->exit();

	return ret_count;
}
} // namespace lua::api_def::panorama