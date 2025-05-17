#include <base/cfg.h>
#include <base/event_log.h>
#include <gui/gui.h>
#include <lua/api_def.h>
#include <lua/engine.h>
#include <lua/helpers.h>
#include <sdk/model_render.h>

#ifdef CSGO_SECURE
#include <VirtualizerSDK.h>
#include <network/helpers.h>
#endif

using namespace evo::ren;

bool lua::script::initialize()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// set panic function
	l.set_panic_func(api_def::panic);

	// hide poor globals first
	hide_globals();

	// register global functions
	register_globals();

	// register namespaces
	register_namespaces();

	// register types
	register_types();

	// attempt loading file
	if (remote.is_proprietary)
	{
#ifndef CSGO_SECURE
		RUNTIME_ERROR("Remote scripts are not supported in this build!");
#else
		if (!l.load_string(network::decrypt_script(file, remote.id, remote.last_update)))
		{
			lua::helpers::error(XOR_STR("syntax_error"), l.get_last_error_description().c_str());
			return false;
		}

		return true;
#endif
	}

	if (!l.load_file(file.c_str()))
	{
		lua::helpers::error(XOR_STR("syntax_error"), l.get_last_error_description().c_str());
		return false;
	}

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif

	return true;
}

void lua::script::unload()
{
	using namespace evo::gui;

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// resolve parent and erase element, freeing it
	for (const auto &e : gui_elements)
	{
		// remove from parent
		if (const auto &c = ctx->find(e))
		{
			if (const auto p = c->parent.lock(); p && p->is_container)
				p->as<control_container>()->remove(e);
			else
				ctx->remove(e);
		}

		// remove from hotkey list
		if (ctx->hotkey_list.find(e) != ctx->hotkey_list.end())
			ctx->hotkey_list.erase(e);
	}

	// erase callbacks
	for (const auto &e : gui_callbacks)
	{
		if (const auto &c = ctx->find(e))
		{
			if (c->universal_callbacks.find(id) != c->universal_callbacks.end())
				c->universal_callbacks[id].clear();
		}
	}

	for (const auto &f : fonts)
	{
		if (const auto &e = draw.fonts[f]; e)
		{
			e->destroy();
			draw.fonts[f] = nullptr;
		}

		api.font.on_free(f);
	}

	for (const auto &t : textures)
	{
		if (const auto &e = draw.textures[t]; e)
		{
			e->destroy();
			draw.textures[t] = nullptr;
		}

		api.texture.on_free(t);
	}

	for (const auto &t : shaders)
	{
		if (const auto &e = draw.shaders[t]; e)
		{
			e->destroy();
			draw.shaders[t] = nullptr;
		}

		api.shader.on_free(t);
	}

	for (const auto &a : animators)
	{
		if (api.anims[a])
			api.anims[a] = nullptr;

		api.animator.on_free(a);
	}

#ifdef CSGO_SECURE
	for (const auto &n : net_ids)
		network::stop_listen(n);
#endif

	if (helpers::timers.contains(id))
		helpers::timers[id].clear();

	fonts.clear();
	textures.clear();
	gui_elements.clear();
	net_ids.clear();

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

bool lua::script::call_main()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// taking off!
	if (!l.run())
	{
		lua::helpers::error(XOR_STR("runtime_error"), l.get_last_error_description().c_str());
		return false;
	}

	// find all forwards
	find_forwards();

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
	return true;
}

void lua::script::find_forwards()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	// declare list of all possible forwards
	const std::vector<std::string> forward_list = {
		XOR_STR("on_paint"),
		XOR_STR("on_frame_stage_notify"),
		XOR_STR("on_setup_move"),
		XOR_STR("on_run_command"),
		XOR_STR("on_create_move"),
		XOR_STR("on_level_init"),
		XOR_STR("on_do_post_screen_space_events"),
		XOR_STR("on_input"),
		XOR_STR("on_game_event"),
		XOR_STR("on_shutdown"),
		XOR_STR("on_config_load"),
		XOR_STR("on_config_save"),
		XOR_STR("on_shot_fired"),
		XOR_STR("on_paint_esp"),
		XOR_STR("on_draw_model_execute"),
		XOR_STR("on_net_data"),
	};

	// iterate through the list and check if we find any functions
	for (const auto &f : forward_list)
	{
		l.get_global(f.c_str());
		if (l.is_function(-1))
			forwards[util::fnv1a(f.c_str())] = f;

		l.pop(1);
	}

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void lua::script::create_forward(const char *n) { forwards[util::fnv1a(n)] = n; }

bool lua::script::has_forward(uint32_t hash) { return forwards.find(hash) != forwards.end(); }

void lua::script::call_forward(uint32_t hash, const std::function<int(state &)> &arg_callback, int ret,
							   const std::function<void(state &)> &callback)
{
	// check if forward exists
	if (!is_running || !has_forward(hash))
		return;

	// call the function
	l.get_global(forwards[hash].c_str());
	if (l.is_nil(-1))
	{
		l.pop(1);
		return;
	}

	if (!l.call(arg_callback ? arg_callback(l) : 0, ret))
	{
		did_error = true;
		lua::helpers::error(XOR_STR("runtime_error"), l.get_last_error_description().c_str());
		if (const auto f = api.find_script_file(id); f.has_value())
			f->get().should_unload = true;

		return;
	}

	if (ret)
	{
		if (l.get_stack_top() < ret)
		{
			did_error = true;
			lua::helpers::error(XOR_STR("runtime_error"), XOR_STR("Not enough return values."));
			if (const auto f = api.find_script_file(id); f.has_value())
				f->get().should_unload = true;

			return;
		}

		if (callback)
			callback(l);

		l.pop(l.get_stack_top());
	}
}

