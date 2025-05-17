
#ifndef SDK_ENTITY_H
#define SDK_ENTITY_H

#include <sdk/client_entity.h>
#include <sdk/cutlvector.h>
#include <sdk/datamap.h>
#include <sdk/macros.h>
#include <util/fnv1a.h>

namespace sdk
{
inline static constexpr auto ef_nodraw = 1 << 5;

enum effect_flags
{
	efl_dirty_abs_transform = (1 << 11),
	efl_dirty_abs_velocity = (1 << 12),
	efl_dirty_spatial_partition = (1 << 15),
};

enum interp_flags
{
	latch_animation_var = (1 << 0),
	latch_simulation_var = (1 << 1),
	exclude_auto_latch = (1 << 2),
	exclude_auto_interpolate = (1 << 3),
	interpolate_linear_only = (1 << 4),
	interpolate_omit_update_last_networked = (1 << 5)
};

class cstudiohdr;
struct studiohdr;
class quaternion_aligned;
class entity_t;

class var_watcher
{
public:
	VIRTUAL(2, set_interpolation_amount(float sec), void(__thiscall *)(void *, float))(sec);
	VIRTUAL(8, reset_to_last_networked(), void(__thiscall *)(void *))();
	VIRTUAL(10, get_debug_name(), const char *(__thiscall *)(void *))();
	inline uint8_t &get_type_flags() { return *reinterpret_cast<uint8_t *>(uintptr_t(this) + 0x1C); }
};

class var_map_entry
{
public:
	unsigned short type;
	unsigned short needs_interp;
	void *data;
	var_watcher *watcher;
};

struct var_mapping
{
	inline var_watcher *find(const uint32_t hash)
	{
		for (auto i = 0; i < entries.count(); i++)
		{
			auto &entry = entries[i];
			if (entry.watcher && FNV1A_CMP_IM(entry.watcher->get_debug_name(), hash))
				return entry.watcher;
		}
		return nullptr;
	}

	cutlvector<var_map_entry> entries;
	int32_t interpolated_count;
	float last_time;
};

struct bone_accessor
{
	std::array<mat3x4, 128> *out;
	uint32_t readable;
	uint32_t writeable;
};

enum move_type
{
	movetype_none = 0,
	movetype_isometric,
	movetype_walk,
	movetype_step,
	movetype_fly,
	movetype_flygravity,
	movetype_vphysics,
	movetype_push,
	movetype_noclip,
	movetype_ladder,
	movetype_observer,
	movetype_custom,
	movetype_last = movetype_custom,
	movetype_max_bits = 4
};

struct collision_prop
{
private:
	uintptr_t pad;

public:
	vec3 mins, maxs;
};

typedef std::array<float, 24> pose_paramater;

class alignas(16) ik_context
{
	FUNC(game->client.at(functions::ik_context::construct), construct(), void *(__thiscall *)(void *))();
	FUNC(game->client.at(functions::ik_context::destruct), destruct(), void *(__thiscall *)(void *))();

public:
	ik_context() { construct(); }
	~ik_context() { destruct(); }
	FUNC(game->client.at(functions::ik_context::init),
		 init(cstudiohdr *const hdr, angle *const ang, vec3 *const origin, float time, int32_t frame_counter,
			  int32_t bone_mask),
		 void(__thiscall *)(void *, cstudiohdr *, angle *, vec3 *, float, int32_t, int32_t))
	(hdr, ang, origin, time, frame_counter, bone_mask);
	FUNC(game->client.at(functions::ik_context::update_targets),
		 update_targets(vec3 *pos, quaternion_aligned *q, mat3x4 *mat, uint8_t *computed),
		 void(__thiscall *)(void *, vec3 *, quaternion_aligned *, mat3x4 *, uint8_t *))
	(pos, q, mat, computed);
	FUNC(game->client.at(functions::ik_context::solve_dependencies),
		 solve_dependencies(vec3 *pos, quaternion_aligned *q, mat3x4 *mat, uint8_t *computed),
		 void(__thiscall *)(void *, vec3 *, quaternion_aligned *, mat3x4 *, uint8_t *))
	(pos, q, mat, computed);

private:
	uint8_t gap[0x1070];
};

class entity_t : public client_entity
{
	FUNC(game->client.at(offsets::base_entity::fn_is_breakable), is_breakable_internal(), bool(__thiscall *)(void *))();

public:
	FUNC(game->client.at(offsets::base_view_model::fn_remove_viewmodel_arm_models), remove_viewmodel_arm_models(),
		 void(__thiscall *)(void *))
	();
	FUNC(game->client.at(offsets::base_view_model::fn_update_viewmodel_arm_models), update_viewmodel_addons(),
		 void(__thiscall *)(void *))
	();

