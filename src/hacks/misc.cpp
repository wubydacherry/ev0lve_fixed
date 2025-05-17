
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/player_list.h>
#include <hacks/antiaim.h>
#include <hacks/misc.h>
#include <sdk/client_entity_list.h>
#include <sdk/client_state.h>
#include <sdk/cs_player_resource.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_client.h>
#include <sdk/engine_sound.h>
#include <sdk/engine_trace.h>
#include <sdk/game_rules.h>
#include <sdk/misc.h>
#include <sdk/model_render.h>
#include <util/mem_guard.h>
#include <util/misc.h>

using namespace sdk;
using namespace detail;
using namespace util;
using namespace hacks;

namespace hacks
{
namespace misc
{
std::vector<typedescription_t> player_types{}, grenade_types{};
typedescription_t *old_player_types = nullptr, *old_grenade_types = nullptr;
uint32_t old_player_num_field{}, old_grenade_num_field{};
int32_t last_reliable_state{};

void on_post_data()
{
	static auto is_overriding_skybox = false;
	const auto active = cfg.local_visuals.night_mode.get();

#if 1
	game->client_entity_list->for_each(
		[&active](client_entity *const ent)
		{
			if (ent->is_player() || ent->get_class_id() == class_id::csragdoll)
				game->leaf_system->renderable_changed(ent->render_handle());

			if (ent->get_class_id() != class_id::sun)
				return;

			const auto sun = reinterpret_cast<entity_t *>(ent);
			sun->get_on() = !active;
			sun->get_size() = active ? 0 : 16;
		});

	if (!game->engine_client->is_connected() || !game->engine_client->is_ingame())
		is_overriding_skybox = !active;
	else if (is_overriding_skybox != active)
	{
		is_overriding_skybox = active;

		((void(__fastcall *)(char const *))game->engine.at(functions::load_named_sky))(
			active ? XOR_STR("sky_csgo_night02") : GET_CONVAR_STR("sv_skyname"));
	}
#endif

	const auto player_map = reinterpret_cast<datamap_t *>(game->client.at(offsets::predmaps::csplayer));
	const auto grenade_map = reinterpret_cast<datamap_t *>(game->client.at(offsets::predmaps::base_csgrenade));
#if 0
	if (player_map->data_desc != player_types.data())
	{
		old_player_types = player_map->data_desc;
		old_player_num_field = player_map->data_num_fields;

		player_types.assign(player_map->data_desc, player_map->data_desc + player_map->data_num_fields);
		player_types.emplace_back(
			field_boolean, reinterpret_cast<recv_prop *>(game->client.at(props::csplayer::wait_for_no_attack))->name,
			offsets::csplayer::wait_for_no_attack, 1);
		player_types.emplace_back(field_integer,
								  reinterpret_cast<recv_prop *>(game->client.at(props::csplayer::move_state))->name,
								  offsets::csplayer::move_state, 4);
		player_types.emplace_back(field_boolean,
								  reinterpret_cast<recv_prop *>(game->client.at(props::csplayer::strafing))->name,
								  offsets::csplayer::strafing, 1);
		player_types.emplace_back(
			field_float,
			reinterpret_cast<recv_prop *>(game->client.at(props::cslocal_player_exclusive::velocity_modifier))->name,
			offsets::cslocal_player_exclusive::velocity_modifier, 4, game->globals->interval_per_tick / 2.5f);
		player_types.emplace_back(
			field_float, reinterpret_cast<recv_prop *>(game->client.at(props::csplayer::thirdperson_recoil))->name,
			offsets::csplayer::thirdperson_recoil, 4, .01f);

		player_map->data_desc = player_types.data();
		player_map->data_num_fields = player_types.size();
		player_map->optimized_map = nullptr;
	}


		if (grenade_map->data_desc != grenade_types.data())
	{
		old_grenade_types = grenade_map->data_desc;
		old_grenade_num_field = grenade_map->data_num_fields;

		grenade_types.assign(grenade_map->data_desc, grenade_map->data_desc + grenade_map->data_num_fields);
		grenade_types.emplace_back(
			field_boolean,
			reinterpret_cast<recv_prop *>(game->client.at(props::base_csgrenade::is_held_by_player))->name,
			offsets::base_csgrenade::is_held_by_player, 1);
		grenade_types.emplace_back(
			field_float, reinterpret_cast<recv_prop *>(game->client.at(props::base_csgrenade::throw_time))->name,
			offsets::base_csgrenade::throw_time, 4, .01f);

		grenade_map->data_desc = grenade_types.data();
		grenade_map->data_num_fields = grenade_types.size();

		for (auto &el : std::array{offsets::predmaps::decoy_grenade, offsets::predmaps::hegrenade,
								   offsets::predmaps::molotov_grenade, offsets::predmaps::sensor_grenade,
								   offsets::predmaps::smoke_grenade, offsets::predmaps::flashbang})
			reinterpret_cast<datamap_t *>(game->client.at(el))->optimized_map = nullptr;
	}

#endif
}

void on_exit()
{
	const auto player_map = reinterpret_cast<datamap_t *>(game->client.at(offsets::predmaps::csplayer));
	const auto grenade_map = reinterpret_cast<datamap_t *>(game->client.at(offsets::predmaps::base_csgrenade));

	if (player_map->data_desc == player_types.data())
	{
		player_map->data_desc = old_player_types;
		player_map->data_num_fields = old_player_num_field;
		player_map->optimized_map = nullptr;
	}

	if (grenade_map->data_desc == grenade_types.data())
	{
		grenade_map->data_desc = old_grenade_types;
		grenade_map->data_num_fields = old_grenade_num_field;
		for (auto &el : std::array{offsets::predmaps::decoy_grenade, offsets::predmaps::hegrenade,
								   offsets::predmaps::molotov_grenade, offsets::predmaps::sensor_grenade,
								   offsets::predmaps::smoke_grenade, offsets::predmaps::flashbang})
			reinterpret_cast<datamap_t *>(game->client.at(el))->optimized_map = nullptr;
	}
}

void on_frame_stage_notify(const framestage stage)
{
#if 0
	if (stage == frame_render_start)
	{
		const auto unload_trigger = "C:\\Users\\User\\dev\\unload";
		if (GetFileAttributes(unload_trigger) != INVALID_FILE_ATTRIBUTES)
		{
			DeleteFileA(unload_trigger);
			std::thread(
				[]()
				{
					game->deinit();
					game = nullptr;
				})
				.detach();
		}
	}
#endif

	if (stage == frame_net_update_end)
		for (auto i = game->client_state->command_ack; i <= game->client_state->last_command; i++)
			pred.info[i % input_max].animation.checksum = 0;

	if (stage == frame_render_start && game->local_player && game->local_player->get_anim_state())
	{
		const auto back_sequence = game->local_player->get_sequence();
		game->local_player->get_sequence() = -1;
		game->local_player->get_anim_state()->player = nullptr;
		hook_manager.update_clientside_animation->call(game->local_player, 0);
		game->local_player->get_anim_state()->player = game->local_player;
		game->local_player->get_sequence() = back_sequence;
	}
}

void on_override_view()
{
	static const auto blur_overlay = game->material_system->find_material(XOR_STR("dev/scope_bluroverlay"));
	static const auto lens_dirt =
		game->material_system->find_material(XOR_STR("models/weapons/shared/scope/scope_lens_dirt"));

	const auto post_process = cfg.local_visuals.disable_post_processing.get();
	SET_CONVAR_INT("cl_csm_shadows", !post_process);
	blur_overlay->set_material_var_flag(material_var_no_draw, post_process);
	lens_dirt->set_material_var_flag(material_var_no_draw, post_process);
	if (post_process)
		reinterpret_cast<float *>(game->client.at(globals::post_processing_params))[5] = 0.f;

	if (cfg.local_visuals.night_mode.get())
	{
		const auto target = cfg.local_visuals.night_color.get();
		SET_CONVAR_FLOAT("mat_ambient_light_r", target.value.r * cfg.local_visuals.night_mode_value.get() * .01f);
		SET_CONVAR_FLOAT("mat_ambient_light_g", target.value.g * cfg.local_visuals.night_mode_value.get() * .01f);
		SET_CONVAR_FLOAT("mat_ambient_light_b", target.value.b * cfg.local_visuals.night_mode_value.get() * .01f);
	}
	else
	{
		SET_CONVAR_FLOAT("mat_ambient_light_r", 0.f);
		SET_CONVAR_FLOAT("mat_ambient_light_g", 0.f);
		SET_CONVAR_FLOAT("mat_ambient_light_b", 0.f);
	}

	// LOL
	static const auto vm_fov = game->cvar->find_var(XOR_STR("viewmodel_fov"));
	vm_fov->flags &= ~FCVAR_ARCHIVE;
	vm_fov->orig_flags = vm_fov->flags | 1;
	if (cfg.local_visuals.vm_fov_enable.get())
	{
		if (vm_fov->get_float() != cfg.local_visuals.vm_fov.get())
			vm_fov->set_float(cfg.local_visuals.vm_fov.get());
	}
	else
	{
		if (vm_fov->get_float() != 60.f)
			vm_fov->set_float(60.f);
	}

	static const auto vm_x = game->cvar->find_var(XOR_STR("viewmodel_offset_x"));
	vm_x->flags &= ~FCVAR_ARCHIVE;
	vm_x->orig_flags = vm_x->flags | 1;
	static const auto vm_y = game->cvar->find_var(XOR_STR("viewmodel_offset_y"));
	vm_y->flags &= ~FCVAR_ARCHIVE;
	vm_y->orig_flags = vm_y->flags | 1;
	static const auto vm_z = game->cvar->find_var(XOR_STR("viewmodel_offset_z"));
	vm_z->flags &= ~FCVAR_ARCHIVE;
	vm_z->orig_flags = vm_z->flags | 1;

	if (vm_x->get_float() != cfg.local_visuals.vm_x.get())
		vm_x->set_float(cfg.local_visuals.vm_x.get());
	if (vm_y->get_float() != cfg.local_visuals.vm_y.get())
		vm_y->set_float(cfg.local_visuals.vm_y.get());
	if (vm_z->get_float() != cfg.local_visuals.vm_z.get())
		vm_z->set_float(cfg.local_visuals.vm_z.get());
}

void on_before_move()
{
	static const auto ref_cl_predict = game->cvar->find_var(XOR_STR("cl_predict"));
	static const auto ref_cl_lagcompensation = game->cvar->find_var(XOR_STR("cl_lagcompensation"));
	static auto rtt_ticks = max_new_cmds;
	static auto previous_team = 1;

	if (game->globals->tickcount % max(max_new_cmds, rtt_ticks) != 0)
		return;

	// switch us to spectator when we need a change.
	if ((cfg.rage.mode.get().test(cfg_t::rage_just_in_time) && !(game->cl_predict && game->cl_lagcompensation)) ||
		(cfg.rage.mode.get().test(cfg_t::rage_ahead_of_time) && !(game->cl_predict && !game->cl_lagcompensation)))
	//|| (cfg.rage.mode.get().test(cfg_t::rage_ahead_of_time_fallback) && !(!cl_predict && cl_lagcompensation)))
	{
		const auto local = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));
		if (!local || local->get_team_num() == 2 || local->get_team_num() == 3)
		{
			previous_team = local ? local->get_team_num() : 1;
			game->engine_client->clientcmd(XOR_STR("jointeam 1"));
		}
		else
		{
			if (cfg.rage.mode.get().test(cfg_t::rage_just_in_time))
			{
				ref_cl_predict->enforce_sent_value(game->cl_predict, true);
				ref_cl_lagcompensation->enforce_sent_value(game->cl_lagcompensation, true);
			}
			else if (cfg.rage.mode.get().test(cfg_t::rage_ahead_of_time))
			{
				ref_cl_predict->enforce_sent_value(game->cl_predict, true);
				ref_cl_lagcompensation->enforce_sent_value(game->cl_lagcompensation, false);
			}
			/*else if (cfg.rage.mode.get().test(cfg_t::rage_ahead_of_time_fallback))
			{
				ref_cl_predict->enforce_sent_value(game->cl_predict, false);
				ref_cl_lagcompensation->enforce_sent_value(game->cl_lagcompensation, true);
			}*/
		}
	}
	else
	{
		switch (previous_team)
		{
		case 2:
			game->engine_client->clientcmd(XOR_STR("jointeam 2 1"));
			break;
		case 3:
			game->engine_client->clientcmd(XOR_STR("jointeam 3 1"));
			break;
		default:
			break;
		}

		previous_team = 1;
		rtt_ticks = 2 * TIME_TO_TICKS(calculate_rtt());
	}
}

