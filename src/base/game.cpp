
#include <base/cfg.h>
#include <base/game.h>
#include <base/hook_manager.h>
#include <chrono>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/dispatch_queue.h>
#include <detail/dx_adapter.h>
#include <detail/entity_rendering.h>
#include <hacks/skinchanger.h>
#include <ren/renderer.h>
#include <sdk/bitbuf.h>
#include <sdk/input_system.h>
#include <thread>
#include <util/anti_debug.h>
#include <util/memory.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif /* CSGO_LUA */

#ifdef CSGO_SECURE
#include <VirtualizerSDK.h>
#include <network/app_hack.h>
#include <network/helpers.h>
#endif /* CSGO_SECURE */

using namespace std::chrono_literals;
using namespace sdk;
using namespace util;
using namespace detail;

#ifdef CSGO_BETA
#include <base/crash_handler.h>

LONG __stdcall exception_handler(EXCEPTION_POINTERS *ex)
{
	// continue execution on useless exceptions.
	if (ex->ExceptionRecord->ExceptionCode <= 0x80000000 || ex->ExceptionRecord->ExceptionCode == 0xE06D7363 ||
		ex->ExceptionRecord->ExceptionCode == 0xE24C4A02 || ex->ExceptionRecord->ExceptionCode == 0xE24C4A03)
		return EXCEPTION_CONTINUE_SEARCH;

	// remove exception handler so it doesn't bug us again.
	RemoveVectoredExceptionHandler(game->exception_handler_handle);

	// handle the crash.
	crash_handler_t crash_handler;
	crash_handler.ex = ex;
	crash_handler.trace();

	// notify the user.
	MessageBoxA(nullptr, XOR_STR("Game crashed, check the log for more information."), XOR_STR("Critical error"),
				XOR_32(MB_TOPMOST | MB_ICONERROR));

	// exit the process.
	exit(ex->ExceptionRecord->ExceptionCode);
	return ex->ExceptionRecord->ExceptionCode;
}

void ex_terminate_handler()
{
	try
	{
		std::rethrow_exception(std::current_exception());
	}
	catch (const std::exception &ex)
	{
		// handle the crash.
		crash_handler_t crash_handler;
		crash_handler.std_ex = ex;
		crash_handler.trace();

		// notify the user.
		MessageBoxA(nullptr, XOR_STR("Game crashed, check the log for more information."), XOR_STR("Critical error"),
					XOR_32(MB_TOPMOST | MB_ICONERROR));
	}

	std::abort();
}
#endif /* CSGO_BETA */

game_t::game_t(uintptr_t base, uint32_t reserved)
	: base(base),
	  /* game modules */
	  client(LOOKUP_MODULE("client.dll")), engine(LOOKUP_MODULE("engine.dll")), server(LOOKUP_MODULE("server.dll")),
	  gameoverlayrenderer(LOOKUP_MODULE("gameoverlayrenderer.dll")),
	  vguimatsurface(LOOKUP_MODULE("vguimatsurface.dll")), inputsystem(LOOKUP_MODULE("inputsystem.dll")),
	  tier0(LOOKUP_MODULE("tier0.dll")), vstdlib(LOOKUP_MODULE("vstdlib.dll")),
	  datacache(LOOKUP_MODULE("datacache.dll")), local(LOOKUP_MODULE("localize.dll")),
	  vphysics(LOOKUP_MODULE("vphysics.dll")), panorama(LOOKUP_MODULE("panorama.dll")),
	  mat(LOOKUP_MODULE("materialsystem.dll")), shaderapidx9(LOOKUP_MODULE("shaderapidx9.dll")),
	  v8(LOOKUP_MODULE("v8.dll")), studiorender(LOOKUP_MODULE("studiorender.dll"))
{
#ifdef CSGO_BETA
	exception_handler_handle = AddVectoredExceptionHandler(1, exception_handler);
	std::set_terminate(ex_terminate_handler);
#endif /* CSGO_BETA */

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
	util::wipe_crt_initalize_table();
#endif /* CSGO_SECURE */

#if defined(CSGO_SECURE) && defined(NDEBUG)
	network::session = reinterpret_cast<const char *>(reserved);
	evo::app = std::make_shared<network::app_hack>();
	NETWORK()->start();

	while (!NETWORK()->is_verified)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		std::this_thread::yield();
	}
#endif /* CSGO_SECURE */

	sdk::init_masks();
	cfg.init();

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif /* CSGO_SECURE */
}

