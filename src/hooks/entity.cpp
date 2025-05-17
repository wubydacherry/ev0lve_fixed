
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>
#include <detail/entity_rendering.h>

using namespace sdk;
using namespace detail;

namespace hooks::entity
{
void __fastcall on_latch_interpolated_variables(entity_t *ent, uint32_t edx, int32_t flags)
{
	if (ent->get_predictable())
	{
		if (flags & interpolate_omit_update_last_networked && pred.is_computing())
			return;

		const auto backup_time = game->globals->curtime;
		if (ent->is_player())
			game->globals->curtime =
				TICKS_TO_TIME(reinterpret_cast<cs_player_t *>(ent)->get_command_context().cmd_number);
		if (ent->get_class_id() == class_id::predicted_view_model)
		{
			const auto owner = reinterpret_cast<cs_player_t *>(
				game->client_entity_list->get_client_entity_from_handle(ent->get_viewmodel_owner()));
			if (owner && owner->is_player())
				game->globals->curtime = TICKS_TO_TIME(owner->get_command_context().cmd_number);
		}

		hook_manager.on_latch_interpolated_variables->call(ent, edx, flags);
		game->globals->curtime = backup_time;
		return;
	}

	if (!ent->is_player() || !(flags & latch_simulation_var))
		return hook_manager.on_latch_interpolated_variables->call(ent, edx, flags);

	const auto player = reinterpret_cast<cs_player_t *>(ent);
	auto &entry = GET_PLAYER_ENTRY(player);
	constexpr auto clock_correction = .03f;
	const auto interp = GET_CONVAR_FLOAT("cl_interp");
	const auto backup_time = player->get_simulation_time();
	player->get_simulation_time() =
		std::clamp(player->get_simulation_time() + entry.dt_interp, game->globals->curtime - interp,
				   game->globals->curtime + clock_correction + interp);
	hook_manager.on_latch_interpolated_variables->call(ent, edx, flags);
	player->get_simulation_time() = backup_time;
	entry.dt_interp = game->globals->curtime - player->get_simulation_time() + interp;
}

void __fastcall do_animation_events(entity_t *ent, uint32_t edx, cstudiohdr *hdr)
{
	ent->get_prev_reset_events_parity() = ent->get_reset_events_parity();
	hook_manager.do_animation_events->call(ent, edx, hdr);
}

void __fastcall maintain_sequence_transitions(entity_t *ent, uintptr_t edx, uintptr_t bone_setup, vec3 *pos,
											  quaternion *q)
{
	float cycle;
	__asm movss cycle, xmm2;

	const auto get_sequence_flags =
		(uintptr_t(__thiscall *)(void *, int32_t))game->client.at(functions::hdr::get_sequence_flags);
	const auto flags =
		reinterpret_cast<uint32_t *>(get_sequence_flags(ent->get_studio_hdr(), ent->get_sequence()) + 12);

	if (game->local_player && ent->get_class_id() == class_id::predicted_view_model &&
		ent->get_new_sequence_parity() != ent->get_prev_new_sequence_parity() && !(*flags & 1))
	{
		const auto player = reinterpret_cast<cs_player_t *const>(
			game->client_entity_list->get_client_entity_from_handle(ent->get_viewmodel_owner()));

		if (player && player->get_is_holding_look_at_weapon())
		{
			const auto dt = game->globals->curtime - TICKS_TO_TIME(game->globals->tickcount);
			ent->get_cycle_offset() =
				-1.f * ((TICKS_TO_TIME(game->client_state->command_ack) + dt - ent->get_anim_time()) *
						ent->get_sequence_cycle_rate(ent->get_studio_hdr(), ent->get_sequence()));
			cycle = 0.f;
		}
		else
			ent->get_cycle_offset() = 0.f;

		*flags |= 1;
		__asm movss xmm2, cycle;
		hook_manager.maintain_sequence_transitions->call(ent, edx, bone_setup, pos, q);
		*flags &= ~1;
	}
	else
	{
		__asm movss xmm2, cycle;
		hook_manager.maintain_sequence_transitions->call(ent, edx, bone_setup, pos, q);
	}
}

void __fastcall set_abs_angles(entity_t *ent, uint32_t edx, angle *ang)
{
	if (ent_ren.is_updating_animstate)
	{
		*(angle *)(uintptr_t(ent) + 0xD0) = *ang;
		return;
	}

	if (uintptr_t(_ReturnAddress()) != game->client.at(functions::return_to_anim_state_update) ||
		(!cfg.rage.enable.get() && ent->index() != game->engine_client->get_local_player()))
		hook_manager.set_abs_angles->call(ent, edx, ang);
}

void __fastcall estimate_abs_velocity(entity_t *ent, uint32_t edx, vec3 *out)
{
	// grab return address from stack frame.
	uintptr_t *_ebp;
	__asm mov _ebp, ebp;
	auto &ret_addr = _ebp[1];

	// game tried to setup velocity for something irrelevant.
	if (ret_addr != game->client.at(functions::return_to_anim_state_setup_velocity) || !ent->is_player())
		return hook_manager.estimate_abs_velocity->call(ent, edx, out);

	// setup for other players when legit.
	if (!cfg.rage.enable.get() && ent->index() != game->engine_client->get_local_player())
		return hook_manager.estimate_abs_velocity->call(ent, edx, out);

	// write 'real' velocity to output.
	auto velocity = ent->get_velocity();
	*out = velocity;

	// skip velocity clamping shit.
	ret_addr += 0x101;

	// emulate volatile registers between the two addresses.
	__asm
	{
			movss xmm1, velocity.z
			movss xmm3, velocity.y
			movss xmm4, velocity.x
	}
}

bool __fastcall setup_bones(entity_t *ent, uint32_t edx, mat3x4 *out, int max_bones, int mask, float time)
{
	static const auto r_jiggle_bones = game->cvar->find_var(XOR_STR("r_jiggle_bones"));
	const auto p = reinterpret_cast<cs_player_t *>(uintptr_t(ent) - 4);

	if ((*(uintptr_t **)(util::get_ebp()))[1] == game->client.at(functions::return_to_hitbox_to_world_transforms) ||
		game->prediction->is_active())
	{
		if (out)
		{
			if (!p->get_bone_accessor().out || !p->get_studio_hdr())
				return false;
			memcpy(out, p->get_bone_accessor().out, sizeof(mat3x4) * min(p->get_studio_hdr()->numbones(), max_bones));
		}

		return true;
	}

	const auto should_fix = cfg.rage.enable.get() || p->index() == game->engine_client->get_local_player();

	//IDA: "84 C0 75 0D F6 87 28 0A 00 00 0A"
	if (uintptr_t(_ReturnAddress()) != game->client.at(functions::return_to_cs_player_setup_bones))
	{
		const auto back = r_jiggle_bones->get_int();
		if (should_fix &&
			(p->is_player() || (p->get_shadow_parent() && p->get_shadow_parent()->get_client_unknown() &&
								p->get_shadow_parent()->get_client_unknown()->get_client_entity() &&
								p->get_shadow_parent()->get_client_unknown()->get_client_entity()->is_player())))
			r_jiggle_bones->set_int(0);
		const auto ret = hook_manager.setup_bones->call(ent, edx, out, max_bones, mask, time);
		r_jiggle_bones->set_int(back);
		return ret;
	}

	// store off some stuff...
	auto &entry = GET_PLAYER_ENTRY(p);
	const auto state = p->get_anim_state();
	const auto backup_curtime = game->globals->curtime;
	const auto backup_entire_body = p->get_snapshot_entire_body();
	const auto backup_upper_body = p->get_snapshot_upper_body();
	const auto bones = bone_changes(p);
	auto rollback_weapon = base_handle();

	// reevaluate the animlod.
	p->get_lod_data().flags = lod_empty;
	p->get_lod_data().frame = 0;
	hook_manager.reevauluate_anim_lod->call(p, edx, mask);
	const auto flags = p->get_lod_data().flags;

	if (should_fix)
	{
		p->get_last_non_skipped_frame() = 0;
		p->get_snapshot_entire_body().player = p->get_snapshot_upper_body().player = nullptr;
		p->get_lod_data().flags |= lod_invisible;

		// restore animation.
		if (p->get_predictable())
			pred.restore_animation(hacks::tickbase.recent.second, true);
		else
		{
			entry.bones.restore(p);
			lag_record *record = nullptr;
			for (auto &rec : entry.records)
				if (!rec.extrapolated)
				{
					record = &rec;
					break;
				}
			if (record)
				p->eye_angles()->z = record->get_resolver_roll();
			p->get_view_height() = entry.view_delta;
			game->globals->curtime = p->get_simulation_time();
		}

		// disable the feet planting.
		if (state)
		{
			state->first_foot_plant = true;

			if (p->get_predictable())
				p->set_abs_angle({0.f, state->foot_yaw, 0.f});
		}

		// rollback the stick to the beginning.
		if (p->is_alive() && state)
		{
			for (const auto &handle : p->get_my_weapons())
			{
				const auto cur =
					reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(handle));

				if (cur == state->weapon)
				{
					rollback_weapon = p->get_active_weapon();
					p->get_active_weapon() = handle;
					break;
				}
			}
			state->old_weapon = reinterpret_cast<weapon_t *const>(
				game->client_entity_list->get_client_entity_from_handle(p->get_active_weapon()));
		}
	}