void on_create_move(user_cmd *const cmd)
{
	// unlock client command buffer.
	const auto addr = reinterpret_cast<uint32_t *>(game->engine.at(globals::patch_send_move_cmds + 1));
	if (*addr != 24)
	{
		mem_guard guard(PAGE_EXECUTE_READWRITE, addr, sizeof(uint8_t));
		*addr = XOR_32(24);
	}

	// set bullrush meme.
	if (!game->rules->data->get_is_valve_ds())
		cmd->buttons |= user_cmd::bull_rush;

	if (!game->engine_client->is_ingame() || !(cmd->buttons & user_cmd::score) || !cfg.misc.reveal_ranks.get())
		return;

	float array[3] = {0, 0, 0};
	reinterpret_cast<bool (*)(float *)>(game->client.at(functions::reveal_rank))(array);
}

void on_sound(start_sound_params *params, const char *name)
{
	static const auto competitive_accept_beep = XOR_STR_STORE("competitive_accept_beep");
	static const auto weapons = XOR_STR_STORE("weapons");
	static const auto auto_semiauto_switch = XOR_STR_STORE("auto_semiauto_switch");
	static const auto player_footsteps = XOR_STR_STORE("player\\footsteps");
	static const auto player_land = XOR_STR_STORE("\\land");

	if (!game->engine_client->is_ingame())
	{
		/* autoaccept code is commented since there is no matchmaking anymore */
		//XOR_STR_STACK(beep, competitive_accept_beep);
		//if (cfg.misc.auto_accept.get() && strstr(name, beep))
		//	reinterpret_cast<void(__stdcall *)(const char *)>(game->client.at(functions::accept_match))("");

		return;
	}

	const auto ent =
		reinterpret_cast<cs_player_t *const>(game->client_entity_list->get_client_entity(params->sound_source));

	auto player = ent;
	if (player && !player->is_player())
	{
		const auto parent = player->get_shadow_parent();
		const auto unknown = parent != nullptr ? parent->get_client_unknown() : nullptr;
		const auto owner =
			unknown != nullptr ? reinterpret_cast<cs_player_t *const>(unknown->get_client_entity()) : nullptr;

		if (owner && owner->is_player())
			player = owner;
	}

	if (!player || !player->is_player() || !player->is_dormant())
		return;

	XOR_STR_STACK(wpns, weapons);
	XOR_STR_STACK(footsteps, player_footsteps);
	XOR_STR_STACK(land, player_land);

	const auto wpn = strstr(name, wpns);
	const auto lnd = strstr(name, land);
	const auto step = strstr(name, footsteps);
	auto origin = params->origin;
	auto &entry = GET_PLAYER_ENTRY(player);

	if (lnd)
		origin.z -= 32.f;

	if (wpn)
		entry.dormant_miss = 0;

	ray r{};
	r.init(origin + vec3(0.f, 0.f, 10.f),
		   origin - vec3(0.f, 0.f, (game->rules->data ? game->rules->data->get_view_vectors()->view.z : 0.f) + 10.f));
	trace_simple_filter filter(player);
	game_trace tr{};
	game->engine_trace->trace_ray(r, mask_playersolid, &filter, &tr);
	if (tr.fraction < 1.f)
		origin = tr.endpos;

	if (entry.visuals.last_update != TIME_TO_TICKS(game->engine_client->get_last_timestamp()) && (wpn || lnd || step))
	{
		if ((origin - entry.visuals.pos.current).length() <= 512.f)
			entry.visuals.pos.update(origin, .05f);
		else
			entry.visuals.pos.current = entry.visuals.pos.target = origin;
		entry.visuals.last_update = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
		entry.visuals.alpha = .3f;
	}
}