	VIRTUAL(17, get_pred_desc_map(), datamap_t* (__thiscall*)(void *))();
	VIRTUAL(11, get_abs_angle(), const angle &(__thiscall *)(void *))();
	VIRTUAL(32, spawn(), void(__thiscall *)(void *))();
	VIRTUAL(55, set_dormant(bool enable), void(__thiscall *)(void *, bool))(enable);
	VIRTUAL(75, set_model_index(int32_t index), void(__thiscall *)(void *, int32_t))(index);
	VIRTUAL(79, world_space_center(), const vec3 &(__thiscall *)(void *))();
	VIRTUAL(101, get_old_origin(), vec3 *(__thiscall *)(void *))();
	VIRTUAL(112, reset_latched(), void(__thiscall *)(void *))();
	VIRTUAL(120, set_next_client_think(float time), void(__thiscall *)(void *, float))(time);
	VIRTUAL(156, is_alive(), bool(__thiscall *)(void *))();
	VIRTUAL(159, is_weapon_world_model(), bool(__thiscall *)(void *))();
	VIRTUAL(166, is_base_combat_weapon(), bool(__thiscall *)(void *))();
	VIRTUAL(222, get_sequence_cycle_rate(cstudiohdr *hdr, int32_t seq),
			float(__thiscall *)(void *, cstudiohdr *, int32_t))
	(hdr, seq);

