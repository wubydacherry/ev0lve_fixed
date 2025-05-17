
#ifndef SDK_CS_PLAYER_H
#define SDK_CS_PLAYER_H

#include <algorithm>
#include <base/game.h>
#include <sdk/client_entity_list.h>
#include <sdk/convar.h>
#include <sdk/cs_player_resource.h>
#include <sdk/engine_client.h>
#include <sdk/game_movement.h>
#include <sdk/math.h>
#include <sdk/surface_props.h>
#include <sdk/weapon.h>

namespace sdk
{
inline static constexpr auto max_cs_player_move_speed = 260.f;
inline static constexpr auto efl_setting_up_bones = 1 << 3;
inline static constexpr auto sv_max_usercmd_future_ticks = 8;

class cs_player_t;
class quaternion;

class animation_layer
{
	char pad_0000[20] = {};

public:
	uint32_t order{};
	uint32_t sequence{};
	float prev_cycle{};
	float weight{};
	float weight_delta_rate{};
	float playback_rate{};
	float cycle{};
	cs_player_t *owner{};

private:
	char pad_0038[4] = {};

public:
	animation_layer &operator=(const animation_layer &rhs)
	{
		if (this == &rhs)
			return *this;

		this->sequence = rhs.sequence;
		this->cycle = rhs.cycle;
		this->playback_rate = rhs.playback_rate;
		this->prev_cycle = rhs.prev_cycle;
		this->weight = rhs.weight;
		this->weight_delta_rate = rhs.weight_delta_rate;
		this->order = rhs.order;

		return *this;
	}

	bool operator!=(const animation_layer &other) const
	{
		return sequence != other.sequence || weight != other.weight || weight_delta_rate != other.weight_delta_rate ||
			   playback_rate != other.playback_rate || cycle != other.cycle || order != other.order;
	}

	inline void increment_layer_cycle(const float delta, const bool loop)
	{
		if (fabsf(playback_rate) <= 0.f)
			return;

		auto cur_cycle = cycle;
		cur_cycle += delta * playback_rate;

		if (!loop && cur_cycle >= 1.f)
			cur_cycle = .999f;

		cur_cycle -= int(cur_cycle);

		if (cur_cycle < 0.f)
			cur_cycle += 1.f;
		else if (cur_cycle > 1.f)
			cur_cycle -= 1.f;

		cycle = cur_cycle;
	}

	inline void increment_layer_weight(const float delta)
	{
		if (fabsf(playback_rate) <= 0.f)
			return;

		weight = std::clamp(weight + delta * weight_delta_rate, 0.f, 1.f);
	}

	inline void set_layer_weight_rate(const float delta, const float previous)
	{
		if (delta == 0.f)
			return;

		weight_delta_rate = (weight - previous) / delta;
	}

	inline void set_layer_ideal_weight_from_sequence_cycle(cstudiohdr *hdr)
	{
		const auto seqdesc = hdr->get_seq_desc(sequence);

		if (!seqdesc)
		{
			weight = 0.f;
			return;
		}

		auto ideal = 1.f;
		auto cur_cycle = cycle;
		if (cur_cycle >= .999f)
			cur_cycle = 1.f;

		if (seqdesc->fadeintime > 0.f && cur_cycle < seqdesc->fadeintime)
			ideal = smoothstep_bounds(0.f, seqdesc->fadeintime, cur_cycle);
		else if (seqdesc->fadeouttime < 1.f && cur_cycle > seqdesc->fadeouttime)
			ideal = smoothstep_bounds(1.f, seqdesc->fadeouttime, cur_cycle);

		if (ideal < .0015f)
			ideal = 0.f;

		weight = std::clamp(ideal, 0.f, 1.f);
	}

	inline void increment_layer_cycle_weight_rate_generic(const float delta, cstudiohdr *hdr)
	{
		const auto old_weight = weight;
		increment_layer_cycle(delta, false);
		set_layer_ideal_weight_from_sequence_cycle(hdr);
		set_layer_weight_rate(delta, old_weight);
	}

	inline bool has_animation_completed(const float delta = 0.f) const
	{
		return cycle + playback_rate * delta >= .999f;
	}

	inline bool has_sequence_completed(const float delta = 0.f) const { return cycle + playback_rate * delta >= 1.f; }