	// call original.
	const auto back = r_jiggle_bones->get_int();
	if (should_fix)
		r_jiggle_bones->set_int(0);
	const auto ret = hook_manager.setup_bones->call(ent, edx, out, max_bones, mask, time);
	r_jiggle_bones->set_int(back);

	// restore the rollback weapon and lod flags.
	if (should_fix)
	{
		if (rollback_weapon)
			p->get_active_weapon() = rollback_weapon;
		p->get_lod_data().flags = flags;
		bones.restore(p);
		p->get_snapshot_entire_body() = backup_entire_body;
		p->get_snapshot_upper_body() = backup_upper_body;
		game->globals->curtime = backup_curtime;
	}

	return ret;
}

void __fastcall invalidate_physics_recursive(entity_t *ent, uint32_t edx, int32_t change_flags)
{
	// stub this if we are updating fake animstate.
	if (ent_ren.is_updating_animstate)
		return;

	hook_manager.invalidate_physics_recursive->call(ent, edx, change_flags);
}

int __fastcall lookup_sequence(entity_t *ent, uint32_t edx, const char *sequence)
{
	// this tends to crash in a fake animstate.
	if (!ent_ren.is_updating_animstate)
		return hook_manager.lookup_sequence->call(ent, edx, sequence);

	return ((int(__fastcall *)(void *, const char *))game->client.at(sdk::functions::lookup_sequence))(
		ent->get_studio_hdr(), sequence);
}

