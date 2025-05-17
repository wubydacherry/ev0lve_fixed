
#include <base/cfg.h>
#include <base/game.h>
#include <filesystem>

#include <gui/gui.h>
#include <sdk/weapon.h>
#include <util/fnv1a.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

#ifdef CSGO_SECURE
#include <util/util.h>
#endif

#define ADD(n) entries[GUI_HASH(#n)] = &n;

weapon_groups cfg_t::get_group(sdk::weapon_t *wpn)
{
	if (!wpn)
		return grp_general;

	if (wpn->is_pistol())
	{
		if (wpn->is_heavy_pistol())
			return grp_heavypistol;

		return grp_pistol;
	}

	if (wpn->is_automatic())
		return grp_automatic;

	if (wpn->is_sniper())
	{
		if (wpn->get_item_definition_index() == sdk::item_definition_index::weapon_awp)
			return grp_awp;

		if (wpn->get_item_definition_index() == sdk::item_definition_index::weapon_ssg08)
			return grp_scout;

		return grp_autosniper;
	}

	if (wpn->get_item_definition_index() == sdk::item_definition_index::weapon_taser)
		return grp_zeus;

	return grp_general;
}

void cfg_t::on_create_move(sdk::weapon_t *wpn)
{
	auto &zeus = rage.weapon_cfgs[grp_zeus];
	if (zeus.min_dmg.get() != 100.f)
	{
		zeus.min_dmg.set(100.f);
		zeus.hitbox->set(aim_upper_body);
		zeus.hitbox->set(aim_lower_body);
		zeus.hitchance_value.set(75.f);
		zeus.multipoint.set(50.f);
		zeus.secure_point.set(secure_point_force);
	}

	// handle changes for the gui
	for (auto &c : rage.weapon_cfgs)
		c.handle_on_anything_changed();

	// handle group selection
	const auto grp = get_group(wpn);
	build_rage_config(grp);

	legit.hack = legit.weapon_cfgs[grp];
	trigger.hack = trigger.weapon_cfgs[grp];
}

void cfg_t::build_rage_config(weapon_groups grp)
{
	// this part sucks but sacrifices should be made
	if (grp == grp_zeus)
	{
		rage.hack = rage.weapon_cfgs[grp];
		return;
	}

	auto &cfg_general = rage.weapon_cfgs[grp_general];
	if (grp == grp_general)
	{
		rage.hack = cfg_general;
		return;
	}

	auto &cfg_target = rage.weapon_cfgs[grp];
	rage.hack = cfg_target.was_anything_changed() ? cfg_target : cfg_general;
}

