
#include <gui/helpers.h>
#include <gui/values.h>

#include <base/cfg.h>
#include <base/game.h>
#include <hacks/skinchanger.h>
#include <menu/macros.h>
#include <menu/menu.h>
#include <sdk/input_system.h>

#include <ren/types/animated_texture.h>

#ifdef CSGO_SECURE
#include <VirtualizerSDK.h>
#include <network/helpers.h>
#endif

#include <resources/cursor.h>
#include <resources/icon_add.h>
#include <resources/icon_alert.h>
#include <resources/icon_allow_dynamic_load.h>
#include <resources/icon_allow_insecure.h>
#include <resources/icon_autoload.h>
#include <resources/icon_bug.h>
#include <resources/icon_clear.h>
#include <resources/icon_cloud.h>
#include <resources/icon_copy.h>
#include <resources/icon_delete.h>
#include <resources/icon_down.h>
#include <resources/icon_error.h>
#include <resources/icon_export.h>
#include <resources/icon_file.h>
#include <resources/icon_folder.h>
#include <resources/icon_import.h>
#include <resources/icon_legit.h>
#include <resources/icon_load.h>
#include <resources/icon_misc.h>
#include <resources/icon_paste.h>
#include <resources/icon_rage.h>
#include <resources/icon_refresh.h>
#include <resources/icon_reset.h>
#include <resources/icon_save.h>
#include <resources/icon_scripts.h>
#include <resources/icon_search.h>
#include <resources/icon_skins.h>
#include <resources/icon_success.h>
#include <resources/icon_up.h>
#include <resources/icon_visuals.h>
#include <resources/loading.h>
#include <resources/logo_head.h>
#include <resources/logo_stripes.h>
#include <resources/pattern.h>
#include <resources/pattern_group.h>

#include <gui/widgets/active_hotkeys.h>
#include <gui/widgets/bomb.h>
#include <gui/widgets/indicators.h>
#include <gui/widgets/spectators.h>

using namespace menu;
using namespace evo;
using namespace evo::gui;
using namespace ren;
using namespace evo::gui::helpers;

namespace menu::tools
{
std::shared_ptr<control> make_stacked_groupboxes(uint64_t id, const vec2 &size,
												 const std::vector<std::shared_ptr<control>> &groups)
{
	const auto l = std::make_shared<layout>(evo::gui::control_id{id, ""}, vec2(), size, s_vertical);
	l->make_not_clip()->margin = rect();

	std::shared_ptr<control> last{};
	for (const auto &c : groups)
	{
		c->margin = rect(0.f, 0.f, 0.f, 10.f);
		l->add(c);
		last = c;
	}

	last->size.y -= 2.f;
	last->margin = rect();
	return l;
}
} // namespace menu::tools