	inline void reset()
	{
		sequence = order = 0;
		prev_cycle = weight = weight_delta_rate = playback_rate = cycle = 0.f;
	}
};

struct alignas(16) bone_snapshot
{
	entity_t *player;
	float weight;
	mat3x4 cache[256];
	const char *weight_list_name;
	float weight_list[256];
	bool weight_list_init;
	bool enabled;
	bool capture_pending;
	float ideal_duration;
	float start_time;
	float end_time;
	vec3 world_pos_capture_pos;
	float setup_time;
	int32_t hdr_id;
	uintptr_t memory;
	int32_t allocation_count;
	int32_t grow_count;
	int32_t size;
	uintptr_t elements;
};

typedef std::array<animation_layer, 13> animation_layers;

struct aimmatrix_transition
{
	float has_been_valid;
	float has_been_invalid;
	float blend_in;
	float blend_out;
	float value;
};

struct pose_param_cache
{
	bool init;
	int32_t index;
	const char *name;
};

struct procedural_foot
{
	sdk::vec3 anim;
	sdk::vec3 anim_last;
	sdk::vec3 plant;
	sdk::vec3 plant_vel;
	float lock_amount;
	float plant_time;
};

struct anim_state
{
	FUNC(game->client.at(functions::anim_state::create), create(cs_player_t *player),
		 void(__thiscall *)(void *, cs_player_t *))
	(player);
	FUNC(game->client.at(functions::anim_state::reset), reset(), void(__thiscall *)(void *))();
	FUNC(game->client.at(functions::anim_state::get_active_weapon_prefix), get_active_weapon_prefix() const,
		 const char *(__thiscall *)(const void *))
	();
	FUNC(game->client.at(functions::anim_state::update_layer_order_preset),
		 update_layer_order_preset(int32_t index, int32_t sequence), void(__thiscall *)(void *, int32_t, int32_t))
	(index, sequence);

	inline void update(float yaw, float pitch)
	{
		auto &enable_invalidate_bone_cache =
			*(bool *)game->client.at(offsets::base_animating::enable_invalidate_bone_cache);
		const auto bak = enable_invalidate_bone_cache;
		((void(__vectorcall *)(void *, uint32_t, float, float, float, bool))game->client.at(
			functions::anim_state::update))(this, 0, 0.f, yaw, pitch, false);
		enable_invalidate_bone_cache = bak;
	}

	inline void copy_meta(anim_state *const to) const
	{
		to->first_run = this->first_run;
		to->first_foot_plant = this->first_foot_plant;
		to->last_frame = this->last_frame;
		to->smooth_eye_position = this->smooth_eye_position;
		to->strafe_change_weight_smooth_falloff = this->strafe_change_weight_smooth_falloff;
		to->cached_model = this->cached_model;
		to->step_height_left = this->step_height_left;
		to->step_height_right = this->step_height_right;
		to->old_weapon = this->old_weapon;
		to->player = this->player;
		memcpy(to->poses, this->poses, sizeof(poses));
		to->foot_left = this->foot_left;
		to->foot_right = this->foot_right;
		to->camera_smooth_height = this->camera_smooth_height;
		to->smooth_height_valid = this->smooth_height_valid;
		to->last_time_velocity_over_ten = this->last_time_velocity_over_ten;
		to->head_interpolation_height = this->head_interpolation_height;
		to->animset_version = this->animset_version;
	}