	OFFSET(float, get_spawn_time, 0x103C0); // VAR(offsets::base_entity, spawn_time, float);
	OFFSET(int32_t, get_simulation_tick, 0x2ac); // VAR(offsets::base_entity, simulation_tick, int32_t);
	OFFSET(int32_t, get_team_num, 0xF4); // VAR(offsets::base_entity, team_num, int32_t);
	OFFSET(vec3, get_origin, 0x138); // VAR(offsets::base_entity, origin, vec3);
	OFFSET(float, get_simulation_time, 0x268); // VAR(offsets::base_entity, simulation_time, float);
	OFFSET(base_handle, get_owner_entity, 0x14C); // VAR(offsets::base_entity, owner_entity, base_handle);
	OFFSET(bool, get_spotted, 0x93D); // VAR(offsets::base_entity, spotted, bool);
	OFFSET(int32_t, get_model_index, 0x258); // 0x258 VAR(offsets::base_entity, model_index, int32_t);
	OFFSET(var_mapping, get_var_mapping, 0x24); // VAR(offsets::base_entity, var_mapping, var_mapping);
	OFFSET(bool, get_predictable, 0x2EE); // 0x2EA + 0x4 // VAR(offsets::base_entity, predictable, bool);
	DATAMAPVAR(get_sequence_loops, bool, 0x270D) //big  ///VAR(offsets::base_entity, sequence_loops, bool);
	DATAMAPVAR(get_ground_speed, float, 0x2704)	//VAR(offsets::base_entity, ground_speed, float); m_flFallVelocity
	OFFSET(int32_t, get_effects, 0xF0); // VAR(offsets::base_entity, effects, int32_t);
	DATAMAPVAR(get_eflags, int, 0xe8) // OFFSET(int32_t, get_eflags, 0xE8);  // VAR(offsets::base_entity, eflags, int32_t);
	DATAMAPVAR(get_move_type, int32_t, 0x25c) // VAR(offsets::base_entity, move_type, int32_t);
	DATAMAPVAR(get_rotation_pred, angle, 0xd0) //VAR(offsets::base_entity, rotation_pred, angle);
	DATAMAPVAR(get_abs_rotation, angle, 0xc4) //VAR(offsets::base_entity, abs_rotation, angle);
	DATAMAPVAR(get_water_level, int8_t, 0x25e) // VAR(offsets::base_entity, water_level, int8_t);
	DATAMAPVAR(get_abs_origin, vec3, 0xa0) // VAR(offsets::base_entity, abs_origin, vec3);
	DATAMAPVAR(get_origin_pred, vec3, 0xac) // VAR(offsets::base_entity, origin_pred, vec3);
	OFFSET(vec3, get_velocity, 0x114); // VAR(offsets::base_entity, velocity, vec3);
	OFFSET(int32_t, get_sequence, 0x28c0); // DATAVARMAP EXISTS ON THIS ONE TOO - maj12; //VAR(offsets::base_animating, sequence, int32_t);
	OFFSET(float, get_playback_rate, 0xa18); // VAR(offsets::base_animating, playback_rate, float);
	OFFSET(int32_t, get_new_sequence_parity, 0xa44); // VAR(offsets::base_animating, new_sequence_parity, int32_t);
	NETVARA(get_prev_new_sequence_parity, int, 0xa44, 0x8) 
	OFFSET(bone_accessor, get_bone_accessor, 0x26a8); // VAR(offsets::base_animating, bone_accessor, bone_accessor);
	OFFSET(cstudiohdr *, get_studio_hdr, 0x2950); // VAR(offsets::base_animating, studio_hdr, cstudiohdr *);
	OFFSET(uint32_t, get_most_recent_model_bone_counter, 0x2690);// VAR(offsets::base_animating, most_recent_model_bone_counter, uint32_t);
	ANETVAR(get_pose_parameter, float, 24, 0x2778) // VAR(offsets::base_animating, pose_parameter, pose_paramater);
	OFFSET(ik_context *, get_ik, 0x2670); //VAR(offsets::base_animating, ik, ik_context *);
	OFFSET(int32_t, get_last_non_skipped_frame, 0xA68); // VAR(offsets::base_animating, last_non_skipped_frame, int32_t);
	OFFSET(int32_t, get_hitbox_set, 0x9fc); // VAR(offsets::base_animating, hitbox_set, int32_t);
	OFFSET(float, get_model_scale, 0x274c); // VAR(offsets::base_animating, model_scale, float);
	//OFFSET(int32_t, get_reset_events_parity, 0xa48); // VAR(offsets::base_animating, reset_events_parity, int32_t);
	OFFSET(bool, get_client_side_animation, 0x28a0); // VAR(offsets::base_animating, client_side_animation, bool);
	NETVARA(get_old_simulation_time, float, 0x268, 4) //MEMBER(offsets::base_entity::simulation_time + 4, old_simulation_time, float);
	OFFSET(float, get_anim_time, 0x260); // VAR(offsets::anim_time_must_be_first, anim_time, float);
	OFFSET(int32_t, get_animating_body, 0xa20); // MEMBER(offsets::base_animating::body, animating_body, int32_t);
	NETVARA(get_can_use_fast_path, bool, 0x26fc, 0x4) //MEMBER(offsets::base_animating::frozen + 4, can_use_fast_path, bool);
	NETVARA(get_hdr_handle, uint16_t, 0x2950, 0x4) // MEMBER(offsets::base_animating::studio_hdr + 4, hdr_handle, uint16_t);
	NETVARA(get_prev_reset_events_parity, int32_t, 0xa48, 0x8) //MEMBER(offsets::base_animating::new_sequence_parity + 8, prev_new_sequence_parity, int32_t);
	OFFSET(int32_t, get_reset_events_parity, 0xa48) // MEMBER(offsets::base_animating::reset_events_parity + 8, prev_reset_events_parity, int32_t);

	OFFSET(bool, get_on, 0xb50);  // VAR(offsets::sun, on, bool);
	OFFSET(int32_t, get_size, 0xb3c); // VAR(offsets::sun, size, int32_t);
	OFFSET(bool, get_bomb_defused, 0x29c0); // VAR(offsets::planted_c4, bomb_defused, bool);
	