void __fastcall notify_should_transmit(entity_t *ent, uint32_t edx, uint32_t state)
{
	const auto e = reinterpret_cast<entity_t *>(uintptr_t(ent) - 8);
	if (state == 1 && e->get_class_id() == class_id::func_brush)
	{
		e->set_dormant(true);
		return;
	}

	return hook_manager.notify_should_transmit->call(ent, edx, state);
}

void __fastcall update_relevant_interpolated_vars(entity_t *ent, uint32_t edx)
{
	const auto back = ent->get_client_side_animation();
	if ((cfg.rage.enable.get() && ent->is_player()) || ent->index() == game->engine_client->get_local_player())
		ent->get_client_side_animation() = false;
	hook_manager.update_relevant_interpolated_vars->call(ent, edx);
	ent->get_client_side_animation() = back;
}

void __fastcall invalidate_mdl_cache(entity_t *ent, uint32_t edx)
{
	if (ent->index() == game->engine_client->get_local_player())
	{
		if (reinterpret_cast<cs_player_t *>(ent)->get_flags() & (1 << 15))
		{
			ent->get_studio_hdr() = nullptr;
			return;
		}

		if (detail::pred.real_hdr)
		{
			if (ent->get_studio_hdr() != detail::pred.real_hdr)
			{
				const auto mdl = ent->get_studio_hdr();
				const auto handle = ent->get_hdr_handle();
				ent->get_studio_hdr() = detail::pred.real_hdr;
				ent->get_hdr_handle() = detail::pred.real_handle;
				hook_manager.invalidate_mdl_cache->call(ent, edx);
				ent->get_hdr_handle() = handle;
				ent->get_studio_hdr() = mdl;
			}

			detail::pred.real_hdr = nullptr;
		}
	}

	hook_manager.invalidate_mdl_cache->call(ent, edx);
}
} // namespace hooks::entity