	uintptr_t layer_order_preset;
	bool first_run;
	bool first_foot_plant;
	int32_t last_frame;
	float smooth_eye_position;
	float strafe_change_weight_smooth_falloff;
	aimmatrix_transition stand_walk_aim;
	aimmatrix_transition stand_run_aim;
	aimmatrix_transition crouch_walk_aim;
	int32_t cached_model;
	float step_height_left;
	float step_height_right;
	sdk::weapon_t *old_weapon;
	sdk::cs_player_t *player;
	sdk::weapon_t *weapon;
	sdk::weapon_t *last_weapon;
	float last_update;
	int32_t last_framecount;
	float last_increment;
	float eye_yaw;
	float eye_pitch;
	float foot_yaw;
	float foot_yaw_last;
	float move_yaw;
	float move_yaw_ideal;
	float move_yaw_current_to_ideal;
	float time_to_align_lower_body;
	float feet_cycle;
	float feet_weight;
	float feet_weight_smoothed;
	float duck_amount;
	float duck_additional;
	float recrouch_weight;
	sdk::vec3 position_current;
	sdk::vec3 position_last;
	sdk::vec3 velocity;
	sdk::vec3 movement_direction;
	sdk::vec3 last_accel_speed;
	float speed;
	float speed_z;
	float walk_speed;
	float running_speed;
	float ducking_speed;
	float moving_time;
	float standing_time;
	bool on_ground;
	bool in_hit_ground;
	float jump_to_fall;
	float air_time;
	float left_ground_height;
	float land_anim_mult;
	float ground_fraction;
	bool landed_this_frame;
	bool left_this_frame;
	float in_air_smooth;
	bool on_ladder;
	float ladder_weight;
	float ladder_speed;
	bool ground_fraction_state;
	bool defuse_started;
	bool plant_started;
	bool twitch_started;
	bool adjust_started;
	cutlvector<uint16_t> modifiers{};
	float next_twicth;
	float last_injury;
	float last_velocity_test;
	sdk::vec3 velocity_last;
	sdk::vec3 target_accel;
	sdk::vec3 accel;
	float acceleration_weight;
	float aim_matrix_transition;
	float aim_matrix_transition_delay;
	bool flashed;
	float strafe_change_weight;
	float strafe_change_target_weight;
	float strafe_change_cycle;
	int32_t strafe_sequence;
	bool strafe_changing;
	float strafe_duration;
	float foop_lerp;
	bool feet_crossed;
	bool is_accelerating;
	pose_param_cache poses[20];
	float move_weight_too_high;
	float static_approach_speed;
	int32_t previous_move_state;
	float stutter_step;
	float action_weight_bias_remainder;
	procedural_foot foot_left;
	procedural_foot foot_right;
	float camera_smooth_height;
	bool smooth_height_valid;
	float last_time_velocity_over_ten;
	float head_interpolation_height;
	float aim_yaw_min;
	float aim_yaw_max;
	float aim_pitch_min;
	float aim_pitch_max;
	int32_t animset_version;
};

enum lod_flags
{
	lod_empty = 0,
	lod_distant = 1,
	lod_outside_frustum = 2,
	lod_invisible = 4,
	lod_dormant = 8
};

struct lod_data
{
	uint32_t bone_mask;
	uint32_t flags, old_flags;
	uint32_t frame;
	float distance;
};

struct command_context
{
	bool needs_processing;
	user_cmd cmd;
	int32_t cmd_number;
};

struct ammo
{
	char *name;
	int32_t dmg_type;
	int32_t tracer_type;
	int32_t min_splash;
	int32_t max_splash;
	int32_t flags;
	int32_t plr_dmg;
	int32_t npc_dmg;
	int32_t max_carry;
	int32_t force_impulse;
	convar *cplr_dmg;
	convar *cnpc_dmg;
	convar *cmax_carry;
	convar *cforce_impulse;
};

class ammo_def
{
	int32_t pad;
	int32_t ind;
	ammo type[32];

public:
	inline static ammo_def *get()
	{
		return ((ammo_def * (__stdcall *)()) game->client.at(offsets::csplayer::fn_get_ammo_def))();
	}

	inline int index(uint32_t search)
	{
		for (auto i = 1; i < ind; i++)
			if (FNV1A_CMP_IM(type[i].name, search))
				return i;

		return -1;
	}

	inline bool can_carry_infinite_ammo(uint32_t index)
	{
		if (index < 1 || index >= ind)
			return false;

		auto max_carry = type[index].max_carry;
		if (max_carry == -1 && type[index].cmax_carry)
			max_carry = type[index].cmax_carry->get_int();

		return max_carry == -2;
	}
};

typedef std::array<base_handle, 64> weapon_handles;
typedef std::array<base_handle, 1> wearable_handles;
struct local_data_t
{
	FUNC(game->client.at(offsets::base_player::fn_init_local_data), init_local_data(),
		 void(__thiscall *)(local_data_t *))
	()

		uint8_t pad[0x210];
};

class cs_player_t : public entity_t
{
	FUNC(game->client.at(offsets::csplayer::fn_is_other_enemy), is_other_enemy(cs_player_t *other),
		 bool(__thiscall *)(cs_player_t *, cs_player_t *))
	(other)

		public : static constexpr auto ef_nointerp = 8;

