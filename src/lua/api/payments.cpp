#include <gui/gui.h>
#include <gui/popups/payment_popup.h>
#include <lua/api_def.h>
#include <lua/helpers.h>

namespace lua::api_def::payments
{
using namespace evo::gui;

int restore(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::function}}))
	{
		s.error(XOR_STR("usage: payments.restore(callback(result))"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("sandbox gone"));
		return 0;
	}

#if 0
	const auto fn = s.registry_add();
	std::thread(
		[me, fn]()
		{
			const auto result = network::get_purchased_items(me->remote.id);
			auto &s = me->l;
			s.registry_get(fn);
			s.create_table();

			int i{1};
			for (const auto &item : result)
				s.set_i(i++, item.c_str());

			if (!s.call(1, 0))
			{
				me->did_error = true;
				lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
				if (const auto f = api.find_script_file(me->id); f.has_value())
					f->get().should_unload = true;

				return;
			}
		})
		.detach();
#endif

	return 0;
}

int create(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::function}}))
	{
		s.error(XOR_STR("usage: payments.create(id, callback(result))"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("sandbox gone"));
		return 0;
	}

	const auto fn = s.registry_add();

	const auto p = std::make_shared<::gui::popups::payment_popup>(evo::gui::control_id{GUI_HASH("ev0lve.payment")},
																  s.get_string(1), me->name, me->remote.id);
	p->open();
	p->on_close = [me, fn](bool res)
	{
		auto &s = me->l;
		s.registry_get(fn);
		s.push(res);
		if (!s.call(1, 0))
		{
			me->did_error = true;
			lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
			if (const auto f = api.find_script_file(me->id); f.has_value())
				f->get().should_unload = true;

			return;
		}
	};

	return 0;
}
} // namespace lua::api_def::payments