void cfg_t::init()
{
	if (!std::filesystem::exists(XOR("ev0lve")))
		std::filesystem::create_directories(XOR("ev0lve"));
	if (!std::filesystem::exists(XOR("ev0lve/default.cfg")))
		std::ofstream _(XOR("ev0lve/default.cfg"));

	ADD(rage.enable);
	ADD(rage.mode);
	ADD(rage.fov);
	ADD(rage.only_visible);
	ADD(rage.auto_engage);
	ADD(rage.enable_fast_fire);
	ADD(rage.dormant);
	ADD(rage.fast_fire);
	ADD(rage.hide_shot);
	ADD(rage.slowwalk);
	ADD(rage.slowwalk_mode);
	ADD(rage.fake_duck);
	ADD(rage.fake_ping);
	ADD(rage.fake_ping_amount);
	ADD(rage.weapon_cfgs[grp_general].secure_point);
	ADD(rage.weapon_cfgs[grp_general].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_general].delay_shot);
	ADD(rage.weapon_cfgs[grp_general].auto_scope);
	ADD(rage.weapon_cfgs[grp_general].hitchance_value);
	ADD(rage.weapon_cfgs[grp_general].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_general].min_dmg);
	ADD(rage.weapon_cfgs[grp_general].hitbox);
	ADD(rage.weapon_cfgs[grp_general].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_general].lethal);
	ADD(rage.weapon_cfgs[grp_general].multipoint);
	ADD(rage.weapon_cfgs[grp_pistol].secure_point);
	ADD(rage.weapon_cfgs[grp_pistol].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_pistol].delay_shot);
	ADD(rage.weapon_cfgs[grp_pistol].auto_scope);
	ADD(rage.weapon_cfgs[grp_pistol].hitchance_value);
	ADD(rage.weapon_cfgs[grp_pistol].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_pistol].min_dmg);
	ADD(rage.weapon_cfgs[grp_pistol].hitbox);
	ADD(rage.weapon_cfgs[grp_pistol].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_pistol].lethal);
	ADD(rage.weapon_cfgs[grp_pistol].multipoint);
	ADD(rage.weapon_cfgs[grp_heavypistol].secure_point);
	ADD(rage.weapon_cfgs[grp_heavypistol].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_heavypistol].delay_shot);
	ADD(rage.weapon_cfgs[grp_heavypistol].auto_scope);
	ADD(rage.weapon_cfgs[grp_heavypistol].hitchance_value);
	ADD(rage.weapon_cfgs[grp_heavypistol].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_heavypistol].min_dmg);
	ADD(rage.weapon_cfgs[grp_heavypistol].hitbox);
	ADD(rage.weapon_cfgs[grp_heavypistol].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_heavypistol].lethal);
	ADD(rage.weapon_cfgs[grp_heavypistol].multipoint);
	ADD(rage.weapon_cfgs[grp_automatic].secure_point);
	ADD(rage.weapon_cfgs[grp_automatic].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_automatic].delay_shot);
	ADD(rage.weapon_cfgs[grp_automatic].auto_scope);
	ADD(rage.weapon_cfgs[grp_automatic].hitchance_value);
	ADD(rage.weapon_cfgs[grp_automatic].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_automatic].min_dmg);
	ADD(rage.weapon_cfgs[grp_automatic].hitbox);
	ADD(rage.weapon_cfgs[grp_automatic].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_automatic].lethal);
	ADD(rage.weapon_cfgs[grp_automatic].multipoint);
	ADD(rage.weapon_cfgs[grp_awp].secure_point);
	ADD(rage.weapon_cfgs[grp_awp].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_awp].delay_shot);
	ADD(rage.weapon_cfgs[grp_awp].auto_scope);
	ADD(rage.weapon_cfgs[grp_awp].hitchance_value);
	ADD(rage.weapon_cfgs[grp_awp].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_awp].min_dmg);
	ADD(rage.weapon_cfgs[grp_awp].hitbox);
	ADD(rage.weapon_cfgs[grp_awp].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_awp].lethal);
	ADD(rage.weapon_cfgs[grp_awp].multipoint);
	ADD(rage.weapon_cfgs[grp_scout].secure_point);
	ADD(rage.weapon_cfgs[grp_scout].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_scout].delay_shot);
	ADD(rage.weapon_cfgs[grp_scout].auto_scope);
	ADD(rage.weapon_cfgs[grp_scout].hitchance_value);
	ADD(rage.weapon_cfgs[grp_scout].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_scout].min_dmg);
	ADD(rage.weapon_cfgs[grp_scout].hitbox);
	ADD(rage.weapon_cfgs[grp_scout].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_scout].lethal);
	ADD(rage.weapon_cfgs[grp_scout].multipoint);
	ADD(rage.weapon_cfgs[grp_autosniper].secure_point);
	ADD(rage.weapon_cfgs[grp_autosniper].secure_fast_fire);
	ADD(rage.weapon_cfgs[grp_autosniper].delay_shot);
	ADD(rage.weapon_cfgs[grp_autosniper].auto_scope);
	ADD(rage.weapon_cfgs[grp_autosniper].hitchance_value);
	ADD(rage.weapon_cfgs[grp_autosniper].dynamic_min_dmg);
	ADD(rage.weapon_cfgs[grp_autosniper].min_dmg);
	ADD(rage.weapon_cfgs[grp_autosniper].hitbox);
	ADD(rage.weapon_cfgs[grp_autosniper].avoid_hitbox);
	ADD(rage.weapon_cfgs[grp_autosniper].lethal);
	ADD(rage.weapon_cfgs[grp_autosniper].multipoint);

	ADD(antiaim.enable);
	ADD(antiaim.disablers);
	ADD(antiaim.pitch);
	ADD(antiaim.yaw);
	ADD(antiaim.yaw_override);
	ADD(antiaim.yaw_modifier);
	ADD(antiaim.yaw_modifier_amount);
	ADD(antiaim.yaw_offset);
	ADD(antiaim.foot_yaw);
	ADD(antiaim.foot_yaw_amount);
	ADD(antiaim.foot_yaw_flip);
	ADD(antiaim.desync_limit);
	ADD(antiaim.body_lean);
	ADD(antiaim.body_lean_amount);
	ADD(antiaim.body_lean_flip);
	ADD(antiaim.movement_mode);
	ADD(antiaim.desync_triggers);
	ADD(antiaim.ensure_body_lean);

	ADD(fakelag.mode);
	ADD(fakelag.lag_amount);
	ADD(fakelag.variance);

	ADD(player_esp.enemy);
	ADD(player_esp.team);
	ADD(player_esp.enemy_skeleton);
	ADD(player_esp.team_skeleton);
	ADD(player_esp.enemy_skeleton_color);
	ADD(player_esp.team_skeleton_color);
	ADD(player_esp.vis_check);
	ADD(player_esp.enemy_color);
	ADD(player_esp.team_color);
	ADD(player_esp.enemy_color_visible);
	ADD(player_esp.team_color_visible);
	ADD(player_esp.prefer_icons);
	ADD(player_esp.apply_on_death);
	ADD(player_esp.glow_enemy);
	ADD(player_esp.glow_team);
	ADD(player_esp.enemy_color_glow);
	ADD(player_esp.team_color_glow);
	ADD(player_esp.enemy_chams.mode);
	ADD(player_esp.enemy_chams.primary);
	ADD(player_esp.enemy_chams.secondary);
	ADD(player_esp.enemy_chams_visible.mode);
	ADD(player_esp.enemy_chams_visible.primary);
	ADD(player_esp.enemy_chams_visible.secondary);
	ADD(player_esp.enemy_chams_history.mode);
	ADD(player_esp.enemy_chams_history.primary);
	ADD(player_esp.enemy_chams_history.secondary);
	ADD(player_esp.enemy_attachment_chams.mode);
	ADD(player_esp.enemy_attachment_chams.primary);
	ADD(player_esp.enemy_attachment_chams.secondary);
	ADD(player_esp.enemy_attachment_chams_visible.mode);
	ADD(player_esp.enemy_attachment_chams_visible.primary);
	ADD(player_esp.enemy_attachment_chams_visible.secondary);
	ADD(player_esp.team_chams.mode);
	ADD(player_esp.team_chams.primary);
	ADD(player_esp.team_chams.secondary);
	ADD(player_esp.team_chams_visible.mode);
	ADD(player_esp.team_chams_visible.primary);
	ADD(player_esp.team_chams_visible.secondary);
	ADD(player_esp.team_attachment_chams.mode);
	ADD(player_esp.team_attachment_chams.primary);
	ADD(player_esp.team_attachment_chams.secondary);
	ADD(player_esp.team_attachment_chams_visible.mode);
	ADD(player_esp.team_attachment_chams_visible.primary);
	ADD(player_esp.team_attachment_chams_visible.secondary);
	ADD(player_esp.offscreen_esp);
	ADD(player_esp.offscreen_radius);
	ADD(player_esp.legit_esp);
	ADD(player_esp.legit_triggers);
	ADD(player_esp.target_scan);
	ADD(player_esp.target_scan_secure);
	ADD(player_esp.target_scan_aim);
	ADD(player_esp.disable_dormancy_interp);
	ADD(player_esp.shared_esp);
	ADD(player_esp.shared_esp_enemy);
	ADD(player_esp.offscreen_color);

	ADD(local_visuals.glow_local);
	ADD(local_visuals.fov_changer);
	ADD(local_visuals.fov);
	ADD(local_visuals.fov2);
	ADD(local_visuals.skip_first_scope);
	ADD(local_visuals.viewmodel_in_scope);
	ADD(local_visuals.thirdperson);
	ADD(local_visuals.thirdperson_distance);
	ADD(local_visuals.thirdperson_dead);
	ADD(local_visuals.thirdperson_nade);
	ADD(local_visuals.alpha_blend);
	ADD(local_visuals.hitmarker);
	ADD(local_visuals.hitsound);
	ADD(local_visuals.local_color_glow);
	ADD(local_visuals.local_chams.mode);
	ADD(local_visuals.local_chams.primary);
	ADD(local_visuals.local_chams.secondary);
	ADD(local_visuals.local_attachment_chams.mode);
	ADD(local_visuals.local_attachment_chams.primary);
	ADD(local_visuals.local_attachment_chams.secondary);
	ADD(local_visuals.fake_chams.mode);
	ADD(local_visuals.fake_chams.primary);
	ADD(local_visuals.fake_chams.secondary);
	ADD(local_visuals.weapon_chams.mode);
	ADD(local_visuals.weapon_chams.primary);
	ADD(local_visuals.weapon_chams.secondary);
	ADD(local_visuals.arms_chams.mode);
	ADD(local_visuals.arms_chams.primary);
	ADD(local_visuals.arms_chams.secondary);
	ADD(local_visuals.impacts);
	ADD(local_visuals.server_impacts);
	ADD(local_visuals.client_impacts);
	ADD(local_visuals.hit_indicator);
	ADD(local_visuals.hit_color);
	ADD(local_visuals.aa_indicator);
	ADD(local_visuals.clr_aa_real);
	ADD(local_visuals.clr_aa_desync);
	ADD(local_visuals.night_mode);
	ADD(local_visuals.disable_post_processing);
	ADD(local_visuals.night_mode_value);
	ADD(local_visuals.night_color);
	ADD(local_visuals.transparent_prop_value);
	ADD(local_visuals.aspect_changer);
	ADD(local_visuals.aspect);
	ADD(local_visuals.hitmarker_style);
	ADD(local_visuals.disable_bob);
	ADD(local_visuals.vm_fov);
	ADD(local_visuals.vm_fov_enable);
	ADD(local_visuals.vm_x);
	ADD(local_visuals.vm_y);
	ADD(local_visuals.vm_z);
	ADD(local_visuals.disable_bloom);

	ADD(world_esp.weapon);
	ADD(world_esp.objective);
	ADD(world_esp.nades);
	ADD(world_esp.visualize_nade_path);
	ADD(world_esp.nade_path_color);
	ADD(world_esp.objective_color);
	ADD(world_esp.weapon_color);
	ADD(world_esp.weapon_ammo_color);
	ADD(world_esp.fire_color);
	ADD(world_esp.smoke_color);
	ADD(world_esp.flash_color);
	ADD(world_esp.grenade_warning);
	ADD(world_esp.grenade_warning_options);
	ADD(world_esp.grenade_warning_progress);
	ADD(world_esp.grenade_warning_damage);
	ADD(world_esp.enemy_radar);

	ADD(misc.bhop);
	ADD(misc.peek_assistant);
	ADD(misc.peek_assistant_mode);
	ADD(misc.peek_assistant_color);
	ADD(misc.preserve_killfeed);
	ADD(misc.penetration_crosshair);
	ADD(misc.eventlog);
	ADD(misc.event_triggers);
	ADD(misc.autostrafe);
	ADD(misc.edge_jump);
	ADD(misc.clantag_changer);
	ADD(misc.auto_accept);
	ADD(misc.reveal_ranks);
	ADD(misc.menu_color);
	ADD(misc.nade_assistant);
	ADD(misc.enable_tick_sound);
	ADD(misc.autostrafe_turn_speed);
	ADD(misc.menu_movement);
	ADD(misc.bb_enable);
	ADD(misc.bb_primary);
	ADD(misc.bb_secondary);
	ADD(misc.bb_utilities);
	ADD(misc.avoid_collisions);
	ADD(misc.unlock_inventory);

	ADD(removals.no_visual_recoil);
	ADD(removals.no_flash);
	ADD(removals.no_smoke);
	ADD(removals.scope);
	ADD(removals.scope_color);

	ADD(lua.allow_insecure);
	ADD(lua.allow_dynamic_load);

	ADD(indicators.indicators);
	ADD(indicators.alpha);
	ADD(indicators.active_hotkeys_pos);
	ADD(indicators.indicators_pos);
	ADD(indicators.spectators_pos);
	ADD(indicators.bomb_pos);
}