	enum pose_param
	{
		strafe_yaw,
		stand,
		lean_yaw,
		speed,
		ladder_yaw,
		ladder_speed,
		jump_fall,
		move_yaw,
		move_blend_crouch,
		move_blend_walk,
		move_blend_run,
		body_yaw,
		body_pitch,
		aim_blend_stand_idle,
		aim_blend_stand_walk,
		aim_blend_stand_run,
		aim_blend_crouch_idle,
		aim_blend_crouch_walk,
		death_yaw
	};

	enum flags
	{
		on_ground = 1 << 0,
		ducking = 1 << 1,
		water_jump = 1 << 3,
		on_train = 1 << 4,
		in_rain = 1 << 5,
		frozen = 1 << 6,
		at_controls = 1 << 7,
		client = 1 << 8,
		fake_client = 1 << 9,
		in_water = 1 << 10,
		fly = 1 << 11,
		swim = 1 << 12,
		conveyor = 1 << 13,
		npc = 1 << 14,
		god_mode = 1 << 15,
		no_target = 1 << 16,
		aim_target = 1 << 17,
		partial_ground = 1 << 18,
		static_prop = 1 << 19,
		graphed = 1 << 20,
		grenade = 1 << 21,
		step_movement = 1 << 22,
		dont_touch = 1 << 23,
		base_velocity = 1 << 24,
		world_brush = 1 << 25,
		object = 1 << 26,
		kill_me = 1 << 27,
		on_fire = 1 << 28,
		dissolving = 1 << 29,
		tansragdoll = 1 << 30,
		unblockable_by_player = 1 << 31
	};

	enum class hitbox
	{
		head,
		neck,
		pelvis,
		body,
		thorax,
		chest,
		upper_chest,
		left_thigh,
		right_thigh,
		left_calf,
		right_calf,
		left_foot,
		right_foot,
		left_hand,
		right_hand,
		left_upper_arm,
		left_forearm,
		right_upper_arm,
		right_forearm,
		max
	};

	inline static const hitbox hitboxes[] = {
		hitbox::head,		   hitbox::neck,		   hitbox::pelvis,		hitbox::body,
		hitbox::thorax,		   hitbox::chest,		   hitbox::upper_chest, hitbox::right_thigh,
		hitbox::left_thigh,	   hitbox::right_calf,	   hitbox::left_calf,	hitbox::right_foot,
		hitbox::left_foot,	   hitbox::right_hand,	   hitbox::left_hand,	hitbox::right_upper_arm,
		hitbox::right_forearm, hitbox::left_upper_arm, hitbox::left_forearm};

	inline static const hitbox local_scan_hitboxes[] = {
		hitbox::head,	   hitbox::left_upper_arm, hitbox::right_upper_arm, hitbox::right_calf,
		hitbox::left_calf, hitbox::pelvis,		   hitbox::upper_chest};

	VAR(offsets::base_combat_character, active_weapon, base_handle);
	VAR(offsets::base_combat_character, my_weapons, weapon_handles);
	VAR(offsets::base_combat_character, my_wearables, wearable_handles);
	DATAMAPVAR(get_next_attack, float, 0x2D80) //VAR(offsets::base_combat_character, next_attack, float);

	VAR(offsets::base_player, ammo, int32_t);
	VAR(offsets::base_player, flags, uint32_t);
	VAR(offsets::base_player, tick_base, int32_t);
	VAR(offsets::base_player, observer_target, base_handle);
	VAR(offsets::base_player, observer_mode, int32_t);
	VAR(offsets::base_player, view_model_0, base_handle);
	VAR(offsets::base_player, fovtime, float);
	VAR(offsets::base_player, fov, int32_t);
	VAR(offsets::base_player, fovstart, int32_t);
	VAR(offsets::base_player, maxspeed, float);
	DATAMAPVAR(get_duck_amount, float, 0x2fbc) //VAR(offsets::base_player, duck_amount, float);
	DATAMAPVAR(get_duck_speed, float, 0x2fc0) //VAR(offsets::base_player, duck_speed, float);
	DATAMAPVAR(get_surface_friction, float, 0x323)// VAR(offsets::base_player, surface_friction, float);
	DATAMAPVAR(get_ground_entity, base_handle, 0x150 ) //VAR(offsets::base_player, ground_entity, base_handle);
	VAR(offsets::base_player, final_predicted_tick, int32_t);
	VAR(offsets::base_player, command_context, command_context);
	VAR(offsets::base_player, surface_props, surfacedata_t *);
	VAR(offsets::base_player, use_new_animstate, bool);
	DATAMAPVAR( get_has_walk_moved_since_last_jump, bool, 0x3228) //VAR(offsets::base_player, has_walk_moved_since_last_jump, bool);
	VAR(offsets::base_player, last_duck_time, float);

