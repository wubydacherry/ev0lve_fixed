
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>
#include <detail/player_list.h>
#include <hacks/skinchanger.h>
#include <sdk/client_entity_list.h>

using namespace detail;
using namespace sdk;

namespace hooks::recv_proxies
{
void __cdecl layer_sequence(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_sequence->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
	{
		auto &info = pred.info[game->client_state->command_ack % input_max].animation;
		const auto act = ((cs_player_t *)str->owner)->get_sequence_activity(data->value.int_);

		if (index == 1)
		{
			if (act == XOR_32(962))
				info.addon.swing_left = false;
		}
		else if (index == 3)
		{
			if (info.layers[index].sequence != data->value.int_)
				info.state.adjust_started = act == XOR_32(979) || act == XOR_32(980);
		}
		else if (index == 7)
			info.state.strafe_sequence = data->value.int_;

		info.layers[index].sequence = data->value.int_;
	}
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].sequence = data->value.int_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_sequence->call(data, str, out);
}

void __cdecl layer_cycle(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_cycle->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
	{
		auto &info = pred.info[game->client_state->command_ack % input_max].animation;
		info.layers[index].cycle = data->value.float_;
		if (index == 6)
			info.state.feet_cycle = data->value.float_;
		else if (index == 7)
			info.state.strafe_change_cycle = data->value.float_;
	}
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].cycle = data->value.float_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_cycle->call(data, str, out);
}

void __cdecl layer_playback_rate(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_playback_rate->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
		pred.info[game->client_state->command_ack % input_max].animation.layers[index].playback_rate =
			data->value.float_;
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].playback_rate = data->value.float_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_playback_rate->call(data, str, out);
}

void __cdecl layer_weight(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_weight->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
	{
		auto &info = pred.info[game->client_state->command_ack % input_max].animation;
		info.layers[index].weight = data->value.float_;
		if (index == 6)
			info.state.feet_weight = data->value.float_;
		else if (index == 7)
			info.state.strafe_change_weight = data->value.float_;
		else if (index == 12)
			info.state.acceleration_weight = data->value.float_;
	}
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].weight = data->value.float_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_weight->call(data, str, out);
}

void __cdecl layer_weight_delta_rate(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_weight_delta_rate->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
		pred.info[game->client_state->command_ack % input_max].animation.layers[index].weight_delta_rate =
			data->value.float_;
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].weight_delta_rate = data->value.float_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_weight_delta_rate->call(data, str, out);
}

void __cdecl layer_order(recv_proxy_data *data, animation_layer *str, void *out)
{
	if (!str->owner)
		return hook_manager.layer_order->call(data, str, out);

	const auto index = str - &str->owner->get_animation_layers()->operator[](0);
	if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
	{
		auto &info = pred.info[game->client_state->command_ack % input_max].animation;
		info.layers[index].order = data->value.int_;

		if (index == 1)
		{
			info.state.layer_order_preset = *reinterpret_cast<uintptr_t *>(game->client.at(globals::anim_layer_orders) +
																		   (data->value.int_ == 7 ? 3 : 10));
			if (game->client_state->delta_tick == -1)
				game->last_full_update_frame = game->client_state->command_ack;
		}
	}
	else
		GET_PLAYER_ENTRY(str->owner).server_layers[index].order = data->value.int_;

//	if (!cfg.rage.enable.get() && ((cs_player_t *)str->owner)->index() != game->engine_client->get_local_player())
//		return hook_manager.layer_order->call(data, str, out);
}