void menu_manager::create()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// gui resources
	ctx->res.general.logo_head = std::make_shared<texture>(data::logo_head, sizeof(data::logo_head));
	ctx->res.general.logo_stripes = std::make_shared<texture>(data::logo_stripes, sizeof(data::logo_stripes));
	ctx->res.general.pattern = std::make_shared<texture>(data::pattern, sizeof(data::pattern));
	ctx->res.general.pattern_group = std::make_shared<texture>(data::pattern_group, sizeof(data::pattern_group));
	ctx->res.general.cursor = std::make_shared<texture>(data::cursor, sizeof(data::cursor));
	ctx->res.general.loader = std::make_shared<texture>(data::loading, sizeof(data::loading));

	draw.textures[GUI_HASH("gui_logo_head")] = ctx->res.general.logo_head;
	draw.textures[GUI_HASH("gui_logo_stripes")] = ctx->res.general.logo_stripes;
	draw.textures[GUI_HASH("gui_pattern")] = ctx->res.general.pattern;
	draw.textures[GUI_HASH("gui_pattern_group")] = ctx->res.general.pattern_group;
	draw.textures[GUI_HASH("gui_cursor")] = ctx->res.general.cursor;
	draw.textures[GUI_HASH("gui_loading")] = ctx->res.general.loader;

	ctx->res.icons.up = std::make_shared<texture>(data::icon_up, sizeof(data::icon_up));
	ctx->res.icons.down = std::make_shared<texture>(data::icon_down, sizeof(data::icon_down));
	ctx->res.icons.notify_clear = std::make_shared<texture>(data::icon_clear, sizeof(data::icon_clear));
	ctx->res.icons.copy = std::make_shared<texture>(data::icon_copy, sizeof(data::icon_copy));
	ctx->res.icons.paste = std::make_shared<texture>(data::icon_paste, sizeof(data::icon_paste));
	ctx->res.icons.alert = std::make_shared<texture>(data::icon_alert, sizeof(data::icon_alert));
	ctx->res.icons.add = std::make_shared<texture>(data::icon_add, sizeof(data::icon_add));
	ctx->res.icons.remove = std::make_shared<texture>(data::icon_delete, sizeof(data::icon_delete));
	ctx->res.icons.search = std::make_shared<texture>(data::icon_search, sizeof(data::icon_search));
	ctx->res.icons.bug = std::make_shared<texture>(data::icon_bug, sizeof(data::icon_bug));

	draw.textures[GUI_HASH("gui_icon_up")] = ctx->res.icons.up;
	draw.textures[GUI_HASH("gui_icon_down")] = ctx->res.icons.down;
	draw.textures[GUI_HASH("gui_icon_clear")] = ctx->res.icons.notify_clear;
	draw.textures[GUI_HASH("gui_icon_copy")] = ctx->res.icons.copy;
	draw.textures[GUI_HASH("gui_icon_paste")] = ctx->res.icons.paste;
	draw.textures[GUI_HASH("gui_icon_alert")] = ctx->res.icons.alert;
	draw.textures[GUI_HASH("gui_icon_add")] = ctx->res.icons.add;
	draw.textures[GUI_HASH("gui_icon_delete")] = ctx->res.icons.remove;
	draw.textures[GUI_HASH("gui_icon_search")] = ctx->res.icons.search;
	draw.textures[GUI_HASH("gui_icon_bug")] = ctx->res.icons.bug;

	// menu resources
	draw.textures[GUI_HASH("icon_rage")] = std::make_shared<texture>(data::icon_rage, sizeof(data::icon_rage));
	draw.textures[GUI_HASH("icon_legit")] = std::make_shared<texture>(data::icon_legit, sizeof(data::icon_legit));
	draw.textures[GUI_HASH("icon_visuals")] = std::make_shared<texture>(data::icon_visuals, sizeof(data::icon_visuals));
	draw.textures[GUI_HASH("icon_misc")] = std::make_shared<texture>(data::icon_misc, sizeof(data::icon_misc));
	draw.textures[GUI_HASH("icon_scripts")] = std::make_shared<texture>(data::icon_scripts, sizeof(data::icon_scripts));
	draw.textures[GUI_HASH("icon_skins")] = std::make_shared<texture>(data::icon_skins, sizeof(data::icon_skins));
	draw.textures[GUI_HASH("icon_debug")] = ctx->res.icons.bug;

	// hack resources
	// draw.manage(GUI_HASH("icon_wep_general"), create_in_game_weapon_texture(XOR("glock")));
	draw.textures[GUI_HASH("icon_cloud")] = std::make_shared<texture>(data::icon_cloud, sizeof(data::icon_cloud));
	draw.textures[GUI_HASH("icon_file")] = std::make_shared<texture>(data::icon_file, sizeof(data::icon_file));
	draw.textures[GUI_HASH("icon_refresh")] = std::make_shared<texture>(data::icon_refresh, sizeof(data::icon_refresh));
	draw.textures[GUI_HASH("icon_folder")] = std::make_shared<texture>(data::icon_folder, sizeof(data::icon_folder));
	draw.textures[GUI_HASH("icon_load")] = std::make_shared<texture>(data::icon_load, sizeof(data::icon_load));
	draw.textures[GUI_HASH("icon_save")] = std::make_shared<texture>(data::icon_save, sizeof(data::icon_save));
	draw.textures[GUI_HASH("icon_import")] = std::make_shared<texture>(data::icon_import, sizeof(data::icon_import));
	draw.textures[GUI_HASH("icon_export")] = std::make_shared<texture>(data::icon_export, sizeof(data::icon_export));
	draw.textures[GUI_HASH("icon_reset")] = std::make_shared<texture>(data::icon_reset, sizeof(data::icon_reset));
	draw.textures[GUI_HASH("icon_autoload")] =
		std::make_shared<texture>(data::icon_autoload, sizeof(data::icon_autoload));
	draw.textures[GUI_HASH("icon_allow_insecure")] =
		std::make_shared<texture>(data::icon_allow_insecure, sizeof(data::icon_allow_insecure));
	draw.textures[GUI_HASH("icon_allow_dynamic_load")] =
		std::make_shared<texture>(data::icon_allow_dynamic_load, sizeof(data::icon_allow_dynamic_load));
	draw.textures[GUI_HASH("icon_success")] = std::make_shared<texture>(data::icon_success, sizeof(data::icon_success));
	draw.textures[GUI_HASH("icon_error")] = std::make_shared<texture>(data::icon_error, sizeof(data::icon_error));

	ctx->tick_sound_callback = []()
	{
		if (!cfg.misc.enable_tick_sound)
			return;

		EMIT_TICK_SOUND();
	};

	// user
	ctx->user.username = XOR("developer");
	ctx->user.active_until = XOR("Never");