	VAR(offsets::csplayer, is_scoped, bool);
	VAR(offsets::csplayer, is_walking, bool);
	VAR(offsets::csplayer, has_helmet, bool);
	VAR(offsets::csplayer, has_heavy_armor, bool);
	VAR(offsets::csplayer, gun_game_immunity, bool);
	VAR(offsets::csplayer, flash_max_alpha, float);
	VAR(offsets::csplayer, flash_bang_time, float);
	VAR(offsets::csplayer, eye_angles, angle);
	VAR(offsets::csplayer, lower_body_yaw_target, float);
	VAR(offsets::csplayer, carried_hostage, base_handle);
	VAR(offsets::csplayer, strafing, bool);
	VAR(offsets::csplayer, move_state, int32_t);
	VAR(offsets::csplayer, shots_fired, int32_t);
	VAR(offsets::csplayer, wait_for_no_attack, bool);
	VAR(offsets::csplayer, thirdperson_recoil, float);
	VAR(offsets::csplayer, addon_bits, int32_t);
	VAR(offsets::csplayer, primary_addon, int32_t);
	VAR(offsets::csplayer, secondary_addon, int32_t);
	VAR(offsets::csplayer, is_looking_at_weapon, bool);
	VAR(offsets::csplayer, is_holding_look_at_weapon, bool);
	VAR(offsets::csplayer, stamina, float);
	VAR(offsets::csplayer, ground_accel_linear_frac_last_time, float);
	VAR(offsets::csplayer, lod_data, lod_data);
	VAR(offsets::csplayer, animation_layers, animation_layers *);
	VAR(offsets::csplayer, anim_state, anim_state *);
	VAR(offsets::csplayer, snapshot_upper_body, bone_snapshot);
	VAR(offsets::csplayer, snapshot_entire_body, bone_snapshot);
	VAR(offsets::csplayer, view_height, float);
	VAR(offsets::csplayer, bounds_change_time, float);
	DATAMAPVAR(get_cycle, float, 0xa14)//VAR(offsets::csplayer, cycle, float);
	VAR(offsets::csplayer, assassination_target_addon, base_handle);
	VAR(offsets::csplayer, is_player_ghost, bool);
	VAR(offsets::csplayer, ragdoll, base_handle);

	MEMBER(0x2a00, player, base_handle); // DT_CSRagdoll
	MEMBER(0x103ec, velocity_modifier, float);
	VAR(offsets::local, view_punch_angle, angle);
	VAR(offsets::local, aim_punch_angle, angle);
	VAR(offsets::local, aim_punch_angle_vel, angle);
	VAR(offsets::local, ducking, bool);
	VAR(offsets::local, ducked, bool);
	VAR(offsets::local, step_size, float);

	MEMBER(offsets::base_player::duck_speed + 0x4, last_position_at_full_crouch_speed, vec2);
	MEMBER(0x100, player_health, int32_t);
	MEMBER(0x117cc, player_armor, int32_t);
	MEMBER(0x117dc, player_defuser, bool);
	MEMBER(offsets::csplayer::flash_duration - 0xC, flash_alpha, float);
	MEMBER(0x108, view_offset, vec3);
	MEMBER(0x144, friction, float);
	MEMBER(0x2fcc, local, local_data_t);

	VIRTUAL(170, eye_angles(), sdk::angle *(__thiscall *)(void *))();
	VIRTUAL(206,
			standard_blending_rules(sdk::cstudiohdr *hdr, sdk::vec3 *pos, sdk::quaternion_aligned *q, float time,
									int32_t mask),
			void(__thiscall *)(void *, sdk::cstudiohdr *, sdk::vec3 *, sdk::quaternion_aligned *, float, int32_t))
	(hdr, pos, q, time, mask);
	VIRTUAL(223, get_layer_sequence_cycle_rate(animation_layer *layer, int32_t sequence),
			float(__thiscall *)(void *, animation_layer *, int32_t))
	(layer, sequence);
	VIRTUAL(263, owns_this_type(const char *type), sdk::weapon_t *(__thiscall *)(void *, const char *, int32_t))
	(type, 0);
	VIRTUAL(264, get_slot(int32_t slot), sdk::weapon_t *(__thiscall *)(void *, int32_t))(slot);