void cfg_t::save(std::ostream &stream)
{
	stream.write(cfg_magic, 4);
	stream.write(cfg_game, 4);
	stream.write(reinterpret_cast<const char *>(&cfg_version), 4);

	for (const auto &[id, entry] : entries)
	{
		stream.write(reinterpret_cast<const char *>(&id), sizeof(id));
		entry->serialize(stream);
	}

	save_autoload_section(stream);
}

void cfg_t::save(const std::string &file)
{
	using namespace evo::gui;

	std::ofstream f(file, std::ios::binary);
	if (!f.is_open())
	{
		notify.add(std::make_shared<notification>(XOR("Save failed"), XOR("Could not open file")));
		EMIT_ERROR_SOUND();
		return;
	}

	save(f);

	notify.add(std::make_shared<notification>(XOR("Save succeeded"), XOR("Config has been saved")));
	EMIT_SUCCESS_SOUND();

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_config_save"),
					  [&](lua::state &s) -> int
					  {
						  s.push(file.c_str());
						  return 1;
					  });
#endif

	last_changed_config = file;
	is_dangerous = false;
}

bool cfg_t::load(std::istream &f)
{
	using namespace evo::gui;

	is_loading = true;

	try
	{
		f.seekg(0, std::istream::end);
		const auto sz = f.tellg();
		f.seekg(0, std::istream::beg);

		if (sz < 12)
		{
			is_loading = false;
			return false;
		}

		char magic[5]{};
		char game[5]{};
		uint32_t version{};
		f.read(magic, 4);
		f.read(game, 4);
		f.read(reinterpret_cast<char *>(&version), 4);

		if (strcmp(magic, cfg_magic) != 0)
		{
			notify.add(std::make_shared<notification>(XOR("Load failed"), XOR("Config file is incompatible")));
			EMIT_ERROR_SOUND();
			is_loading = false;
			return false;
		}

		if (strcmp(game, cfg_game) != 0)
		{
			notify.add(std::make_shared<notification>(XOR("Load failed"), XOR("Config is made for another game")));
			EMIT_ERROR_SOUND();
			is_loading = false;
			return false;
		}

		if (version != cfg_version)
		{
			notify.add(std::make_shared<notification>(XOR("Load failed"), XOR("Outdated config file")));
			EMIT_ERROR_SOUND();
			is_loading = false;
			return false;
		}

#ifdef CSGO_LUA
		lua::api.autoload.clear();
#endif

		// reset so we don't have any overlapping values (fix: hotkey bug)
		for (auto &e : entries)
			e.second->reset();

		while (!f.eof())
		{
			const auto cur_pos = f.tellg();
			if (cur_pos < 0)
				break;

			uint64_t sig_test{};
			f.read(reinterpret_cast<char *>(&sig_test), sizeof(sig_test));

			if (sig_test == autoload_magic)
			{
				load_autoload_section(f);
				break;
			}
			else
				f.seekg(cur_pos);

			uint64_t hash{};
			f.read(reinterpret_cast<char *>(&hash), sizeof(hash));

			// hash cannot be 0
			if (!hash)
				continue;

			if (entries.find(hash) == entries.end())
			{
				// peek type
				char type{};
				f.read(&type, 1);
				f.seekg(static_cast<size_t>(f.tellg()) - 1);

				// determine type and insert wherever we can
				value_entry *entry{nullptr};
				if (type == vt_bool)
				{
					unk_bool_entries[hash] = setting<bool>{};
					entry = &unk_bool_entries[hash];
				}
				else if (type == vt_float)
				{
					unk_float_entries[hash] = setting<float>{};
					entry = &unk_float_entries[hash];
				}
				else if (type == vt_bits)
				{
					unk_bit_entries[hash] = setting<bitset>{};
					entry = &unk_bit_entries[hash];
				}
				else if (type == vt_color)
				{
					unk_color_entries[hash] = setting<color>{};
					entry = &unk_color_entries[hash];
				}

				// if we could determine the type, deserialize entry and add it to entry list
				if (entry)
				{
					entry->deserialize(f);
					entries[hash] = entry;
				}
			}
			else
			{
				if (!entries[hash]->deserialize(f))
					continue;
			}
		}
	}
	catch (...)
	{
		EMIT_ERROR_SOUND();
		is_loading = false;
		drop();
		return false;
	}

	post_load();

	// handle changes for the gui
	for (auto &c : rage.weapon_cfgs)
		c.handle_on_anything_changed(true);

#ifdef CSGO_LUA
	std::thread(
		[]()
		{
			while (cfg.is_updating || lua::api.is_updating)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				std::this_thread::yield();
			}

			lua::api.run_autoload();
		})
		.detach();