void lua::script::hide_globals()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	l.unset_global(XOR_STR("getfenv")); // allows getting environment table

	l.unset_global(XOR_STR("loadfile"));
	if (!cfg.lua.allow_dynamic_load.get())
	{
		l.unset_global(XOR_STR("load"));	   // allows direct load
		l.unset_global(XOR_STR("loadstring")); // allows direct string load
		l.unset_global(XOR_STR("dofile"));	   // same as loadfile but also executes it
	}

	l.unset_global(XOR_STR("gcinfo"));		   // garbage collector info
	l.unset_global(XOR_STR("collectgarbage")); // forces GC to collect
	l.unset_global(XOR_STR("newproxy"));	   // proxy functions
	l.unset_global(XOR_STR("coroutine"));	   // allows managing coroutines
	l.unset_global(XOR_STR("setfenv"));		   // allows replacing environment table
	l.unset_global(XOR_STR("_G"));			   // disable global table loop

	l.unset_in_table(XOR_STR("ffi"), XOR_STR("C"));	   // useless without load()
	l.unset_in_table(XOR_STR("ffi"), XOR_STR("load")); // allows loading DLLs
	l.unset_in_table(XOR_STR("ffi"), XOR_STR("gc"));   // forces GC to collect
	l.unset_in_table(XOR_STR("ffi"), XOR_STR("fill")); // memory setting

	l.unset_in_table(XOR_STR("string"), XOR_STR("dump")); // useless without load()

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void lua::script::register_namespaces()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	l.create_namespace(XOR_STR("event_log"), {
												 {XOR_STR("add"), api_def::event_log::add},
												 {XOR_STR("output"), api_def::event_log::output},
											 });

	l.set_integers_for_table(XOR_STR("event_log"), {{XOR_STR("white"), 1},
													{XOR_STR("red"), 2},
													{XOR_STR("purple"), 3},
													{XOR_STR("green"), 4},
													{XOR_STR("light_green"), 5},
													{XOR_STR("lime"), 6},
													{XOR_STR("light_red"), 7},
													{XOR_STR("gray"), 8},
													{XOR_STR("light_yellow"), 9},
													{XOR_STR("sky"), 10},
													{XOR_STR("light_blue"), 11},
													{XOR_STR("blue"), 12},
													{XOR_STR("gray_blue"), 13},
													{XOR_STR("pink"), 14},
													{XOR_STR("soft_red"), 15},
													{XOR_STR("yellow"), 16}});

	l.create_namespace(XOR_STR("render"),
					   {
						   {XOR_STR("color"), api_def::render::color},
						   {XOR_STR("rect_filled"), api_def::render::rect_filled},
						   {XOR_STR("rect"), api_def::render::rect},
						   {XOR_STR("rect_filled_rounded"), api_def::render::rect_filled_rounded},
						   {XOR_STR("text"), api_def::render::text},
						   {XOR_STR("get_screen_size"), api_def::render::get_screen_size},
						   {XOR_STR("create_font"), api_def::render::create_font},
						   {XOR_STR("create_font_gdi"), api_def::render::create_font_gdi},
						   {XOR_STR("get_text_size"), api_def::render::get_text_size},
						   {XOR_STR("circle"), api_def::render::circle},
						   {XOR_STR("circle_filled"), api_def::render::circle_filled},
						   {XOR_STR("create_texture"), api_def::render::create_texture},
						   {XOR_STR("create_texture_bytes"), api_def::render::create_texture_bytes},
						   {XOR_STR("create_texture_rgba"), api_def::render::create_texture_rgba},
						   {XOR_STR("set_texture"), api_def::render::set_texture},
						   {XOR_STR("push_texture"), api_def::render::set_texture},
						   {XOR_STR("pop_texture"), api_def::render::pop_texture},
						   {XOR_STR("get_texture_size"), api_def::render::get_texture_size},
						   {XOR_STR("create_animator_float"), api_def::render::create_animator_float},
						   {XOR_STR("create_animator_color"), api_def::render::create_animator_color},
						   {XOR_STR("push_clip_rect"), api_def::render::push_clip_rect},
						   {XOR_STR("pop_clip_rect"), api_def::render::pop_clip_rect},
						   {XOR_STR("set_uv"), api_def::render::set_uv},
						   {XOR_STR("push_uv"), api_def::render::set_uv},
						   {XOR_STR("pop_uv"), api_def::render::pop_uv},
						   {XOR_STR("rect_filled_multicolor"), api_def::render::rect_filled_multicolor},
						   {XOR_STR("line"), api_def::render::line},
						   {XOR_STR("triangle"), api_def::render::triangle},
						   {XOR_STR("triangle_filled"), api_def::render::triangle_filled},
						   {XOR_STR("triangle_filled_multicolor"), api_def::render::triangle_filled_multicolor},
						   {XOR_STR("create_texture_svg"), api_def::render::create_texture_svg},
						   {XOR_STR("create_texture_stream"), api_def::render::create_texture_stream},
						   {XOR_STR("create_font_stream"), api_def::render::create_font_stream},
						   {XOR_STR("set_alpha"), api_def::render::set_alpha},
						   {XOR_STR("get_alpha"), api_def::render::get_alpha},
						   {XOR_STR("push_alpha"), api_def::render::set_alpha},
						   {XOR_STR("pop_alpha"), api_def::render::pop_alpha},
						   {XOR_STR("blur"), api_def::render::blur},
						   {XOR_STR("create_shader"), api_def::render::create_shader},
						   {XOR_STR("set_shader"), api_def::render::set_shader},
						   {XOR_STR("wrap_text"), api_def::render::wrap_text},
						   {XOR_STR("set_rotation"), api_def::render::set_rotation},
						   {XOR_STR("set_frame"), api_def::render::set_frame},
						   {XOR_STR("set_loop"), api_def::render::set_loop},
						   {XOR_STR("reset_loop"), api_def::render::reset_loop},
						   {XOR_STR("get_frame_count"), api_def::render::get_frame_count},
						   {XOR_STR("rect_rounded"), api_def::render::rect_rounded},
						   {XOR_STR("circle_filled_multicolor"), api_def::render::circle_filled_multicolor},
						   {XOR_STR("set_anti_aliasing"), api_def::render::set_anti_aliasing},
						   {XOR_STR("set_render_mode"), api_def::render::set_render_mode},
					   });

	l.set_integers_for_table(XOR_STR("render"), {// alignment
												 {XOR_STR("align_left"), 0},
												 {XOR_STR("align_top"), 0},
												 {XOR_STR("align_center"), 1},
												 {XOR_STR("align_right"), 2},
												 {XOR_STR("align_bottom"), 2},
												 {XOR_STR("top_left"), 1},
												 {XOR_STR("top_right"), 2},
												 {XOR_STR("bottom_left"), 4},
												 {XOR_STR("bottom_right"), 8},
												 {XOR_STR("top"), 1 | 2},
												 {XOR_STR("bottom"), 4 | 8},
												 {XOR_STR("left"), 1 | 4},
												 {XOR_STR("right"), 2 | 8},
												 {XOR_STR("all"), 1 | 2 | 4 | 8},

												 // font flags
												 {XOR_STR("font_flag_shadow"), 1},
												 {XOR_STR("font_flag_outline"), 2},
												 {XOR_STR("font_flag_anti_alias"), 4},

												 // easing
												 {XOR_STR("linear"), 0},
												 {XOR_STR("ease_in"), 1},
												 {XOR_STR("ease_out"), 2},
												 {XOR_STR("ease_in_out"), 3},

												 // render modes
												 {XOR_STR("mode_normal"), 0},
												 {XOR_STR("mode_wireframe"), 1},

												 // outline modes
												 {XOR_STR("outline_inset"), 0},
												 {XOR_STR("outline_outset"), 1},
												 {XOR_STR("outline_center"), 2},

												 // esp sides
												 {XOR_STR("esp_left"), 0},
												 {XOR_STR("esp_right"), 1},
												 {XOR_STR("esp_top"), 2},
												 {XOR_STR("esp_bottom"), 3}});

	l.set_uint64s_for_table(XOR_STR("render"),
							{
								{XOR_STR("font_small"), FNV1A("small8")},
								{XOR_STR("font_main"), GUI_HASH("gui_main")},
								{XOR_STR("font_bold"), GUI_HASH("gui_bold")},

								{XOR_STR("texture_logo_head"), GUI_HASH("gui_logo_head")},
								{XOR_STR("texture_logo_stripes"), GUI_HASH("gui_logo_stripes")},
								{XOR_STR("texture_pattern"), GUI_HASH("gui_pattern")},
								{XOR_STR("texture_pattern_group"), GUI_HASH("gui_pattern_group")},
								{XOR_STR("texture_cursor"), GUI_HASH("gui_cursor")},
								{XOR_STR("texture_loading"), GUI_HASH("gui_loading")},
								{XOR_STR("texture_icon_up"), GUI_HASH("gui_icon_up")},
								{XOR_STR("texture_icon_down"), GUI_HASH("gui_icon_down")},
								{XOR_STR("texture_icon_clear"), GUI_HASH("gui_icon_clear")},
								{XOR_STR("texture_icon_copy"), GUI_HASH("gui_icon_copy")},
								{XOR_STR("texture_icon_paste"), GUI_HASH("gui_icon_paste")},
								{XOR_STR("texture_icon_alert"), GUI_HASH("gui_icon_alert")},
								{XOR_STR("texture_icon_rage"), GUI_HASH("gui_icon_rage")},
								{XOR_STR("texture_icon_legit"), GUI_HASH("gui_icon_legit")},
								{XOR_STR("texture_icon_visuals"), GUI_HASH("gui_icon_visuals")},
								{XOR_STR("texture_icon_misc"), GUI_HASH("gui_icon_misc")},
								{XOR_STR("texture_icon_scripts"), GUI_HASH("gui_icon_scripts")},
								{XOR_STR("texture_icon_skins"), GUI_HASH("gui_icon_skins")},
								{XOR_STR("texture_icon_cloud"), GUI_HASH("gui_icon_cloud")},
								{XOR_STR("texture_icon_file"), GUI_HASH("gui_icon_file")},
								{XOR_STR("texture_avatar"), GUI_HASH("user_avatar")},
								{XOR_STR("back_buffer"), GUI_HASH("back_buffer")},
								{XOR_STR("back_buffer_downsampled"), GUI_HASH("back_buffer_downsampled")},
							});

	l.get_global(XOR_STR("render"));
	{
		l.create_table();
		{
			l.create_table();
			{
				const auto c = sdk::color::attention();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("attention"));

			l.create_table();
			{
				const auto c = sdk::color::info();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("info"));

			l.create_table();
			{
				const auto c = sdk::color::warning();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("warning"));

			l.create_table();
			{
				const auto c = sdk::color::health_start();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("health_start"));

			l.create_table();
			{
				const auto c = sdk::color::health_end();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("health_end"));

			l.create_table();
			{
				const auto c = sdk::color::armor();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("armor"));

			l.create_table();
			{
				const auto c = sdk::color::ammo();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("ammo"));

			l.create_table();
			{
				const auto c = sdk::color::detonation();
				l.set_field(XOR_STR("r"), c.red());
				l.set_field(XOR_STR("g"), c.green());
				l.set_field(XOR_STR("b"), c.blue());
				l.set_field(XOR_STR("a"), c.alpha());
			}
			l.set_field(XOR_STR("detonation"));
		}
		l.set_field(XOR_STR("colors"));
	}
	l.pop(1);

	l.create_namespace(XOR_STR("utils"), {
											 {XOR_STR("random_int"), api_def::utils::random_int},
											 {XOR_STR("random_float"), api_def::utils::random_float},
											 {XOR_STR("flags"), api_def::utils::flags},
											 {XOR_STR("find_interface"), api_def::utils::find_interface},
											 {XOR_STR("find_pattern"), api_def::utils::find_pattern},
											 {XOR_STR("get_weapon_info"), api_def::utils::get_weapon_info},
											 {XOR_STR("new_timer"), api_def::utils::new_timer},
											 {XOR_STR("run_delayed"), api_def::utils::run_delayed},
											 {XOR_STR("world_to_screen"), api_def::utils::world_to_screen},
											 {XOR_STR("get_rtt"), api_def::utils::get_rtt},
											 {XOR_STR("get_time"), api_def::utils::get_time},
											 {XOR_STR("http_get"), api_def::utils::http_get},
											 {XOR_STR("http_post"), api_def::utils::http_post},
											 {XOR_STR("json_encode"), api_def::utils::json_encode},
											 {XOR_STR("json_decode"), api_def::utils::json_decode},
											 {XOR_STR("set_clan_tag"), api_def::utils::set_clan_tag},
											 {XOR_STR("scale_damage"), api_def::utils::scale_damage},
											 {XOR_STR("trace"), api_def::utils::trace},
											 {XOR_STR("trace_bullet"), api_def::utils::trace_bullet},
#ifdef CSGO_SECURE
											 {XOR_STR("aes256_encrypt"), api_def::utils::aes256_encrypt},
											 {XOR_STR("aes256_decrypt"), api_def::utils::aes256_decrypt},
											 {XOR_STR("base64_encode"), api_def::utils::base64_encode},
											 {XOR_STR("base64_decode"), api_def::utils::base64_decode},
											 {XOR_STR("get_unix_time"), api_def::utils::get_unix_time},
#endif
										 });

	l.create_namespace(XOR_STR("gui"), {
										   {XOR_STR("textbox"), api_def::gui::textbox_construct},
										   {XOR_STR("checkbox"), api_def::gui::checkbox_construct},
										   {XOR_STR("slider"), api_def::gui::slider_construct},
										   {XOR_STR("get_checkbox"), api_def::gui::get_checkbox},
										   {XOR_STR("get_slider"), api_def::gui::get_slider},
										   {XOR_STR("add_notification"), api_def::gui::add_notification},
										   {XOR_STR("for_each_hotkey"), api_def::gui::for_each_hotkey},
										   {XOR_STR("is_menu_open"), api_def::gui::is_menu_open},
										   {XOR_STR("get_combobox"), api_def::gui::get_combobox},
										   {XOR_STR("combobox"), api_def::gui::combobox_construct},
										   {XOR_STR("label"), api_def::gui::label_construct},
										   {XOR_STR("button"), api_def::gui::button_construct},
										   {XOR_STR("color_picker"), api_def::gui::color_picker_construct},
										   {XOR_STR("get_color_picker"), api_def::gui::get_color_picker},
										   {XOR_STR("show_message"), api_def::gui::show_message},
										   {XOR_STR("show_dialog"), api_def::gui::show_dialog},
										   {XOR_STR("get_menu_rect"), api_def::gui::get_menu_rect},
										   {XOR_STR("list"), api_def::gui::list_construct},
									   });

	l.set_integers_for_table(XOR_STR("gui"), {
												 {XOR_STR("hotkey_toggle"), 0},
												 {XOR_STR("hotkey_hold"), 1},
												 {XOR_STR("dialog_buttons_ok_cancel"), 0},
												 {XOR_STR("dialog_buttons_yes_no"), 1},
												 {XOR_STR("dialog_buttons_yes_no_cancel"), 2},
												 {XOR_STR("dialog_result_affirmative"), 0},
												 {XOR_STR("dialog_result_negative"), 1},
											 });

	l.create_namespace(XOR_STR("input"), {
											 {XOR_STR("is_key_down"), api_def::input::is_key_down},
											 {XOR_STR("is_mouse_down"), api_def::input::is_mouse_down},
											 {XOR_STR("get_cursor_pos"), api_def::input::get_cursor_pos},
										 });

	l.set_integers_for_table(XOR_STR("input"), {
												   {XOR_STR("mouse_left"), 0},
												   {XOR_STR("mouse_right"), 1},
												   {XOR_STR("mouse_middle"), 2},
												   {XOR_STR("mouse_back"), 3},
												   {XOR_STR("mouse_forward"), 4},
											   });

	l.extend_namespace(XOR_STR("math"), {
											{XOR_STR("vec3"), api_def::math::vec3_new},
											{XOR_STR("clamp"), api_def::math::clamp},
										});

	l.create_namespace(XOR_STR("panorama"), {{XOR_STR("eval"), api_def::panorama::eval}});

	l.create_namespace(XOR_STR("fs"), {
										  {XOR_STR("read"), api_def::fs::read},
										  {XOR_STR("read_stream"), api_def::fs::read_stream},
										  {XOR_STR("write"), api_def::fs::write},
										  {XOR_STR("write_stream"), api_def::fs::write_stream},
										  {XOR_STR("remove"), api_def::fs::remove},
										  {XOR_STR("exists"), api_def::fs::exists},
										  {XOR_STR("is_file"), api_def::fs::is_file},
										  {XOR_STR("is_dir"), api_def::fs::is_dir},
										  {XOR_STR("create_dir"), api_def::fs::create_dir},
									  });

