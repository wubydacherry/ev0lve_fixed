
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>
#include <hacks/antiaim.h>

using namespace sdk;
using namespace detail;

namespace hooks::cs_player
{
void __fastcall physics_simulate(cs_player_t *player, uint32_t edx)
{
	auto &context = player->get_command_context();
	const auto needs_processing =
		context.needs_processing && player->get_simulation_tick() != game->globals->tickcount && player->is_alive();

	if (context.cmd.tick_count == INT_MAX)
	{
		auto &current = pred.info[context.cmd_number % input_max].animation;
		auto &before = pred.info[(context.cmd_number - 1) % input_max].animation;
		const auto backup = animation_copy(0, player);
		before.restore(player);
		local_record.addon.vm = 0;
		current = animation_copy(context.cmd_number, player);
		backup.restore(player);
		return;
	}

	const auto viewmodel = reinterpret_cast<entity_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_view_model_0()));
	const auto view_time = viewmodel ? viewmodel->get_anim_time() : 0.f;
	const auto fov_time = player->get_fovtime();

	hook_manager.physics_simulate->call(player, edx);

	if (viewmodel && viewmodel->get_anim_time() != view_time)
		viewmodel->get_anim_time() = TICKS_TO_TIME(context.cmd_number);

	if (fov_time != player->get_fovtime())
		player->get_fovtime() = TICKS_TO_TIME(context.cmd_number);

	if (needs_processing)
	{
		const auto hdr = player->get_studio_hdr();
		if (pred.real_hdr)
			player->get_studio_hdr() = pred.real_hdr;

		auto &info = pred.info[(context.cmd.command_number - 1) % input_max];
		const auto s = player->get_anim_state();
		if (context.cmd.command_number > game->client_state->last_command && (context.cmd.buttons & user_cmd::jump) &&
			!(player->get_flags() & cs_player_t::on_ground) && info.sequence == context.cmd.command_number - 1 &&
			info.flags & cs_player_t::on_ground)
		{
			player_list.build_activity_modifiers(player, local_record.addon.activity_modifiers);
			player->try_initiate_animation(4, XOR_32(985), local_record.addon.activity_modifiers,
										   context.cmd.command_number);
		}

		player->get_thirdperson_recoil() = player->get_punch_angle_scaled().x;

		if (game->rules->data)
		{
			if (!(player->get_flags() & cs_player_t::ducking) && !player->get_ducking() && !player->get_ducked())
				player->get_view_offset() = game->rules->data->get_view_vectors()->view;
			else if (player->get_ducked() && !player->get_ducking())
				player->get_view_offset() = game->rules->data->get_view_vectors()->duck_view;
		}

		const auto fresh = game->input->verified_commands[context.cmd_number % input_max].crc ==
						   *reinterpret_cast<int32_t *>(&game->globals->interval_per_tick);
		if (fresh && context.cmd.buttons & (user_cmd::attack | user_cmd::attack2 | user_cmd::reload))
			player->get_is_looking_at_weapon() = player->get_is_holding_look_at_weapon() = false;

		const auto sequence = player->get_sequence();
		const auto cycle = player->get_cycle();
		const auto playback_rate = player->get_playback_rate();
		player_list.merge_local_animation(player, &context.cmd);

		player->get_sequence() = sequence;
		player->get_cycle() = cycle;
		player->get_playback_rate() = playback_rate;

		if (fresh)
		{
			const auto curtime = game->globals->curtime;
			game->globals->curtime = pred.backup_curtime;
			hook_manager.do_animation_events_overlay->call(player, edx, player->get_studio_hdr());
			game->globals->curtime = curtime;
		}

		const auto wpn = reinterpret_cast<weapon_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
		const auto world_model =
			wpn ? reinterpret_cast<entity_t *>(
					  game->client_entity_list->get_client_entity_from_handle(wpn->get_weapon_world_model()))
				: nullptr;

		if (info.sequence == context.cmd.command_number - 1 && info.ev == XOR_32(52) && world_model)
			world_model->get_effects() &= ~ef_nodraw;

		// failsafe to show active world model if it fails to unhide by the time deploy is complete
		if (fresh && world_model)
		{
			const auto act = wpn->get_sequence_activity(wpn->get_sequence());
			if (act != XOR_32(183) && act != XOR_32(224))
				world_model->get_effects() &= ~ef_nodraw;
		}

		if (fresh)
			player_list.update_addon_bits(player);

		player->get_studio_hdr() = hdr;
	}
}