void game_t::init()
{
	// we dont need it now - maj12;
	/*if (*(uintptr_t *)engine.at(globals::build_offset) != XOR_32(globals::build))
	{
		MessageBox(nullptr, XOR_STR("Cheat outdated, await update."), XOR_STR("Critical error"), MB_ICONERROR);
		RUNTIME_ERROR("Cheat outdated.");
	}*/

	/* grab our favorite interfaces */
	engine_client = encrypted_ptr<engine_client_t>(engine.at(interfaces::vengine_client014));
	hl_client = encrypted_ptr<hl_client_t>(client.at(interfaces::vclient018));
	client_entity_list = encrypted_ptr<client_entity_list_t>(client.at(interfaces::vclient_entity_list003));
	cvar = encrypted_ptr<cvar_t>(vstdlib.at(interfaces::vengine_cvar007));
	cs_game_movement = encrypted_ptr<cs_game_movement_t>(client.at(interfaces::game_movement001));
	mdl_cache = encrypted_ptr<mdl_cache_t>(datacache.at(interfaces::mdlcache004));
	localize = encrypted_ptr<localize_t>(local.at(interfaces::localize_001));
	prediction = encrypted_ptr<prediction_t>(client.at(interfaces::vclient_prediction001));
	engine_trace = encrypted_ptr<engine_trace_t>(engine.at(interfaces::engine_trace_client004));
	surface_props = encrypted_ptr<surface_props_t>(vphysics.at(interfaces::vphysics_surface_props001));
	model_info_client = encrypted_ptr<model_info_client_t>(engine.at(interfaces::vmodel_info_client004));
	debug_overlay = encrypted_ptr<debug_overlay_t>(engine.at(interfaces::vdebug_overlay004));
	render_view = encrypted_ptr<render_view_t>(engine.at(interfaces::vengine_render_view014));
	surface = encrypted_ptr<mat_surface_t>(vguimatsurface.at(interfaces::vgui_surface031));
	model_render = encrypted_ptr<model_render_t>(engine.at(interfaces::vengine_model016));
	material_system = encrypted_ptr<material_system_t>(mat.at(interfaces::vmaterial_system080));
	string_table = encrypted_ptr<string_table_t>(engine.at(interfaces::vengine_client_string_table001));
	hardware_config = encrypted_ptr<hardware_config_t>(
		((uintptr_t(*)())mat.at(0x164F0))()); //interfaces::material_system_hardware_config013_callback
	input_system = encrypted_ptr<input_system_t>(inputsystem.at(interfaces::input_system_version001));
	game_event_manager = encrypted_ptr<game_event_manager_t>(engine.at(interfaces::gameeventsmanager002));
	panorama_ui_engine = encrypted_ptr<panorama_ui_engine_t>(panorama.at(interfaces::panorama_uiengine001));
	studio_render = encrypted_ptr<studio_render_t>(studiorender.at(interfaces::vstudio_render026));
	game_console = encrypted_ptr<game_console_t>(client.at(interfaces::game_console004));

	/* grab the rest of the objects */
	globals = encrypted_ptr<global_vars_base_t>(engine.at(globals::global_vars_base));
	mem_alloc = encrypted_ptr<mem_alloc_t>(tier0.at(0x4FA64)).deref(1); //globals::mem_alloc
	cs_player_resource = encrypted_ptr<cs_player_resource_t>(client.at(globals::player_resource));
	client_state = encrypted_ptr<client_state_t>(engine.at(globals::client_state)).deref(1);
	input = encrypted_ptr<input_t>(client.at(0x52627B0)); //globals::input
	weapon_system = encrypted_ptr<weapon_system_t>(client.at(globals::weapon_system));
	rules = encrypted_ptr<cs_game_rules_t>(client.at(globals::game_rules));
	view_render = encrypted_ptr<view_render_t>(client.at(0x5344950)).deref(1); //ox0x5344950
	glow_manager = encrypted_ptr<glow_manager_t>(client.at(globals::glow_manager));
	leaf_system = encrypted_ptr<leaf_system_t>(client.at(globals::leaf_system));
	key_values_system =
		encrypted_ptr<key_values_system_t>(reinterpret_cast<uintptr_t (*)()>(vstdlib.at(globals::key_values_system))());
	modifier_table = encrypted_ptr<modifier_table_t>(client.at(globals::modifier_table));
	model_loader = encrypted_ptr<model_loader_t>(engine.at(0x785860)); //model_loader
	steam_context = encrypted_ptr<steam_context_t>(engine.at(0x904C6)).deref(1); //globals::steam_context

	/* halt here until game has loaded. */
	while (!client_mode)
		client_mode = encrypted_ptr<client_mode_t>(
			**reinterpret_cast<uintptr_t **>((*reinterpret_cast<uintptr_t **>(hl_client()))[10] + 5));

	/* start our dispatch queue. */
	dispatch.spawn();

	/* initialize some stuff in the cheat. */
	aim_helper::build_seed_table();
	hacks::skin_changer->parse_game_items();
	hacks::skin_changer->parse_weapon_names();

	/* finish final init. */
	hooks::hl_client::level_init_pre_entity(nullptr, 0, nullptr);
	hacks::skin_changer->load_skins();
	ent_ren.init();

	/* IMPORTANT: never put any initializer code *AFTER* the hook_manager dispatch. */

	/* run the hook manager. */
	hook_manager.init();
	hook_manager.attach();
}

void game_t::deinit()
{
	// oh well...
	std::this_thread::sleep_for(100ms);

	// remove all hooks.
	hook_manager.detach();

	// oh well...
	std::this_thread::sleep_for(100ms);

	// kill the renderer.
	dx_adapter::create_objects();
	evo::ren::draw.destroy_objects();
	evo::ren::draw = {};

#ifdef CSGO_LUA
	// unload all scripts
	lua::api.stop_all();
#endif

#if defined(CSGO_SECURE) && defined(NDEBUG)
	// stop services
	network::disconnect();
#endif

	// get rid of our maps.
	hacks::misc::on_exit();
	client_state->request_full_update();

	// free textures.
	ent_ren.on_exit();

	/* dispose dispatch queue. */
	dispatch.decomission();

	/* allow window events on exit. */
	input_system->enable_input(true);
	input_system->reset_input_state();

	// kill off remains.
	FreeLibrary(HMODULE(base));
	FreeLibraryAndExitThread(HMODULE(base), 0);
}

std::unique_ptr<game_t> game;