#endif

	is_loading = false;
	return true;
}

bool cfg_t::load_from_string(const std::string &str)
{
	using namespace evo::gui;

	std::stringstream ss;
	ss << str;

	if (!load(ss))
		return false;

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_config_load"),
					  [&](lua::state &s) -> int
					  {
						  s.push(XOR("remote"));
						  return 1;
					  });
#endif

	notify.add(std::make_shared<notification>(XOR("Load succeeded"), XOR("Config has been loaded")));
	EMIT_SUCCESS_SOUND();

	is_dangerous = false;
	return true;
}

bool cfg_t::load(const std::string &file, bool welcome)
{
	using namespace evo::gui;

	std::ifstream f(file, std::ios::binary);
	if (!f.is_open())
	{
		notify.add(std::make_shared<notification>(XOR("Load failed"), XOR("Could not open file")));
		EMIT_ERROR_SOUND();
		is_loading = false;
		return false;
	}

	if (!load(f))
		return false;

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_config_load"),
					  [&](lua::state &s) -> int
					  {
						  s.push(file.c_str());
						  return 1;
					  });
#endif

	if (welcome)
		notify.add(std::make_shared<notification>(tinyformat::format(XOR("Welcome back, %s!"), ctx->user.username),
												  XOR("ev0lve is ready")));
	else
	{
		notify.add(std::make_shared<notification>(XOR("Load succeeded"), XOR("Config has been loaded")));
		EMIT_SUCCESS_SOUND();
	}

	last_changed_config = file;
	is_dangerous = false;
	return true;
}