	FUNC(game->client.at(offsets::csplayer::fn_post_build_transformations),
		 post_build_transformations(sdk::mat3x4 *out, uint32_t mask),
		 void(__thiscall *)(cs_player_t *, sdk::mat3x4 *, uint32_t))
	(out, mask);
	FUNC(game->client.at(offsets::csplayer::fn_update_addon_models), update_addon_models(bool force),
		 int32_t(__thiscall *)(cs_player_t *, bool))
	(force);

	inline int32_t get_health()
	{
		if (game->cs_player_resource->data)
			return game->cs_player_resource->data->get_health()[index()];
		return get_player_health();
	}

	inline int32_t get_armor()
	{
		if (game->cs_player_resource->data)
			return game->cs_player_resource->data->get_armor()[index()];
		return get_player_armor();
	}

	inline bool holds_bomb()
	{
		if (game->cs_player_resource->data)
			return game->cs_player_resource->data->get_player_c4() == index();
		return false;
	}

	inline bool has_helmet()
	{
		if (game->cs_player_resource->data)
			return game->cs_player_resource->data->get_has_helmet()[index()];
		return get_has_helmet();
	}

	inline bool has_defuser()
	{
		if (game->cs_player_resource->data)
			return game->cs_player_resource->data->get_has_defuser()[index()];
		return get_player_defuser();
	}

	inline bool is_enemy(cs_player_t *other = nullptr)
	{
		if (!other)
		{
			other = reinterpret_cast<cs_player_t *>(
				game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

			if (other && !other->is_alive())
				other = reinterpret_cast<cs_player_t *>(
					game->client_entity_list->get_client_entity_from_handle(other->get_observer_target()));
		}

		return other ? other->is_other_enemy(this) : false;
	}

	inline bool is_valid(bool not_dormant = false)
	{
		const auto ragdoll =
			reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity_from_handle(get_ragdoll()));
		return is_alive() && !ragdoll && (game->local_player == this || !is_dormant() || not_dormant) && index() >= 1 &&
			   index() <= 65;
	}

	inline bool is_on_ground()
	{
		return get_flags() & on_ground || get_flags() & partial_ground || get_flags() & conveyor;
	}

	inline bool is_fake_player() { return game->engine_client->get_player_info(index()).fakeplayer; }

	inline vec3 get_eye_position()
	{
		auto res = get_abs_origin();
		if (index() == game->engine_client->get_local_player())
		{
			res += get_view_offset();
			modify_eye_position(res);
		}
		else
		{
			res += game->cs_game_movement->get_view_offset(false);
			if (is_on_ground())
				res -=
					(game->cs_game_movement->get_view_offset(false) - game->cs_game_movement->get_view_offset(true)) *
					get_duck_amount();
		}

		return res;
	}

	void modify_eye_position(vec3 &pos);

	inline angle get_punch_angle_scaled() { return get_aim_punch_angle() * GET_CONVAR_FLOAT("weapon_recoil_scale"); }

	inline int32_t get_ammo_count(uint32_t hash)
	{
		const auto def = ammo_def::get();
		const auto ind = def->index(hash);

		if (ind == -1)
			return 0;

		if (def->can_carry_infinite_ammo(ind))
			return XOR_32(999);

		const auto ammos = reinterpret_cast<int32_t *>(&get_ammo());
		return ammos[ind];
	}