void __cdecl general(recv_proxy_data *data, void *ent, void *out)
{
	if (!data->prop || !data->prop->name)
		return hook_manager.general->call(data, ent, out);

	if (FNV1A_CMP(data->prop->name, "m_flThirdpersonRecoil"))
	{
		const auto recoil_scale = GET_CONVAR_FLOAT("weapon_recoil_scale");
		if (fabsf(recoil_scale) > .01f)
			((cs_player_t *)ent)->get_aim_punch_angle().x = data->value.float_ / recoil_scale;
		return hook_manager.general->call(data, ent, out);
	}

	auto &info = pred.info[game->client_state->command_ack % input_max];
	const auto str = reinterpret_cast<animation_layer *>(ent);
	if (FNV1A_CMP(data->prop->name, "m_flPrevCycle") && str->owner)
	{
		const auto index = str - &str->owner->get_animation_layers()->operator[](0);

		if (((cs_player_t *)str->owner)->index() == game->engine_client->get_local_player())
			info.animation.layers[index].prev_cycle = data->value.float_;
		else
			GET_PLAYER_ENTRY(str->owner).server_layers[index].prev_cycle = data->value.float_;
		return;
	}

	const auto is_lower_body_yaw_target = FNV1A_CMP(data->prop->name, "m_flLowerBodyYawTarget");
	const auto is_ang_eye_0 = FNV1A_CMP(data->prop->name, "m_angEyeAngles[0]");
	const auto is_ang_eye_1 = FNV1A_CMP(data->prop->name, "m_angEyeAngles[1]");
	const auto is_vec_view_offset = FNV1A_CMP(data->prop->name, "m_vecViewOffset[2]");
	const auto is_tick_base = FNV1A_CMP(data->prop->name, "m_nTickBase");
	const auto is_addon_junk = FNV1A_CMP(data->prop->name, "m_iAddonBits") ||
							   FNV1A_CMP(data->prop->name, "m_iPrimaryAddon") ||
							   FNV1A_CMP(data->prop->name, "m_iSecondaryAddon");
	const auto is_fov_time = FNV1A_CMP(data->prop->name, "m_flFOVTime");
	const auto is_ground_frac = FNV1A_CMP(data->prop->name, "m_flGroundAccelLinearFracLastTime");

	if (!is_tick_base && !is_lower_body_yaw_target && !is_ang_eye_0 && !is_ang_eye_1 && !is_vec_view_offset &&
		!is_addon_junk && !is_fov_time && !is_ground_frac)
		return hook_manager.general->call(data, ent, out);

	if (((cs_player_t *)ent)->index() != game->engine_client->get_local_player())
	{
		if (!((cs_player_t *)ent)->is_player())
			return hook_manager.general->call(data, ent, out);

		if (is_tick_base && ((cs_player_t *)ent)->is_dormant())
			GET_PLAYER_ENTRY(((cs_player_t *)ent)).pvs = true;

		if (is_ground_frac)
		{
			auto &entry = GET_PLAYER_ENTRY(((cs_player_t *)ent));
			entry.ground_accel_linear_frac_last_time = data->value.float_;
			entry.ground_accel_linear_frac_last_time_stamp = game->client_state->command_ack;
		}

		if (is_ang_eye_0)
			GET_PLAYER_ENTRY(((cs_player_t *)ent)).bones.eye_ang.x = data->value.float_;

		if (is_ang_eye_1)
			GET_PLAYER_ENTRY(((cs_player_t *)ent)).bones.eye_ang.y = data->value.float_;

		if (is_vec_view_offset)
			GET_PLAYER_ENTRY(((cs_player_t *)ent)).bones.view_offset = data->value.float_;

		return hook_manager.general->call(data, ent, out);
	}

	if (is_addon_junk || is_fov_time)
		return;

	if (is_lower_body_yaw_target)
	{
		info.animation.lower_body_yaw_target = data->value.float_;
		return;
	}
	else if (is_vec_view_offset && fabsf(data->value.float_ - info.view_offset) < .375f)
		data->value.float_ = info.view_offset;
	else if (is_tick_base)
	{
		info.animation.state.last_update = TICKS_TO_TIME(data->value.int_ - 1);
		const auto dt = game->client_state->command_ack - game->client_state->last_command_ack;

		if (dt != 0)
		{
			auto sent_commands = 0;
			for (auto i = game->client_state->last_command_ack + 1; i <= game->client_state->command_ack; i++)
			{
				const auto &p = pred.info[i % input_max];
				if (p.sequence != i)
					continue;
				sent_commands += p.tickbase.sent_commands;
			}

			const auto processed_commands = min(sent_commands, dt);
			if (info.tickbase.limit != 0)
				info.tickbase.limit = std::clamp(sv_maxusrcmdprocessticks - processed_commands, 0, info.tickbase.limit);
			else
				info.tickbase.limit = 0;

			auto &next = pred.info[(game->client_state->command_ack + 1) % input_max];
			if (next.sequence == game->client_state->command_ack + 1 && info.tickbase.skip_fake_commands)
			{
				if (!next.tickbase.skip_fake_commands && abs(info.tickbase.base - data->value.int_) <= 2)
					next.tickbase.adjust = info.tickbase.base - data->value.int_;

				if (next.tickbase.adjust == pred.last_adjust && next.tickbase.adjust > 1)
				{
					if (++pred.adjust_counter > 2 && pred.last_adjust_spawn != ((cs_player_t *)ent)->get_spawn_time())
					{
						info.tickbase.limit = max(info.tickbase.limit - next.tickbase.adjust, 0);
						pred.last_adjust_spawn = ((cs_player_t *)ent)->get_spawn_time();
						pred.adjust_counter = 0;
					}
				}
				else
					pred.adjust_counter = 0;

				pred.last_adjust = next.tickbase.adjust;
			}
		}

		if (hacks::tickbase.lag.second == game->client_state->command_ack &&
			abs(hacks::tickbase.lag.first - data->value.int_) <= 4)
			hacks::tickbase.lag.first = data->value.int_;
	}

	hook_manager.general->call(data, ent, out);
}