void cfg_t::export_to_clipboard()
{
#ifndef CSGO_SECURE
	return;
#else
	using namespace evo::gui;
	std::stringstream ss;
	save(ss);

	clipboard::set(evo::util::base64_encode(ss.str()));

	notify.add(std::make_shared<notification>(XOR("Export succeeded"), XOR("Config has been exported")));
	EMIT_SUCCESS_SOUND();
#endif
}

void cfg_t::import_from_clipboard()
{
#ifndef CSGO_SECURE
	return;
#else
	using namespace evo::gui;
	std::stringstream ss;

	try
	{
		ss << evo::util::base64_decode(clipboard::get());
	}
	catch (...)
	{
		notify.add(std::make_shared<notification>(XOR("Import failed"), XOR("Invalid config data")));
		EMIT_ERROR_SOUND();
		return;
	}

	if (!load(ss))
		return;

	notify.add(std::make_shared<notification>(XOR("Import succeeded"), XOR("Config has been imported")));
	EMIT_SUCCESS_SOUND();
#endif
}

void cfg_t::drop()
{
	is_loading = true;

	for (auto &e : entries)
		e.second->reset();

	post_load();

	is_loading = false;
	is_dangerous = true;
}

void cfg_t::post_load()
{
	using namespace evo::gui;
	ctx->reset();
	colors.accent = misc.menu_color.get();
}