void __fastcall post_data_update(cs_player_t *player, uint32_t edx, uint32_t type)
{
	const auto p = reinterpret_cast<cs_player_t *>(uintptr_t(player) - 8);

#ifndef NDEBUG
	if (game->dbg.treat_bots_as_real.get() && p->index() < game->client_state->get_user_info_table()->get_num_strings())
		reinterpret_cast<player_info *>(
			game->client_state->get_user_info_table()->get_string_user_data(p->index() - 1, nullptr))
			->fakeplayer = false;
#endif

	player_list.on_update_end(p);

	if (p->is_enemy() && cfg.world_esp.enemy_radar.get() &&
		!(game->rules->data && game->rules->data->get_is_valve_ds()))
		p->get_spotted() = true;

//	if (!cfg.rage.enable.get() && p->index() != game->engine_client->get_local_player())
//		return hook_manager.post_data_update->call(player, edx, type);

	*p->get_old_origin() = p->get_origin_pred();
	*reinterpret_cast<angle *>(uintptr_t(p->get_old_origin()) + sizeof(vec3)) = p->get_rotation_pred();

	const auto back = player->get_client_side_animation();
	if (true || player->index() == game->engine_client->get_local_player())
		player->get_client_side_animation() = false;
	hook_manager.post_data_update->call(player, edx, type);
	player->get_client_side_animation() = back;
}

void __fastcall do_animation_events(cs_player_t *player, uint32_t edx, cstudiohdr *hdr)
{
	if (!player->get_predictable())
		hook_manager.do_animation_events_overlay->call(player, edx, hdr);
}

void __fastcall calc_view(cs_player_t *player, uint32_t edx, vec3 *eye, angle *ang, float *znear, float *zfar,
						  float *fov)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	const auto iron_sight = wpn ? &wpn->get_iron_sight_controller() : nullptr;
	const auto bak_iron_sight = iron_sight ? *iron_sight : 0;

	if (cfg.local_visuals.fov_changer.get() && iron_sight)
		*iron_sight = 0;

	auto &state = player->get_anim_state();
	const auto bak_state = state;
	const auto bak_view_z = player->get_view_offset().z;
	state = nullptr;
	if ((true || cfg.antiaim.enable.get()) && player == game->local_player)
		player->get_view_offset().z = hacks::aa.get_visual_eye_position().z - player->get_abs_origin().z;
	hook_manager.calc_view->call(player, edx, eye, ang, znear, zfar, fov);
	state = bak_state;
	player->get_view_offset().z = bak_view_z;

	if (cfg.removals.no_visual_recoil.get())
	{
		const auto org = player->eye_angles();
		ang->x = org->x;
		ang->y = org->y;
	}

	if (cfg.local_visuals.fov_changer.get() && iron_sight)
		*iron_sight = bak_iron_sight;
}

void __fastcall fire_event(cs_player_t *player, uint32_t edx, vec3 *origin, angle *angles, int event,
						   const char *options)
{
	hook_manager.fire_event->call(player, edx, origin, angles, event, options);

	if (player->index() == game->engine_client->get_local_player())
	{
		auto &context = player->get_command_context();
		auto &info = pred.info[(context.cmd.command_number - 1) % input_max];
		if (info.sequence == context.cmd.command_number - 1)
			info.ev = event;
	}
}

void __fastcall obb_change_callback(cs_player_t *player, uint32_t edx, vec3 *old_mins, vec3 *mins, vec3 *old_maxs,
									vec3 *maxs)
{
	if (game->cs_game_movement->is_processing)
		return;

	auto &entry = GET_PLAYER_ENTRY(player);

	if (player->get_predictable() && game->prediction->is_active())
	{
		player->get_view_height() = player->get_origin_pred().z + old_maxs->z;
		player->get_bounds_change_time() = game->globals->curtime;
		entry.view_delta = old_maxs->z;
		if (pred.is_computing())
			entry.smooth_bounds_change_time = pred.backup_curtime;
	}
	else if (!player->get_predictable())
	{
		entry.old_maxs_z = old_maxs->z;

		hook_manager.obb_change_callback->call(player, edx, old_mins, mins, old_maxs, maxs);

		entry.bones.view_height = player->get_view_height();
		entry.bones.bounds_change_time = player->get_bounds_change_time();
		entry.bones.obb_maxs = *maxs;
	}
}