void __cdecl general_int(recv_proxy_data *data, void *ent, void *out)
{
	if (!data->prop || !data->prop->name)
		return hook_manager.general_int->call(data, ent, out);

	if (FNV1A_CMP(data->prop->name, "m_nModelIndex") &&
		((cs_player_t *)ent)->index() == game->engine_client->get_local_player())
		pred.real_model = data->value.int_;

	hook_manager.general_int->call(data, ent, out);
}

void __cdecl general_vec(recv_proxy_data *data, void *ent, void *out)
{
	if (!data->prop || !data->prop->name)
		return hook_manager.general_vec->call(data, ent, out);

	if (FNV1A_CMP(data->prop->name, "m_aimPunchAngle"))
	{
		vec3 *o = reinterpret_cast<vec3 *>(out);
		o->y = data->value.vector_.y;
		o->z = data->value.vector_.z;
		return;
	}

	hook_manager.general_vec->call(data, ent, out);
}

void __cdecl int_to_ehandle(recv_proxy_data *data, entity_t *ent, void *out)
{
	if (data->value.int_ == 0x1FFFFF)
		*reinterpret_cast<int32_t *>(out) = -1;
	else
		*reinterpret_cast<int32_t *>(out) = data->value.int_ & 0x7FF | (data->value.int_ >> 11 << 16);

	if (data->prop && data->prop->name && FNV1A_CMP(data->prop->name, "m_hGroundEntity"))
		GET_PLAYER_ENTRY(((cs_player_t *)ent)).bones.ground_entity = *reinterpret_cast<base_handle *>(out);
}

void __cdecl weapon_handle(recv_proxy_data *data, entity_t *str, void *out)
{
	if (!game->local_player)
		return hook_manager.weapon_handle->call(data, str, out);

	if (data->value.int_ == 0x1FFFFF)
		*reinterpret_cast<int32_t *>(out) = -1;
	else
		*reinterpret_cast<int32_t *>(out) = data->value.int_ & 0x7FF | (data->value.int_ >> 11 << 16);

	if (pred.info[game->client_state->command_ack % input_max].wpn != *reinterpret_cast<int32_t *>(out))
	{
		str->get_cycle() = str->get_cycle_offset() = 0.f;
		str->get_anim_time() = TICKS_TO_TIME(game->client_state->command_ack);
	}
}

void __cdecl viewmodel_sequence(recv_proxy_data *data, entity_t *str, void *out)
{
	if (!game->local_player)
		return hook_manager.viewmodel_sequence->call(data, str, out);

	const auto owner = game->client_entity_list->get_client_entity_from_handle(str->get_viewmodel_owner());
	const auto viewmodel_weapon = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(str->get_viewmodel_weapon()));

	if (owner && owner->index() == game->engine_client->get_local_player() && viewmodel_weapon &&
		viewmodel_weapon->is_knife())
		data->value.int_ = hacks::skin_changer->fix_sequence(reinterpret_cast<weapon_t *>(str), data->value.int_);

	const auto act = str->get_sequence_activity(data->value.int_);

	if (act == str->get_sequence_activity(*reinterpret_cast<int32_t *>(out)))
		return;

	*reinterpret_cast<int32_t *>(out) = data->value.int_;

	// we do not use the cycle offset. instead, we make the animation start align.
	str->get_cycle_offset() = 0.f;

	if (str->lookup_sequence(XOR_STR("lookat01")) == data->value.int_ || act == XOR_32(213)) // ACT_VM_IDLE_LOWERED
		str->get_anim_time() = TICKS_TO_TIME(game->client_state->command_ack);
}

void __cdecl simulation_time(recv_proxy_data *data, entity_t *ent, void *out)
{
	if (ent && ent->is_player())
	{
		auto &entry = GET_PLAYER_ENTRY(reinterpret_cast<cs_player_t *const>(ent));
		entry.addt = data->value.int_;

		if (!entry.addt)
			return;
	}

	hook_manager.simulation_time->call(data, ent, out);
}

void __cdecl effects(recv_proxy_data *data, weapon_t *wpn, void *out)
{
	const auto old = wpn->get_effects();
	hook_manager.effects->call(data, wpn, out);

	// client-controlled.
	if (wpn->get_class_id() == class_id::base_weapon_world_model)
	{
		const auto parent = reinterpret_cast<weapon_t *>(
			game->client_entity_list->get_client_entity_from_handle(wpn->get_combat_weapon_parent()));
		const auto owner =
			parent ? reinterpret_cast<cs_player_t *>(
						 game->client_entity_list->get_client_entity_from_handle(parent->get_owner_entity()))
				   : nullptr;

		if (owner && owner->is_player() && owner->is_alive() &&
			owner->index() == game->engine_client->get_local_player())
			wpn->get_effects() = (wpn->get_effects() & ~ef_nodraw) | (old & ef_nodraw);
	}
}
} // namespace hooks::recv_proxies