bool cfg_t::register_lua_bool_entry(uint64_t id)
{
	const auto entry_exists = entries.find(id) != entries.end();

	// check if it already exists
	if (unk_bool_entries.find(id) != unk_bool_entries.end())
	{
		if (entry_exists)
			return true;
	}
	else
	{
		// attempting to override internal value
		if (entry_exists)
			return false;

		unk_bool_entries[id] = setting<bool>{};
	}

	// add to entry list
	entries[id] = &unk_bool_entries[id];
	return true;
}

bool cfg_t::register_lua_float_entry(uint64_t id)
{
	const auto entry_exists = entries.find(id) != entries.end();

	// check if it already exists
	if (unk_float_entries.find(id) != unk_float_entries.end())
	{
		if (entry_exists)
			return true;
	}
	else
	{
		// attempting to override internal value
		if (entry_exists)
			return false;

		unk_float_entries[id] = setting<float>{};
	}

	// add to entry list
	entries[id] = &unk_float_entries[id];
	return true;
}

bool cfg_t::register_lua_bit_entry(uint64_t id)
{
	const auto entry_exists = entries.find(id) != entries.end();

	// check if it already exists
	if (unk_bit_entries.find(id) != unk_bit_entries.end())
	{
		if (entry_exists)
			return true;
	}
	else
	{
		// attempting to override internal value
		if (entry_exists)
			return false;

		unk_bit_entries[id] = setting<bitset>{};
	}

	// add to entry list
	entries[id] = &unk_bit_entries[id];
	return true;
}