	inline void try_initiate_animation(size_t layer, int32_t activity, std::array<uint16_t, 3> &modifiers,
									   uint32_t seed)
	{
		const auto empty_mapping = game->client.at(functions::hdr::empty_mapping);
		const auto hdr = get_studio_hdr();
		const auto state = get_anim_state();

		if (!hdr || !hdr->sequences_available() || hdr->get_numseq() == 1 || !state)
			return;

		if (hdr->get_activity_list_version() < 1)
			hdr->index_model_sequences();

		auto &mapping = hdr->get_activity_mapping();

		if (!mapping)
			mapping = hdr->find_mapping();

		if (uintptr_t(mapping) != empty_mapping &&
			(mapping->studio_hdr != hdr->hdr || mapping->v_model != hdr->v_model))
			mapping->reinitialize(hdr);

		if (!mapping->sequence_tuples)
			return;

		hash_value_type dummy{activity, 0, 0, 0};
		const uint32_t handle = mapping->act_to_sequence_hash.find_hash(&dummy);
		const int32_t data_index = handle & 0xFFFF;
		const int32_t bucket_index = (handle >> 16) & 0xFFFF;

		if (bucket_index >= mapping->act_to_sequence_hash.buckets.count() ||
			data_index >= mapping->act_to_sequence_hash.buckets[bucket_index].count())
			return;

		const auto data = &mapping->act_to_sequence_hash.buckets[bucket_index][data_index];

		int32_t top_score = -1;
		std::vector<int32_t> top_scores;
		top_scores.reserve(data->count);

		for (int i = 0; i < data->count; i++)
		{
			const auto info = &mapping->sequence_tuples[data->starting_index + i];

			auto score = 0;

			for (auto k = 0; k < info->count_activity_modifiers; k++)
			{
				auto matching_mods = false;

				for (auto m = 0; m < modifiers.size(); m++)
					if (modifiers[m] != 0xFFFF && info->activity_modifiers[k] == modifiers[m])
					{
						matching_mods = true;
						break;
					}

				auto done = false;

				switch (info->calculation_modes[k])
				{
				case 0:
					if (matching_mods)
						score += 2;
					else
						score--;
					break;
				case 1:
					if (matching_mods)
						score += 2;
					break;
				case 2:
					score += (matching_mods ? 2 : -2);
					break;
				case 3:
					if (matching_mods)
						done = true;
					break;
				case 4:
					if (!matching_mods)
						done = true;
					break;
				}

				if (done)
					break;
			}

			if (score > top_score)
				top_scores.clear();

			if (score >= top_score)
			{
				for (auto i = 0; i < info->weight; i++)
					top_scores.push_back(info->seqnum);

				top_score = score;
			}
		}

		if (top_scores.empty())
			return;

		random_seed(seed);
		const auto winner = top_scores[random_int(0, top_scores.size() - 1)];

		if (winner < 2)
			return;

		auto &l = get_animation_layers()->at(layer);
		l.sequence = winner;
		l.playback_rate = get_layer_sequence_cycle_rate(&l, winner);
		l.cycle = l.weight = 0.f;
		state->update_layer_order_preset(layer, winner);
	}
};

struct move_delta_t
{
	void store(cs_player_t *player)
	{
		view_offset = player->get_view_offset().z;
		duck_amount = player->get_duck_amount();
		duck_speed = player->get_duck_speed();
		stamina = player->get_stamina();
		velocity_modifier = player->get_velocity_modifier();
		ground_entity = player->get_ground_entity();
		ground_accel_linear_frac_last_time = player->get_ground_accel_linear_frac_last_time();
		move_type = player->get_move_type();
		move_state = player->get_move_state();
		flags = player->get_flags();
		local = player->get_local();
		last_duck_time = player->get_last_duck_time();
		has_walk_moved_since_last_jump = player->get_has_walk_moved_since_last_jump();
		last_position_at_full_crouch_speed = player->get_last_position_at_full_crouch_speed();
	}

	void restore(cs_player_t *player) const
	{
		player->get_view_offset().z = view_offset;
		player->get_duck_amount() = duck_amount;
		player->get_duck_speed() = duck_speed;
		player->get_stamina() = stamina;
		player->get_velocity_modifier() = velocity_modifier;
		player->get_ground_entity() = ground_entity;
		player->get_ground_accel_linear_frac_last_time() = ground_accel_linear_frac_last_time;
		player->get_move_type() = move_type;
		player->get_move_state() = move_state;
		player->get_flags() = flags;
		player->get_local() = local;
		player->get_last_duck_time() = last_duck_time;
		player->get_has_walk_moved_since_last_jump() = has_walk_moved_since_last_jump;
		player->get_last_position_at_full_crouch_speed() = last_position_at_full_crouch_speed;
	}

	float view_offset;
	float duck_amount;
	float duck_speed;
	float stamina;
	float velocity_modifier;
	base_handle ground_entity;
	float ground_accel_linear_frac_last_time;
	int32_t move_type;
	int32_t move_state;
	int32_t flags;
	local_data_t local;
	float last_duck_time;
	bool has_walk_moved_since_last_jump;
	vec2 last_position_at_full_crouch_speed;
};
} // namespace sdk

#endif // SDK_CS_PLAYER_H
