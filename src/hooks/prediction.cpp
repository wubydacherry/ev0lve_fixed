
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>
#include <detail/player_list.h>
#include <hacks/antiaim.h>
#include <hacks/misc.h>
#include <hacks/tickbase.h>
#include <sdk/client_entity_list.h>

using namespace detail;
using namespace hacks;
using namespace sdk;

namespace hooks::prediction
{
void __fastcall run_command(prediction_t *prediction, uint32_t edx, cs_player_t *player, user_cmd *cmd,
							uintptr_t helper)
{
	if (player->index() != game->engine_client->get_local_player() || cmd->command_number <= 0)
		return hook_manager.run_command->call(prediction, edx, player, cmd, helper);

	pred.last_sequence = cmd->command_number;

	tickbase.compute_current_limit(cmd->command_number);

	// adjust for tickbase shift.
	tickbase.on_run_command(cmd, player->get_tick_base());

	// fixup weapon garbage.
	auto &info = pred.info[cmd->command_number % input_max];
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	auto zoom_level = -1;
	if (wpn)
	{
		if (cmd->predicted)
			wpn->get_last_client_fire_bullet_time() = TICKS_TO_TIME(player->get_tick_base());
		else
		{
			zoom_level = wpn->get_zoom_level();
			wpn->get_last_client_fire_bullet_time() = TICKS_TO_TIME(player->get_tick_base()) - 1.f;
		}

		if (info.sequence == cmd->command_number)
			wpn->get_postpone_fire_ready_time() = info.post_pone_time;
	}

	// restore dumb shit.
	auto &entry = GET_PLAYER_ENTRY(player);
	auto &last = pred.info[(cmd->command_number - 1) % input_max];
	if (last.sequence == cmd->command_number - 1)
	{
		entry.smooth_bounds_change_time = last.smooth_bounds_change_time;
		player->get_view_height() = last.view_height;
		player->get_bounds_change_time() = last.bounds_change_time;
	}

	// run prediction.
	hook_manager.run_command->call(prediction, edx, player, cmd, helper);

	info.tickbase.base = player->get_tick_base();
	info.origin = player->get_origin();
	info.obb_maxs = player->get_collideable()->obb_maxs();
	info.view_offset = player->get_view_offset().z;
	info.wpn = player->get_active_weapon();
	info.ground_entity = player->get_ground_entity();
	info.last_shot = wpn ? wpn->get_last_shot_time() : 0.f;
	info.throw_time = (wpn && wpn->is_grenade()) ? wpn->get_throw_time() : 0.f;
	info.move_type = player->get_move_type();
	info.view_delta = entry.view_delta;
	info.view_height = player->get_view_height();
	info.bounds_change_time = player->get_bounds_change_time();
	if (game->input->verified_commands[cmd->command_number % input_max].crc ==
		*reinterpret_cast<int32_t *>(&game->globals->interval_per_tick))
		info.smooth_bounds_change_time = entry.smooth_bounds_change_time;
	info.velocity = player->get_velocity();

	// play scope sound.
	if (!(cmd->buttons & user_cmd::reload) && zoom_level > -1 &&
		wpn == reinterpret_cast<weapon_t *>(
				   game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon())))
	{
		if (zoom_level < wpn->get_zoom_level())
		{
			const auto in_sound = wpn->get_zoom_in_sound();
			if (in_sound && in_sound[0])
				player->emit_sound(in_sound);
		}
		else if (zoom_level > wpn->get_zoom_level())
		{
			const auto out_sound = wpn->get_zoom_out_sound();
			if (out_sound && out_sound[0])
				player->emit_sound(out_sound);
		}
	}
}

bool __fastcall perform_prediction(prediction_t *prediction, uint32_t edx, int32_t slot, cs_player_t *player,
								   bool recv_update, int32_t ack, int32_t out)
{
	const auto step_sounds = game->rules->data ? &game->rules->data->get_play_all_step_sounds_on_server() : nullptr;
	auto ret = false;
	if (step_sounds)
	{
		const auto bak = *step_sounds;
		*step_sounds = false;
		ret = hook_manager.perform_prediction->call(prediction, edx, slot, player, recv_update, ack, out);
		*step_sounds = bak;
	}
	else
		ret = hook_manager.perform_prediction->call(prediction, edx, slot, player, recv_update, ack, out);
	player->get_final_predicted_tick() = out;
	return ret;
}
} // namespace hooks::prediction