#ifdef CSGO_SECURE
	l.create_namespace(XOR_STR("zip"), {
										   {XOR_STR("create"), api_def::zip::create},
										   {XOR_STR("open"), api_def::zip::open},
										   {XOR_STR("open_stream"), api_def::zip::open_stream},
									   });

	l.create_namespace(XOR_STR("payments"), {
												{XOR_STR("create"), api_def::payments::create},
												{XOR_STR("restore"), api_def::payments::restore},
											});

	l.create_namespace(XOR_STR("net"), {
										   {XOR_STR("http_get"), api_def::net::http_get},
										   {XOR_STR("http_post"), api_def::net::http_post},
										   {XOR_STR("listen"), api_def::net::listen},
										   {XOR_STR("broadcast"), api_def::net::broadcast},
									   });
#endif

	l.create_namespace(XOR_STR("mat"), {
										   {XOR_STR("create"), api_def::mat::create},
										   {XOR_STR("find"), api_def::mat::find},
										   {XOR_STR("for_each_material"), api_def::mat::for_each_material},
										   {XOR_STR("override_material"), api_def::mat::override_material},
									   });

	l.set_integers_for_table(XOR_STR("mat"),
							 {
								 {XOR_STR("var_debug"), sdk::material_var_debug},
								 {XOR_STR("var_no_debug_override"), sdk::material_var_no_debug_override},
								 {XOR_STR("var_no_draw"), sdk::material_var_no_draw},
								 {XOR_STR("var_use_in_fillrate_mode"), sdk::material_var_use_in_fillrate_mode},
								 {XOR_STR("var_vertexcolor"), sdk::material_var_vertexcolor},
								 {XOR_STR("var_vertexalpha"), sdk::material_var_vertexalpha},
								 {XOR_STR("var_selfillum"), sdk::material_var_selfillum},
								 {XOR_STR("var_additive"), sdk::material_var_additive},
								 {XOR_STR("var_alphatest"), sdk::material_var_alphatest},
								 {XOR_STR("var_znearer"), sdk::material_var_znearer},
								 {XOR_STR("var_model"), sdk::material_var_model},
								 {XOR_STR("var_flat"), sdk::material_var_flat},
								 {XOR_STR("var_nocull"), sdk::material_var_nocull},
								 {XOR_STR("var_nofog"), sdk::material_var_nofog},
								 {XOR_STR("var_ignorez"), sdk::material_var_ignorez},
								 {XOR_STR("var_decal"), sdk::material_var_decal},
								 {XOR_STR("var_envmapsphere"), sdk::material_var_envmapsphere},
								 {XOR_STR("var_envmapcameraspace"), sdk::material_var_envmapcameraspace},
								 {XOR_STR("var_basealphaenvmapmask"), sdk::material_var_basealphaenvmapmask},
								 {XOR_STR("var_translucent"), sdk::material_var_translucent},
								 {XOR_STR("var_normalmapalphaenvmapmask"), sdk::material_var_normalmapalphaenvmapmask},
								 {XOR_STR("var_needs_software_skinning"), sdk::material_var_needs_software_skinning},
								 {XOR_STR("var_opaquetexture"), sdk::material_var_opaquetexture},
								 {XOR_STR("var_envmapmode"), sdk::material_var_envmapmode},
								 {XOR_STR("var_suppress_decals"), sdk::material_var_suppress_decals},
								 {XOR_STR("var_halflambert"), sdk::material_var_halflambert},
								 {XOR_STR("var_wireframe"), sdk::material_var_wireframe},
								 {XOR_STR("var_allowalphatocoverage"), sdk::material_var_allowalphatocoverage},
								 {XOR_STR("var_alpha_modified_by_proxy"), sdk::material_var_alpha_modified_by_proxy},
								 {XOR_STR("var_vertexfog"), sdk::material_var_vertexfog},
							 });

	l.create_table();
	{
		l.set_field(XOR_STR("frame_undefined"), -1);
		l.set_field(XOR_STR("frame_start"), 0);
		l.set_field(XOR_STR("frame_net_update_start"), 1);
		l.set_field(XOR_STR("frame_net_update_postdataupdate_start"), 2);
		l.set_field(XOR_STR("frame_net_update_postdataupdate_end"), 3);
		l.set_field(XOR_STR("frame_net_update_end"), 4);
		l.set_field(XOR_STR("frame_render_start"), 5);
		l.set_field(XOR_STR("frame_render_end"), 6);
		l.set_field(XOR_STR("in_attack"), sdk::user_cmd::attack);
		l.set_field(XOR_STR("in_jump"), sdk::user_cmd::jump);
		l.set_field(XOR_STR("in_duck"), sdk::user_cmd::duck);
		l.set_field(XOR_STR("in_forward"), sdk::user_cmd::forward);
		l.set_field(XOR_STR("in_back"), sdk::user_cmd::back);
		l.set_field(XOR_STR("in_use"), sdk::user_cmd::use);
		l.set_field(XOR_STR("in_left"), sdk::user_cmd::left);
		l.set_field(XOR_STR("in_move_left"), sdk::user_cmd::move_left);
		l.set_field(XOR_STR("in_right"), sdk::user_cmd::right);
		l.set_field(XOR_STR("in_move_right"), sdk::user_cmd::move_right);
		l.set_field(XOR_STR("in_attack2"), sdk::user_cmd::attack2);
		l.set_field(XOR_STR("in_score"), sdk::user_cmd::score);
	}
	l.set_global(XOR_STR("csgo"));

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void lua::script::register_globals()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	l.set_global_function(XOR_STR("print"), api_def::print);
	l.set_global_function(XOR_STR("require"), api_def::require);
	l.set_global_function(XOR_STR("loadfile"), api_def::loadfile);

	l.create_table();
	{
		l.set_field(XOR_STR("save"), api_def::database::save);
		l.set_field(XOR_STR("load"), api_def::database::load);

		l.create_metatable(XOR_STR("database"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::stub_index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("database"));

	l.create_table();
	{
		l.set_field(XOR_STR("is_in_game"), api_def::engine::is_in_game);
		l.set_field(XOR_STR("exec"), api_def::engine::exec);
		l.set_field(XOR_STR("get_local_player"), api_def::engine::get_local_player);
		l.set_field(XOR_STR("get_view_angles"), api_def::engine::get_view_angles);
		l.set_field(XOR_STR("get_player_for_user_id"), api_def::engine::get_player_for_user_id);
		l.set_field(XOR_STR("get_player_info"), api_def::engine::get_player_info);
		l.set_field(XOR_STR("set_view_angles"), api_def::engine::set_view_angles);

		l.create_metatable(XOR_STR("engine"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::stub_index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("engine"));

	l.create_table();
	{
		l.set_field(XOR_STR("get_entity"), api_def::entities::get_entity);
		l.set_field(XOR_STR("get_entity_from_handle"), api_def::entities::get_entity_from_handle);
		l.set_field(XOR_STR("for_each"), api_def::entities::for_each);
		l.set_field(XOR_STR("for_each_z"), api_def::entities::for_each_z);
		l.set_field(XOR_STR("for_each_player"), api_def::entities::for_each_player);

		l.create_metatable(XOR_STR("entities"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::entities::index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("entities"));

	l.create_table();
	{
		l.create_table();
		{
			l.create_metatable(XOR_STR("ev0lve"));
			l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
			l.set_field(XOR_STR("__index"), api_def::ev0lve_index);
			l.set_metatable();
		}
		l.set_field(XOR_STR("ev0lve"));

		l.create_table();
		{
			l.create_metatable(XOR_STR("server"));
			l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
			l.set_field(XOR_STR("__index"), api_def::server_index);
			l.set_metatable();
		}
		l.set_field(XOR_STR("server"));
	}
	l.set_global(XOR_STR("info"));

	l.create_table();
	{
		l.create_metatable(XOR_STR("cvar"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::cvar::index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("cvar"));

	l.create_table();
	{
		l.create_metatable(XOR_STR("global_vars"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::global_vars_index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("global_vars"));

	l.create_table();
	{
		l.create_metatable(XOR_STR("game_rules"));
		l.set_field(XOR_STR("__newindex"), api_def::stub_new_index);
		l.set_field(XOR_STR("__index"), api_def::game_rules_index);
		l.set_metatable();
	}
	l.set_global(XOR_STR("game_rules"));

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void lua::script::register_types()
{
#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_START;
#endif

	l.create_type(XOR_STR("gui.textbox"), {
											  {XOR_STR("get_value"), api_def::gui::textbox_get_value},
											  {XOR_STR("get"), api_def::gui::textbox_get_value},
											  {XOR_STR("set_value"), api_def::gui::textbox_set_value},
											  {XOR_STR("set"), api_def::gui::textbox_set_value},
											  {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											  {XOR_STR("set_visible"), api_def::gui::control_set_visible},
											  {XOR_STR("add_callback"), api_def::gui::control_add_callback},
											  {XOR_STR("get_name"), api_def::gui::control_get_name},
											  {XOR_STR("get_type"), api_def::gui::control_get_type},
										  });

	l.create_type(XOR_STR("gui.checkbox"), {
											   {XOR_STR("get_value"), api_def::gui::checkbox_get_value},
											   {XOR_STR("get"), api_def::gui::checkbox_get_value},
											   {XOR_STR("set_value"), api_def::gui::checkbox_set_value},
											   {XOR_STR("set"), api_def::gui::checkbox_set_value},
											   {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											   {XOR_STR("set_visible"), api_def::gui::control_set_visible},
											   {XOR_STR("add_callback"), api_def::gui::control_add_callback},
											   {XOR_STR("get_name"), api_def::gui::control_get_name},
											   {XOR_STR("get_type"), api_def::gui::control_get_type},
										   });

	l.create_type(XOR_STR("gui.slider"), {
											 {XOR_STR("get_value"), api_def::gui::slider_get_value},
											 {XOR_STR("get"), api_def::gui::slider_get_value},
											 {XOR_STR("set_value"), api_def::gui::slider_set_value},
											 {XOR_STR("set"), api_def::gui::slider_set_value},
											 {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											 {XOR_STR("set_visible"), api_def::gui::control_set_visible},
											 {XOR_STR("add_callback"), api_def::gui::control_add_callback},
											 {XOR_STR("get_name"), api_def::gui::control_get_name},
											 {XOR_STR("get_type"), api_def::gui::control_get_type},
										 });

	l.create_type(XOR_STR("gui.combobox"), {
											   {XOR_STR("get_value"), api_def::gui::combobox_get_value},
											   {XOR_STR("get"), api_def::gui::combobox_get_value},
											   {XOR_STR("set_value"), api_def::gui::combobox_set_value},
											   {XOR_STR("set"), api_def::gui::combobox_set_value},
											   {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											   {XOR_STR("set_visible"), api_def::gui::control_set_visible},
											   {XOR_STR("add_callback"), api_def::gui::control_add_callback},
											   {XOR_STR("get_options"), api_def::gui::combobox_get_options},
											   {XOR_STR("get_name"), api_def::gui::control_get_name},
											   {XOR_STR("get_type"), api_def::gui::control_get_type},
										   });

	l.create_type(XOR_STR("gui.color_picker"), {
												   {XOR_STR("get_value"), api_def::gui::color_picker_get_value},
												   {XOR_STR("get"), api_def::gui::color_picker_get_value},
												   {XOR_STR("set_value"), api_def::gui::color_picker_set_value},
												   {XOR_STR("set"), api_def::gui::color_picker_set_value},
												   {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
												   {XOR_STR("set_visible"), api_def::gui::control_set_visible},
												   {XOR_STR("add_callback"), api_def::gui::control_add_callback},
												   {XOR_STR("get_name"), api_def::gui::control_get_name},
												   {XOR_STR("get_type"), api_def::gui::control_get_type},
											   });

	l.create_type(XOR_STR("gui.label"), {
											{XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											{XOR_STR("set_visible"), api_def::gui::control_set_visible},
											{XOR_STR("add_callback"), api_def::gui::control_add_callback},
											{XOR_STR("get_name"), api_def::gui::control_get_name},
											{XOR_STR("get_type"), api_def::gui::control_get_type},
										});

	l.create_type(XOR_STR("gui.button"), {
											 {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
											 {XOR_STR("set_visible"), api_def::gui::control_set_visible},
											 {XOR_STR("add_callback"), api_def::gui::control_add_callback},
											 {XOR_STR("get_name"), api_def::gui::control_get_name},
											 {XOR_STR("get_type"), api_def::gui::control_get_type},
										 });

	l.create_type(XOR_STR("gui.list"), {
										   {XOR_STR("set_tooltip"), api_def::gui::control_set_tooltip},
										   {XOR_STR("set_visible"), api_def::gui::control_set_visible},
										   {XOR_STR("add_callback"), api_def::gui::control_add_callback},
										   {XOR_STR("get_value"), api_def::gui::list_get_value},
										   {XOR_STR("get"), api_def::gui::list_get_value},
										   {XOR_STR("set_value"), api_def::gui::list_set_value},
										   {XOR_STR("set"), api_def::gui::list_set_value},
										   {XOR_STR("add"), api_def::gui::list_add},
										   {XOR_STR("remove"), api_def::gui::list_remove},
										   {XOR_STR("get_options"), api_def::gui::list_get_options},
										   {XOR_STR("get_name"), api_def::gui::control_get_name},
										   {XOR_STR("get_type"), api_def::gui::control_get_type},
									   });

	l.create_type(XOR_STR("render.anim_float"), {
													{XOR_STR("direct"), api_def::render::anim_float_direct},
													{XOR_STR("get_value"), api_def::render::anim_float_get_value},
													{XOR_STR("get"), api_def::render::anim_float_get_value},
												});

	l.create_type(XOR_STR("render.anim_color"), {
													{XOR_STR("direct"), api_def::render::anim_color_direct},
													{XOR_STR("get_value"), api_def::render::anim_color_get_value},
													{XOR_STR("get"), api_def::render::anim_color_get_value},
												});

	l.create_type(XOR_STR("csgo.netvar"), {
											  {XOR_STR("__index"), api_def::net_prop::index},
											  {XOR_STR("__newindex"), api_def::net_prop::new_index},
										  });

	l.create_type(XOR_STR("csgo.entity"), {
											  {XOR_STR("__index"), api_def::entity::index},
											  {XOR_STR("__newindex"), api_def::entity::new_index},
										  });

	l.create_type(XOR_STR("csgo.cvar"), {
											{XOR_STR("get_int"), api_def::cvar::get_int},
											{XOR_STR("get_float"), api_def::cvar::get_float},
											{XOR_STR("set_int"), api_def::cvar::set_int},
											{XOR_STR("set_float"), api_def::cvar::set_float},
											{XOR_STR("get_string"), api_def::cvar::get_string},
											{XOR_STR("set_string"), api_def::cvar::set_string},
										});

	l.create_type(XOR_STR("csgo.event"), {
											 {XOR_STR("__index"), api_def::event::index},
											 {XOR_STR("__newindex"), api_def::stub_new_index},
										 });

	l.create_type(XOR_STR("csgo.user_cmd"), {
												{XOR_STR("get_command_number"), api_def::user_cmd::get_command_number},
												{XOR_STR("get_view_angles"), api_def::user_cmd::get_view_angles},
												{XOR_STR("get_move"), api_def::user_cmd::get_move},
												{XOR_STR("get_buttons"), api_def::user_cmd::get_buttons},
												{XOR_STR("set_view_angles"), api_def::user_cmd::set_view_angles},
												{XOR_STR("set_move"), api_def::user_cmd::set_move},
												{XOR_STR("set_buttons"), api_def::user_cmd::set_buttons},
											});

	l.create_type(XOR_STR("csgo.material"), {
												{XOR_STR("__index"), api_def::mat::index},
												{XOR_STR("__gc"), api_def::mat::gc},
											});

	l.create_type(XOR_STR("csgo.material_var"), {
													{XOR_STR("get_float"), api_def::mat::get_float},
													{XOR_STR("set_float"), api_def::mat::set_float},
													{XOR_STR("get_int"), api_def::mat::get_int},
													{XOR_STR("set_int"), api_def::mat::set_int},
													{XOR_STR("get_string"), api_def::mat::get_string},
													{XOR_STR("set_string"), api_def::mat::set_string},
													{XOR_STR("get_vector"), api_def::mat::get_vector},
													{XOR_STR("set_vector"), api_def::mat::set_vector},
												});

	l.create_type(XOR_STR("utils.timer"), {{XOR_STR("start"), api_def::timer::start},
										   {XOR_STR("stop"), api_def::timer::stop},
										   {XOR_STR("run_once"), api_def::timer::run_once},
										   {XOR_STR("set_delay"), api_def::timer::set_delay},
										   {XOR_STR("is_active"), api_def::timer::is_active}});

	l.create_type(XOR_STR("vec3"), {
									   {XOR_STR("__index"), api_def::math::vec3_index},
									   {XOR_STR("__newindex"), api_def::math::vec3_new_index},
									   {XOR_STR("__add"), api_def::math::vec3_add},
									   {XOR_STR("__sub"), api_def::math::vec3_sub},
									   {XOR_STR("__mul"), api_def::math::vec3_mul},
									   {XOR_STR("__div"), api_def::math::vec3_div},
									   {XOR_STR("length"), api_def::math::vec3_length},
									   {XOR_STR("length2d"), api_def::math::vec3_length2d},
									   {XOR_STR("length_sqr"), api_def::math::vec3_length_sqr},
									   {XOR_STR("length2d_sqr"), api_def::math::vec3_length2d_sqr},
									   {XOR_STR("dot"), api_def::math::vec3_dot},
									   {XOR_STR("cross"), api_def::math::vec3_cross},
									   {XOR_STR("normalize"), api_def::math::vec3_normalize},
									   {XOR_STR("unpack"), api_def::math::vec3_unpack},
								   });

	l.create_type(XOR_STR("render.esp_builder"), {
													 {XOR_STR("text"), api_def::render::esp_text},
													 {XOR_STR("bar"), api_def::render::esp_bar},
													 {XOR_STR("icon"), api_def::render::esp_icon},
												 });

#ifdef CSGO_SECURE
	l.create_type(XOR_STR("zip"), {
									  {XOR_STR("__gc"), api_def::zip::gc},
									  {XOR_STR("get_files"), api_def::zip::get_files},
									  {XOR_STR("read"), api_def::zip::read},
									  {XOR_STR("read_stream"), api_def::zip::read_stream},
									  {XOR_STR("write"), api_def::zip::write},
									  {XOR_STR("write_stream"), api_def::zip::write_stream},
									  {XOR_STR("save"), api_def::zip::save},
									  {XOR_STR("exists"), api_def::zip::exists},
									  {XOR_STR("extract"), api_def::zip::extract},
									  {XOR_STR("extract_all"), api_def::zip::extract_all},
								  });
#endif

#ifdef CSGO_SECURE
	VIRTUALIZER_SHARK_WHITE_END;
#endif
}

void lua::script::add_gui_element(uint64_t e_id) { gui_elements.emplace_back(e_id); }

void lua::script::add_gui_element_with_callback(uint64_t e_id)
{
	if (std::find(gui_callbacks.begin(), gui_callbacks.end(), e_id) == gui_callbacks.end())
		gui_callbacks.emplace_back(e_id);
}

void lua::script::add_font(uint64_t _id) { fonts.emplace_back(_id); }

void lua::script::add_texture(uint64_t _id) { textures.emplace_back(_id); }

void lua::script::add_animator(uint64_t _id) { animators.emplace_back(_id); }

void lua::script::add_shader(uint64_t _id) { shaders.emplace_back(_id); }

void lua::script::add_net_id(const std::string &_id) { net_ids.emplace_back(_id); }

void lua::script::run_timers() const
{
	if (!is_running || lua::helpers::timers[id].empty())
		return;

	auto &my_timers = lua::helpers::timers[id];

	// loop through all existing callbacks
	for (auto it = my_timers.begin(); it != my_timers.end();)
	{
		// get current callback
		const auto timer = *it;

		// skip if inactive
		if (!timer->is_active())
		{
			++it;
			continue;
		}

		// we waited long enough
		if (timer->cycle_completed())
		{
			// run callback
			const auto cbk = timer->get_callback();

			if (cbk)
				cbk();
		}

		// should be deleted
		if (timer->should_delete())
		{
			// erase callback
			it = my_timers.erase(it);
		}
		else
			++it;
	}
}