void on_round_start()
{
	// reset garbage
	for (auto &entry : player_list.entries)
		entry.hack = hack_kind_none;

	if (!cfg.misc.bb_enable.get())
		return;

	const auto primary = cfg.misc.bb_primary.get();
	const auto secondary = cfg.misc.bb_secondary.get();
	const auto utilities = cfg.misc.bb_utilities.get();

	std::string cmd;
	if (primary.test(cfg_t::buybot_primary_auto))
		cmd += XOR_STR("buy scar20;");
	else if (primary.test(cfg_t::buybot_primary_scout))
		cmd += XOR_STR("buy ssg08;");
	else if (primary.test(cfg_t::buybot_primary_awp))
		cmd += XOR_STR("buy awp;");

	if (secondary.test(cfg_t::buybot_secondary_duals))
		cmd += XOR_STR("buy elite;");
	else if (secondary.test(cfg_t::buybot_secondary_deagle))
		cmd += XOR_STR("buy deagle;");
	else if (secondary.test(cfg_t::buybot_secondary_tec))
		cmd += XOR_STR("buy tec9;");

	if (utilities.test(cfg_t::buybot_utilities_armor))
		cmd += XOR_STR("buy vest;");
	if (utilities.test(cfg_t::buybot_utilities_helmet))
		cmd += XOR_STR("buy vesthelm;");
	if (utilities.test(cfg_t::buybot_utilities_defuse))
		cmd += XOR_STR("buy defuser;");
	if (utilities.test(cfg_t::buybot_utilities_taser))
		cmd += XOR_STR("buy taser;");
	if (utilities.test(cfg_t::buybot_utilities_smoke))
		cmd += XOR_STR("buy smokegrenade;");
	if (utilities.test(cfg_t::buybot_utilities_he))
		cmd += XOR_STR("buy hegrenade;");
	if (utilities.test(cfg_t::buybot_utilities_molotov))
		cmd += XOR_STR("buy molotov;");
	if (utilities.test(cfg_t::buybot_utilities_flash))
		cmd += XOR_STR("buy flashbang;");
	if (utilities.test(cfg_t::buybot_utilities_flash2))
		cmd += XOR_STR("buy flashbang;");

	if (!cmd.empty())
		game->engine_client->clientcmd_unrestricted(cmd.c_str(), nullptr);
}
} // namespace misc
} // namespace hacks