void __fastcall reevauluate_anim_lod(cs_player_t *player, uint32_t edx, int32_t bone_mask)
{
	// stubbed this.
}

double __fastcall get_fov(cs_player_t *player, uintptr_t edx)
{
	constexpr auto default_fov = 90;

	if (game->prediction->is_active())
		return hook_manager.get_fov->call(player, edx);

	if (game->client.at(functions::return_to_should_draw_viewmodel) == uintptr_t(_ReturnAddress()))
		return cfg.local_visuals.viewmodel_in_scope.get() ? default_fov : -1.f;

	const auto backup_fov = player->get_fov();
	const auto backup_fov_start = player->get_fovstart();

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	if (cfg.local_visuals.fov_changer.get() && wpn)
	{
		const auto info = wpn->get_cs_weapon_data();

		auto remap_fov = [&](int32_t in)
		{
			auto changed_fov = default_fov + static_cast<int32_t>(cfg.local_visuals.fov.get());

			if (in == default_fov || in == 0 ||
				in == info->zoom_fov1 && cfg.local_visuals.skip_first_scope.get() && info->zoom_levels > 1)
				return changed_fov;

			auto delta = changed_fov - in;

			if (in == info->zoom_fov2 || (in == info->zoom_fov1 && !cfg.local_visuals.skip_first_scope.get()))
				return changed_fov - static_cast<int32_t>(delta * cfg.local_visuals.fov2.get() / 100.f);

			return in;
		};

		player->get_fov() = remap_fov(player->get_fov());
		player->get_fovstart() = remap_fov(player->get_fovstart());
	}

	const auto ret = hook_manager.get_fov->call(player, edx);
	player->get_fov() = backup_fov;
	player->get_fovstart() = backup_fov_start;
	return ret;
}

int32_t __fastcall get_default_fov(cs_player_t *player, uintptr_t edx)
{
	const auto org = hook_manager.get_default_fov->call(player, edx);

	if (game->prediction->is_active() || !cfg.local_visuals.fov_changer.get())
		return org;

	return org + static_cast<int32_t>(cfg.local_visuals.fov.get());
}

void __fastcall update_clientside_animation(cs_player_t *player, uint32_t edx)
{
	const auto state = player->get_anim_state();
	if (state && (true || player->index() == game->engine_client->get_local_player()))
	{
		state->player = nullptr;
		hook_manager.update_clientside_animation->call(player, edx);
		state->player = player;
	}
	else
		hook_manager.update_clientside_animation->call(player, edx);
}

#ifdef CSGO_DEBUG_MODIFY_EYE_POSITION
void __fastcall modify_eye_position_server(uintptr_t state, uint32_t edx, vec3 *eye_position)
{
	if (game->local_player)
	{
		const auto setup_bones =
			((void(__thiscall *)(uintptr_t, mat3x4 *, uint32_t))game->server.at(functions::server::setup_bones));
		alignas(16) mat3x4 bones[max_bones]{};
		setup_bones(*(uintptr_t *)(state + 0x50), bones, bone_used_by_hitbox | bone_used_by_attachment);
		const auto hdr = game->local_player->get_studio_hdr();
		if (pred.real_hdr)
			game->local_player->get_studio_hdr() = pred.real_hdr;
		debug->draw_player(game->local_player, bones, sdk::color(0, 0, 255, 130), 4.f);
		game->local_player->get_studio_hdr() = hdr;
	}

	hook_manager.modify_eye_position_server->call(state, edx, eye_position);
}
#endif

#ifndef NDEBUG
bool __fastcall is_bot(uintptr_t bot, uint32_t edx) { return false; }