	//OFFSET(base_handle, get_viewmodel_weapon, 0x29d8); //MEMBER(offsets::base_view_model::weapon, viewmodel_weapon, base_handle);
	//OFFSET(base_handle, get_viewmodel_owner, 0x29dc);  // MEMBER(offsets::base_view_model::owner, viewmodel_owner, base_handle);
	//OFFSET(int32_t, get_viewmodel_sequence, 0x28c0); // MEMBER(offsets::base_view_model::sequence, viewmodel_sequence, int32_t);
	MEMBER(offsets::base_view_model::weapon, viewmodel_weapon, base_handle);
	MEMBER(offsets::base_view_model::owner, viewmodel_owner, base_handle);
	MEMBER(offsets::base_view_model::sequence, viewmodel_sequence, int32_t);
	DATAMAPVAR(get_cycle, float, 0xa14) // VAR(offsets::base_view_model, cycle, float);
	//DATAMAPVAR(get_animation_parity, int32_t, "m_nAnimationParity") // VAR(offsets::base_view_model, animation_parity, int32_t);
	OFFSET(int32_t, get_cycle_offset, 0x29f4); // VAR(offsets::base_view_model, cycle_offset, float);

	FUNC(game->client.at(offsets::base_entity::fn_emit_sound), emit_sound(const char *name),
		 int32_t(__thiscall *)(entity_t *, const char *, entity_t *))
	(name, this);
	FUNC(game->client.at(offsets::base_animating::fn_lookup_sequence), lookup_sequence(const char *name),
		 int32_t(__thiscall *)(entity_t *, const char *))
	(name);
	FUNC(game->client.at(offsets::base_animating::fn_get_sequence_activity), get_sequence_activity(int32_t sequence),
		 int(__thiscall *)(entity_t *, int32_t))
	(sequence);
	FUNC(game->client.at(offsets::base_animating::fn_lookup_bone), lookup_bone(const char *name),
		 int32_t(__thiscall *)(entity_t *, const char *))
	(name);

	inline void set_abs_origin(const vec3 &origin, bool full = false)
	{
		get_abs_origin() = origin;
		if (full && !(get_eflags() & efl_dirty_spatial_partition))
		{
			((void(__stdcall *)(entity_t *))game->client.at(functions::add_to_dirty_kd_tree))(this);
			get_eflags() |= efl_dirty_spatial_partition;
		}
		get_eflags() &= ~efl_dirty_abs_transform;
	}

	inline void set_abs_angle(const angle &ang)
	{
		get_abs_rotation() = ang;
		get_eflags() &= ~efl_dirty_abs_transform;
	}

	inline bool is_breakable()
	{
		const auto result = is_breakable_internal();

		if (!result && (get_class_id() == class_id::breakable_surface ||
						(get_class_id() == class_id::base_entity && get_collideable()->get_solid() == solid_bsp)))
			return true;

		return result;
	}

	inline entity_type get_entity_type()
	{
		switch (get_class_id())
		{
		case class_id::hostage:
			return entity_type::hostage;
		case class_id::chicken:
			return entity_type::chicken;
		case class_id::csplayer:
			return entity_type::player;
		case class_id::c4:
			return entity_type::c4;
		case class_id::planted_c4:
			return entity_type::plantedc4;
		case class_id::inferno:
		case class_id::base_csgrenade_projectile:
		case class_id::molotov_projectile:
		case class_id::smoke_grenade_projectile:
		case class_id::sensor_grenade_projectile:
			return entity_type::projectile;
		case class_id::econ_entity:
			return entity_type::item;
		default:
			break;
		}

		if (is_base_combat_weapon())
			return entity_type::weapon_entity;

		return entity_type::none;
	}

	inline void on_simulation_time_changing(float previous, float current)
	{
		auto f = game->client.at(offsets::base_entity::fn_on_simulation_time_changing);
		__asm
		{
				mov ecx, this
				movss xmm1, previous
				movss xmm2, current
				call f
		}
	}
};
} // namespace sdk

#endif // SDK_ENTITY_H