bool cfg_t::register_lua_color_entry(uint64_t id)
{
	const auto entry_exists = entries.find(id) != entries.end();

	// check if it already exists
	if (unk_color_entries.find(id) != unk_color_entries.end())
	{
		if (entry_exists)
			return true;
	}
	else
	{
		// attempting to override internal value
		if (entry_exists)
			return false;

		unk_color_entries[id] = setting<color>{};
	}

	// add to entry list
	entries[id] = &unk_color_entries[id];
	return true;
}

void cfg_t::save_autoload_section(std::ostream &stream)
{
#ifdef CSGO_LUA
	uint64_t section{autoload_magic};
	stream.write(reinterpret_cast<char *>(&section), sizeof(section));

	uint32_t count{lua::api.autoload.size()};
	stream.write(reinterpret_cast<char *>(&count), sizeof(count));

	for (auto &e : lua::api.autoload)
	{
		uint32_t id{e};
		stream.write(reinterpret_cast<char *>(&id), sizeof(id));
	}
#endif
}

void cfg_t::load_autoload_section(std::istream &stream)
{
#ifdef CSGO_LUA
	uint32_t count{};
	stream.read(reinterpret_cast<char *>(&count), sizeof(count));

	while (count--)
	{
		uint32_t id{};
		stream.read(reinterpret_cast<char *>(&id), sizeof(id));

		lua::api.autoload.emplace_back(id);
	}
#endif
}

cfg_t cfg{};