#if defined(CSGO_SECURE) && !defined(_DEBUG)
/*	const auto ud = network::get_user_data();
	ctx->user.username = ud.username;
	ctx->user.active_until = ud.expiry;

	if (ud.avatar_size)
	{
		auto dec_avatar = network::get_decoded_avatar();
		ctx->user.avatar =
			std::make_shared<animated_texture>(reinterpret_cast<void *>(dec_avatar.data()), ud.avatar_size);
		ctx->user.avatar->create();

		draw.textures[GUI_HASH("user_avatar")] = ctx->user.avatar;
	}*/
#endif

	const auto window_sz = vec2{ 800.f, 525.f };

	const auto group_sz = window_size_to_groupbox_size(window_sz.x, window_sz.y);
	const auto group_half_sz = group_sz * vec2{1.f, .5f} - vec2{0.f, 5.f};

	const auto wnd = make_window(
		XOR("wnd_main"), { 50.f, 50.f }, window_sz,
		[](std::shared_ptr<layout> &e)
		{
			e->add(MAKE_TAB("rage", "RAGE")->make_selected());
			e->add(MAKE_TAB("visuals", "VISUALS"));
			e->add(MAKE_TAB("misc", "MISC"));
			e->add(MAKE_TAB("skins", "SKINS"));
#ifdef CSGO_LUA
			e->add(MAKE_TAB("scripts", "SCRIPTS"));
#endif
#ifndef NDEBUG
			e->add(MAKE_TAB("debug", "DEBUG"));
#endif
		},
		[&](std::shared_ptr<layout> &e)
		{
			e->add(make_tab_area(GUI_HASH("rage_container"), true, s_none, group::rage_tab));
			e->add(make_tab_area(GUI_HASH("visuals_container"), false, s_none, group::visuals_tab));

			e->add(make_tab_area(
				GUI_HASH("misc_container"), false,
				[&](std::shared_ptr<layout> &e)
				{
					e->add(make_groupbox(XOR("misc.general"), XOR("General"), group_sz, group::misc_general));
					e->add(tools::make_stacked_groupboxes(
						GUI_HASH("misc.settings_helpers"), group_sz,
						{
							make_groupbox(XOR("misc.settings"), XOR("Settings"), group_half_sz, group::misc_settings),
							make_groupbox(XOR("misc.helpers"), XOR("Helpers"), group_half_sz, group::misc_helpers),
						}));
					e->add(make_groupbox(XOR("misc.configs"), XOR("Configs"), group_sz, group::misc_configs));
				}));

			e->add(make_tab_area(GUI_HASH("skins_container"), false,
								 [&](std::shared_ptr<layout> &e)
								 {
									 e->add(make_groupbox(XOR("skins.select"), XOR("Select"), group_sz,
														  group::skinchanger_select));
									 e->add(make_groupbox(XOR("skins.settings"), XOR("Settings"), group_sz,
														  group::skinchanger_settings));
									 // TODO: e->add(make_groupbox(XOR("skins.preview"), XOR("Preview"), group_sz,
									 // group::skinchanger_preview));
								 }));

#ifdef CSGO_LUA
			e->add(make_tab_area(GUI_HASH("scripts_container"), false,
								 [&](std::shared_ptr<layout> &e)
								 {
									 e->add(make_groupbox(XOR("scripts.general"), XOR("General"), group_sz,
														  group::scripts_general));
									 e->add(make_groupbox(XOR("scripts.elements_a"), XOR("Elements A"), group_sz,
														  group::scripts_elements));
									 e->add(make_groupbox(XOR("scripts.elements_b"), XOR("Elements B"), group_sz,
														  group::scripts_elements));
								 }));
#endif
#ifndef NDEBUG
			e->add(make_tab_area(
				GUI_HASH("debug_container"), false,
				[&](std::shared_ptr<layout> &e)
				{ e->add(make_groupbox(XOR("debug.general"), XOR("General"), group_sz, group::debug_general)); }));
#endif
		});

	wnd->is_visible = wnd->is_taking_input = ctx->should_render_cursor = false;
	ctx->add(wnd);
	main_wnd = wnd;

	// debug.enabled = true;

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void menu_manager::toggle()
{
	if (const auto wnd = main_wnd.lock(); wnd && can_toggle)
	{
		wnd->is_visible = !wnd->is_visible;
		wnd->is_taking_input = wnd->is_visible;

		if (!wnd->is_visible)
		{
			ctx->active = nullptr;
			for (const auto &p : ctx->active_popups)
				p->close();
		}

		ctx->should_render_cursor = wnd->is_visible;

		const auto hotkeys = ctx->find(::gui::widgets::active_hotkeys_id);
		if (hotkeys)
		{
			const auto wdg = hotkeys->as<widget>();
			wdg->is_forced_visibility = wnd->is_visible;
			wdg->is_taking_input = wnd->is_visible;
		}

		const auto indicators = ctx->find(::gui::widgets::indicators_id);
		if (indicators)
		{
			const auto wdg = indicators->as<widget>();
			wdg->is_forced_visibility = wnd->is_visible;
			wdg->is_taking_input = wnd->is_visible;
		}

		const auto spectators = ctx->find(::gui::widgets::spectators_id);
		if (spectators)
		{
			const auto wdg = spectators->as<widget>();
			wdg->is_forced_visibility = wnd->is_visible;
			wdg->is_taking_input = wnd->is_visible;
		}

		const auto bomb = ctx->find(::gui::widgets::bomb_id);
		if (bomb)
		{
			const auto wdg = bomb->as<widget>();
			wdg->is_forced_visibility = wnd->is_visible;
			wdg->is_taking_input = wnd->is_visible;
		}

		if (!cfg.misc.menu_movement.get())
			game->input_system->reset_input_state();
	}
}

void menu_manager::finalize()
{
	if (did_finalize)
		return;

	// add gui stuff
	cfg.entries[GUI_HASH("gui.debug")] = &ctx->show_ids;
	cfg.entries[GUI_HASH("gui.blur_quality")] = &ctx->blur_quality;

#if defined(CSGO_SECURE) && !defined(_DEBUG)
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// force reset gui!
	ctx->reset();

#ifndef CSGO_SECURE
#ifdef CSGO_LUA
	// refresh available scripts
	if (const auto b = ctx->find<button>(GUI_HASH("scripts.general.refresh")); b)
		b->callback();
#endif

	// refresh available configs
	if (const auto b = ctx->find<button>(GUI_HASH("misc.configs.refresh")); b)
		b->callback();
#endif

	hacks::skin_changer->fill_weapon_list();

	did_finalize = true;

#if defined(CSGO_SECURE) && !defined(_DEBUG)
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

bool menu_manager::is_open()
{
	const auto wnd = main_wnd.lock();
	return wnd && wnd->is_visible;
}