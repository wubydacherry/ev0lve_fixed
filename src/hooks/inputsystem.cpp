
#include <base/cfg.h>
#include <base/hook_manager.h>
#include <menu/menu.h>
#include <sdk/client_mode.h>
#include <sdk/input_system.h>
#include <sdk/misc.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif /* CSGO_LUA */

namespace hooks::inputsystem
{
LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	if (!evo::gui::ctx)
		return hook_manager.wnd_proc->call(wnd, msg, w_param, l_param);

	// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/inputsystem/inputsystem.cpp#L1604
	// using this to replace enable_input and reset_input_state, didn't find better way to handle menu movement
	auto &input_enable = *(bool *)(*(uintptr_t *)game->inputsystem.at(sdk::globals::input_enable));

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_input"),
					  [&](lua::state &s) -> int
					  {
						  s.push(static_cast<int>(msg));
						  s.push(static_cast<int>(w_param));
						  s.push(static_cast<int>(l_param));

						  return 3;
					  });
#endif

	const auto handle_gui_input = [&]()
	{
		if (!evo::gui::ctx->active && msg == XOR_32(WM_KEYDOWN))
		{
			if (w_param == XOR_32(VK_DELETE) || w_param == XOR_32(VK_INSERT))
			{
				menu::menu.toggle();
				return true;
			}

			if (w_param == XOR_32(VK_ESCAPE) && menu::menu.is_open())
			{
				if (!evo::gui::ctx->active_popups.empty())
					evo::gui::ctx->active_popups.back()->close();
				else
					menu::menu.toggle();

				return true;
			}
		}

		const auto is_menu_open = menu::menu.is_open();
		const auto chat = (sdk::csgo_hud_chat_t *)sdk::find_hud_element(XOR_STR("CCSGO_HudChat"));

		evo::gui::ctx->should_process_hotkeys =
			!((game->game_console->is_console_visible() || chat && chat->is_open) && !is_menu_open);
		return evo::gui::ctx->refresh(msg, w_param, l_param) && is_menu_open;
	};

	input_enable = !handle_gui_input();

	// set cursor to none since we render our own
	if (menu::menu.is_open())
		SetCursor(nullptr);

	// handle menu movement
	if (cfg.misc.menu_movement.get() && (msg == XOR_32(WM_KEYDOWN) || msg == XOR_32(WM_KEYUP)) && menu::menu.is_open())
	{
		if (evo::gui::ctx->active && evo::gui::ctx->active->is_taking_text)
			return hook_manager.wnd_proc->call(wnd, msg, w_param, l_param);

		const auto button_code =
			((int(__stdcall *)(int))game->inputsystem.at(sdk::functions::input_system::vk_to_button_code))(w_param);
		if (const auto binding = game->engine_client->binding_for_key(button_code); binding)
		{
			switch (util::fnv1a(binding))
			{
			case FNV1A("+forward"):
			case FNV1A("+back"):
			case FNV1A("+moveleft"):
			case FNV1A("+moveright"):
			case FNV1A("+jump"):
			case FNV1A("+duck"):
			case FNV1A("+walk"):
			case FNV1A("+lookatweapon"):
			case FNV1A("lastinv"):
				input_enable = true;
				break;
			}

			// also support slot here
			if (std::string(binding).starts_with(XOR_STR("slot")))
				input_enable = true;
		}
	}

	return hook_manager.wnd_proc->call(wnd, msg, w_param, l_param);
}
} // namespace hooks::inputsystem
