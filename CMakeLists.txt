
cmake_minimum_required(VERSION 3.16)
project(csgo_internal)

set(CMAKE_CXX_STANDARD 23)

# compile options.
add_compile_options(-m32
	-march=x86-64-v2
	-mno-sse4a
	-fno-rtti
	-Xclang -fno-rtti-data
	-fno-omit-frame-pointer
	-mno-omit-leaf-frame-pointer
	-Wno-comment
	-Wno-new-returns-null
	-Wno-unused-result
	-Wno-macro-redefined
	-Wno-pch-date-time)
add_compile_definitions(_HAS_ITERATOR_DEBUGGING=0
	_SECURE_SCL=0
	_HAS_STATIC_RTTI=0
	_X86_
	_WINDOWS
	_CRT_SECURE_NO_WARNINGS
	WIN32)

if (CMAKE_BUILD_TYPE MATCHES Debug)
#	add_compile_options(-gdwarf)
#	add_link_options(-Xlinker -debug:dwarf)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif (CMAKE_BUILD_TYPE MATCHES Debug)

if (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	add_compile_options(-Ofast)
	add_link_options(LINKER:/pdbaltpath:wntdll.pdb)
endif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)

# force json cpp to C++14 so it doesn't require RTTI bullshit as it's disabled via compiler flag
add_compile_definitions(JSON_HAS_CPP_14 JSON_HAS_CPP_11)

include_directories(contrib)
include_directories(contrib/nlohmann/include)
include_directories(contrib/gui/include)
include_directories(contrib/gui/deps/renderer/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(src)
include_directories($ENV{DXSDK_DIR}/Include)

link_directories($ENV{DXSDK_DIR}/Lib/x86)

set(CSGO_FILES
	# base files
	src/base/game.cpp
	src/base/draw_manager.cpp
	src/base/hook_manager.cpp
	src/base/cfg.cpp
	src/base/event_log.cpp
	src/base/debug_overlay.cpp
	# additional gui files
	src/gui/selectable_script.cpp
	src/gui/selectable_config.cpp
	src/gui/public_software_item.cpp
	src/gui/preview_model.cpp
	src/gui/popups/about_popup.cpp
	src/gui/popups/public_software_popup.cpp
	src/gui/widgets/active_hotkeys.cpp
	src/gui/widgets/indicators.cpp
	src/gui/widgets/spectators.cpp
	src/gui/widgets/bomb.cpp
	# menu files
	src/menu/menu.cpp
	src/menu/misc.cpp
	src/menu/rage.cpp
	src/menu/visuals.cpp
	src/menu/scripts.cpp
	src/menu/skinchanger.cpp
	src/menu/debug.cpp
	src/menu/effects/snow_effect.cpp
	# detail files
	src/detail/player_list.cpp
	src/detail/aim_helper.cpp
	src/detail/custom_prediction.cpp
	src/detail/custom_tracing.cpp
	src/detail/dispatch_queue.cpp
	src/detail/events.cpp
	src/detail/shot_tracker.cpp
	src/detail/dx_adapter.cpp
	src/detail/entity_rendering.cpp
	# hack files
	src/hacks/advertisement.cpp
	src/hacks/antiaim.cpp
	src/hacks/chams.cpp
	src/hacks/esp.cpp
	src/hacks/misc.cpp
	src/hacks/movement.cpp
	src/hacks/rage.cpp
	src/hacks/peek_assistant.cpp
	src/hacks/skinchanger.cpp
	src/hacks/tickbase.cpp
	src/hacks/visuals.cpp
	src/hacks/grenade_prediction.cpp
	# hook files
	src/hooks/entity.cpp
	src/hooks/hl_client.cpp
	src/hooks/leaf_system.cpp
	src/hooks/prediction.cpp
	src/hooks/client_mode.cpp
	src/hooks/client_state.cpp
	src/hooks/engine_client.cpp
	src/hooks/game_movement.cpp
	src/hooks/mat_system_surface.cpp
	src/hooks/miscellaneous.cpp
	src/hooks/cs_player.cpp
	src/hooks/cs_grenade_projectile.cpp
	src/hooks/weapon.cpp
	src/hooks/recv_proxies.cpp
	src/hooks/model_render.cpp
	src/hooks/trace_filter_for_player_head_collision.cpp
	src/hooks/steam.cpp
	src/hooks/inputsystem.cpp
	# util files
	src/util/fnv1a.cpp
	src/util/memory.cpp
	# entry point
	src/main.cpp)

if (CSGO_BETA)
	set(CSGO_FILES ${CSGO_FILES} src/base/crash_handler.cpp)

	add_compile_definitions(CSGO_BETA)
	link_libraries(dbghelp)
endif()


	add_compile_definitions(CSGO_LUA)
	link_directories(${CMAKE_CURRENT_BINARY_DIR}/contrib/luajit/src)
	include_directories(contrib/luajit/src)
	link_libraries(luajit wininet)
	add_link_options(-Wl,-map)

	set(CSGO_FILES ${CSGO_FILES}
		src/lua/state.cpp
		src/lua/runtime_state.cpp
		src/lua/engine.cpp
		src/lua/script.cpp
		src/lua/api/globals.cpp
		src/lua/api/event_log.cpp
		src/lua/api/render.cpp
		src/lua/api/utils.cpp
		src/lua/api/gui.cpp
		src/lua/api/input.cpp
		src/lua/api/engine.cpp
		src/lua/api/cvar.cpp
		src/lua/api/event.cpp
		src/lua/api/user_cmd.cpp
		src/lua/api/entity.cpp
		src/lua/api/math.cpp
		src/lua/api/panorama.cpp
		src/lua/api/fs.cpp
		src/lua/api/net_prop.cpp
		src/lua/helpers.cpp
		src/util/cvar_lexer.cpp
		src/lua/api/timer.cpp
		src/lua/api/database.cpp
		src/lua/api/mat.cpp
	)

	if (CSGO_SECURE)
		set(CSGO_FILES ${CSGO_FILES}
			src/lua/api/zip.cpp
			src/lua/api/payments.cpp
			src/lua/api/net.cpp
		)
	endif()



set(REN_USE_DX9 ON)
add_compile_definitions(REN_USE_DX9 EVO_GUI_XOR="util/value_obfuscation.h" EV0_GUI_CSGO)
add_subdirectory (contrib/gui)

add_library(csgo_internal SHARED ${CSGO_FILES})

if (CSGO_LUA)
	add_dependencies(csgo_internal libluajit)
endif()

target_link_libraries(csgo_internal PRIVATE renderer gui d3d9 d3dx9)
target_precompile_headers(csgo_internal PRIVATE src/stdafx.h)