void __fastcall run_command_server(uintptr_t move, uint32_t edx, uintptr_t player, user_cmd *cmd, uintptr_t move_helper)
{
	const auto get_entity = ((uintptr_t(__fastcall *)(int32_t))game->server.at(functions::server::get_entity));
	const auto setup_bones =
		((void(__thiscall *)(uintptr_t, mat3x4 *, uint32_t))game->server.at(functions::server::setup_bones));

	const auto run_cmd = [&](user_cmd *c)
	{
		auto &vtable = *(uintptr_t *)(player);
		const auto org_vtable = vtable;
		if (game->dbg.treat_bots_as_real.get())
		{
			uintptr_t our_table[1024];
			memcpy(our_table, (uintptr_t *)(vtable - sizeof(uintptr_t) * 4), sizeof(our_table));
			our_table[509 + 4] = uintptr_t(is_bot);
			vtable = uintptr_t(&our_table[4]);
		}
		hook_manager.run_command_server->call(move, edx, player, c, move_helper);
		vtable = org_vtable;
	};

	if (game->dbg.allow_fakelag_on_bot.get() && get_entity(2) == player)
	{
		static uint32_t last_cmd{}, count_cmd{};

		const auto bot_mimic_yaw_offset = game->cvar->find_var(XOR_STR("bot_mimic_yaw_offset"));
		bot_mimic_yaw_offset->set_float(0.f);

		if (game->client_state->last_command == last_cmd)
		{
			count_cmd++;
			return;
		}

		for (auto i = 0; i < count_cmd + 1; i++)
		{
			const auto &other = game->input->commands[(last_cmd + 1 + i) % input_max];
			uint8_t tmp[0x68];
			auto *tmpc = (user_cmd *)tmp;
			memcpy((void *)tmpc, (void *)cmd, sizeof(tmp));
			tmpc->command_number = other.command_number;
			tmpc->tick_count = other.tick_count;
			tmpc->viewangles = other.viewangles;
			tmpc->forwardmove = other.forwardmove;
			tmpc->sidemove = other.sidemove;
			tmpc->upmove = other.upmove;
			tmpc->buttons = other.buttons;
			tmpc->weapon_select = other.weapon_select;
			tmpc->weapon_select_subtype = other.weapon_select_subtype;
			// adjust tickbase.
			*reinterpret_cast<int32_t *>(player + 0x10C8) =
				pred.info[tmpc->command_number % input_max].tickbase.base - 1;
			run_cmd(tmpc);
		}

		last_cmd = game->client_state->last_command;
		count_cmd = 0;
	}
	else
		run_cmd(cmd);

	auto index = -1;
	for (auto i = 0; i < 65; i++)
		if (get_entity(i) == player)
		{
			index = i;
			break;
		}

	if (game->dbg.visualized_matrices_index.get() == index)
	{
		if (index == game->engine_client->get_local_player())
		{
			auto &shared = game->dbg.shared_data[cmd->command_number % input_max];
			if (shared.idx != cmd->command_number)
			{
				shared.idx = cmd->command_number;
				shared.flags = 0;
			}

			if (shared.flags == 1)
			{
				setup_bones(player, shared.server_mat, bone_used_by_hitbox | bone_used_by_attachment);
				shared.flags |= 2;

				const auto client_player =
					reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(index));

				if (!client_player)
					return;

				const auto time = TICKS_TO_TIME(2 + shared.lag);
				const auto hdr = client_player->get_studio_hdr();
				if (pred.real_hdr)
					client_player->get_studio_hdr() = pred.real_hdr;
				debug->draw_player(client_player, shared.server_mat, sdk::color(0, 0, 255, 130), time);
				debug->draw_player(client_player, shared.client_mat, sdk::color(0, 255, 0, 130), time);
				client_player->get_studio_hdr() = hdr;
			}
		}
		else
		{
			const auto tickbase = *reinterpret_cast<uint32_t *>(player + 4296) - 1;
			auto &shared = game->dbg.shared_data[tickbase % input_max];
			if (shared.idx != tickbase)
			{
				shared.idx = tickbase;
				shared.flags = 0;
			}

			if (!shared.flags)
			{
				setup_bones(player, shared.server_mat, bone_used_by_hitbox | bone_used_by_attachment);
				shared.flags |= 2;
			}
		}
	}
}
#endif
} // namespace hooks::cs_player