
#ifndef DETAIL_PLAYER_LIST_H
#define DETAIL_PLAYER_LIST_H

#include <base/cfg.h>
#include <hacks/misc.h>
#include <hacks/tickbase.h>
#include <sdk/client_state.h>
#include <sdk/convar.h>
#include <sdk/cs_player.h>
#include <sdk/cutlvector.h>
#include <sdk/prediction.h>
#include <util/circular_buffer.h>

namespace detail
{
namespace aim_helper
{
std::optional<sdk::vec3> get_hitbox_position(sdk::cs_player_t *player, sdk::cs_player_t::hitbox id,
											 const sdk::mat3x4 *bones);
}

inline static constexpr auto flip_margin = 30.f;
inline static constexpr auto teleport_dist = 64 * 64;

enum resolver_state
{
	resolver_default = 0,
	resolver_flip,
	resolver_shot,
	resolver_state_max
};

enum resolver_direction
{
	resolver_networked = 0,
	resolver_max_extra,
	resolver_max_max,
	resolver_max,
	resolver_min,
	resolver_min_min,
	resolver_min_extra,
	resolver_center,
	resolver_direction_max
};

enum hack_kind
{
	hack_kind_none,
	hack_kind_ev0lve,
	hack_kind_fatality,
};

__forceinline float calculate_lerp()
{
	return fmaxf(GET_CONVAR_FLOAT("cl_interp"), GET_CONVAR_FLOAT("cl_interp_ratio") / GET_CONVAR_INT("cl_updaterate"));
}

__forceinline float calculate_rtt()
{
	return game->client_state->net_channel->get_latency(sdk::flow_incoming) +
		   game->client_state->net_channel->get_latency(sdk::flow_outgoing);
}

struct addon_t
{
	bool in_jump{}, in_rate_limit{}, swing_left = true;
	std::array<uint16_t, 3> activity_modifiers{};
	float next_lby_update{};
	float adjust_weight{};
	int32_t vm{};
};

struct lag_record
{
	lag_record() = default;
	explicit lag_record(sdk::cs_player_t *const player);
	~lag_record() = default;

	lag_record &operator=(const lag_record &other)
	{
		valid = other.valid;
		resolved = other.resolved;
		shot = other.shot;
		has_state = false;
		has_visual_matrix = false;
		has_previous_matrix = false;
		has_matrix = false;
		breaking_lc = false;
		force_safepoint = other.force_safepoint;
		do_not_set = other.do_not_set;
		index = other.index;
		addon = other.addon;
		res_state = other.res_state;
		res_direction = other.res_direction;
		res_right = other.res_right;
		player = other.player;
		previous = other.previous;
		dormant = other.dormant;
		extrapolated = other.extrapolated;
		strafing = other.strafing;
		velocity = other.velocity;
		final_velocity = other.final_velocity;
		origin = other.origin;
		abs_origin = other.abs_origin;
		obb_mins = other.obb_maxs;
		obb_maxs = other.obb_maxs;
		layers = other.layers;
		memcpy(custom_layers, other.custom_layers, sizeof(custom_layers));
		memcpy(poses, other.poses, sizeof(poses));
		memcpy(state, other.state, sizeof(state));
		sim_time = other.sim_time;
		recv_time = other.recv_time;
		delta_time = other.delta_time;
		server_tick = other.server_tick;
		duck = other.duck;
		lower_body_yaw_target = other.lower_body_yaw_target;
		velocity_modifier = other.velocity_modifier;
		eye_ang = other.eye_ang;
		abs_angle = other.abs_angle;
		memcpy(abs_ang, other.abs_ang, sizeof(abs_ang));
		flags = other.flags;
		eflags = other.eflags;
		effects = other.effects;
		lag = other.lag;
		real_lag = other.real_lag;
		move_state = other.move_state;
		tick_count = other.tick_count;
		head_safety = other.head_safety;
		head_safety_size = other.head_safety_size;
		stamina = other.stamina;
		override_bounds_change_time = other.override_bounds_change_time;
		override_view_height = other.override_view_height;
		return *this;
	}

	void determine_simulation_ticks(lag_record *last);

	inline bool is_moving() const { return velocity.length() > 5.f; }

	inline bool is_moving_on_server() const
	{
		return (layers[6].playback_rate > 0.f && layers[6].weight > 0.f) || !(flags & sdk::cs_player_t::on_ground);
	}

	bool is_breaking_lagcomp() const;

	void update_animations(sdk::user_cmd *cmd = nullptr);
	sdk::anim_state predict_animation_state(sdk::user_cmd *cmd = nullptr);
	void play_additional_animations(sdk::user_cmd *cmd, const sdk::anim_state &pred_state);
	void build_matrix(resolver_direction direction);
	void perform_matrix_setup();
	float get_resolver_roll(std::optional<resolver_direction> direction_override = std::nullopt);

	bool valid = true, resolved{}, shot{}, has_state{}, has_matrix{}, has_previous_matrix{}, has_visual_matrix{},
		 force_safepoint{}, do_not_set{}, breaking_lc{};
	int32_t index{};
	addon_t addon{};
	resolver_state res_state{};
	resolver_direction res_direction{};
	bool res_right{};
	sdk::cs_player_t *player{};
	lag_record *previous{};
	alignas(16) sdk::mat3x4 mat[sdk::max_bones]{};
	alignas(16) sdk::mat3x4 vis_mat[sdk::max_bones]{};
	alignas(16) sdk::mat3x4 res_mat[resolver_direction_max][sdk::max_bones]{};
	alignas(16) sdk::mat3x4 previous_res_mat[resolver_direction_max][sdk::max_bones]{};
	alignas(16) sdk::mat3x4 unprocessed_mat[sdk::max_bones]{};

	bool dormant{};
	bool extrapolated{};
	bool strafing{};
	sdk::vec3 velocity{};
	sdk::vec3 final_velocity{};
	sdk::vec3 origin{};
	sdk::vec3 abs_origin{};
	sdk::vec3 obb_mins{};
	sdk::vec3 obb_maxs{};
	sdk::animation_layers layers{};
	sdk::animation_layers custom_layers[resolver_direction_max]{};
	sdk::pose_paramater poses[resolver_direction_max]{};
	sdk::anim_state state[resolver_direction_max]{};
	float sim_time{};
	float recv_time{};
	float delta_time{};
	int32_t server_tick{};
	float duck{};
	float lower_body_yaw_target{};
	float velocity_modifier{};
	sdk::angle eye_ang{}, abs_angle{};
	sdk::angle abs_ang[resolver_direction_max]{};
	int32_t flags{};
	int32_t eflags{};
	int32_t effects{};
	int32_t lag{}, real_lag{};
	int32_t move_state{};
	int32_t tick_count{};
	int32_t head_safety{};
	float head_safety_size{};
	float stamina{};
	float override_bounds_change_time{};
	float override_view_height{};
};

struct bone_changes
{
	bone_changes() = default;

	explicit bone_changes(sdk::cs_player_t *player)
	{
		bounds_change_time = player->get_bounds_change_time();
		view_height = player->get_view_height();
		view_offset = player->get_view_offset().z;
		eye_ang = *player->eye_angles();
		obb_maxs = player->get_collideable()->obb_maxs();
		ground_entity = player->get_ground_entity();
	}

	void restore(sdk::cs_player_t *player) const
	{
		player->get_bounds_change_time() = bounds_change_time;
		player->get_view_height() = view_height;
		player->get_view_offset().z = view_offset;
		*player->eye_angles() = eye_ang;
		player->get_collideable()->obb_maxs() = obb_maxs;
		player->get_ground_entity() = ground_entity;
	}

	float bounds_change_time;
	float view_height;
	float view_offset;
	sdk::angle eye_ang;
	sdk::vec3 obb_maxs;
	sdk::base_handle ground_entity;
};

namespace aim_helper
{
struct rage_target;
}

struct shot
{
	lag_record *record;
	sdk::vec3 start{}, end{};
	std::vector<sdk::vec3> impacts{};
	uint32_t hitgroup{};
	sdk::cs_player_t::hitbox hitbox{};
	float time{};
	int32_t damage{};
	bool confirmed{}, impacted{}, skip{}, manual{}, alt_attack{};
	int32_t cmd_num{}, cmd_tick{}, server_tick{}, target_tick{};
	std::shared_ptr<aim_helper::rage_target> target = nullptr;
	resolver_direction target_direction{};
	bool target_right{};

	bool spread_miss{};

	struct
	{
		std::vector<sdk::vec3> impacts{};
		int32_t hitgroup{}, damage{}, health{}, index{};
	} server_info;
};

class i_player_list
{
	struct player_entry
	{
		player_entry() = default;
		explicit player_entry(sdk::cs_player_t *const player);

		lag_record *get_record(float target_time, bool visual = false);
		void get_boundary_times(float &first, float &second, bool visual = false);

		void run_extrapolation(bool full_setup);
		void fill_current_real_lag();

		bool is_visually_fakeducking();
		bool is_charged();
		bool is_exploiting();
		bool is_peeking();

		sdk::cs_player_t *player;
		hack_kind hack;

		struct
		{
			float alpha{}, dormant_alpha{}, previous_health{}, previous_armor{}, previous_ammo{};
			uint32_t last_update{};
			sdk::interpolated_value<sdk::vec3> pos{};
			bool is_visible{}, has_matrix{};
			alignas(16) sdk::mat3x4 matrix[sdk::max_bones]{};
			sdk::vec3 abs_org;
		} visuals{};

		struct
		{
			struct resolver_state_t
			{
				inline void switch_to_opposite(const lag_record &record, const resolver_direction dir)
				{
#ifndef NDEBUG
					if (game->dbg.force_resolver_direction.get() != -1)
						return;
#endif
					const auto pos_dir = detail::aim_helper::get_hitbox_position(
						record.player, sdk::cs_player_t::hitbox::head, record.res_mat[dir]);
					if (!pos_dir.has_value())
						return;

					// try to maximize angle delta so that we increase the possibility of hitting next time.
					std::pair<resolver_direction, float> current_delta = {resolver_networked, FLT_EPSILON};
					const auto ang_dir = calc_angle(record.origin, *pos_dir);

					// perform reduction over all open positions.
					for (auto i = 0; i < resolver_direction_max; i++)
					{
						const auto pos = detail::aim_helper::get_hitbox_position(
							record.player, sdk::cs_player_t::hitbox::head, record.res_mat[i]);
						if (!pos.has_value())
							continue;

						const auto ang = calc_angle(record.origin, *pos);
						const auto delta = fabsf(std::remainderf(ang.y - ang_dir.y, sdk::yaw_bounds));

						if (!eliminated_positions.test(i) && delta > current_delta.second)
							current_delta = {static_cast<resolver_direction>(i), delta};
					}

					// set new resolve direction.
					if (current_delta.second != FLT_EPSILON)
						set_direction(current_delta.first);
				}

				inline void set_direction(resolver_direction dir)
				{
#ifndef NDEBUG
					if (game->dbg.force_resolver_direction.get() != -1)
						return;
#endif
					direction = dir;
					eliminated_positions.reset(dir);
				}

				enum resolver_direction direction = resolver_networked;
				std::bitset<resolver_direction_max> eliminated_positions{};
			} states[2][resolver_state_max]{};

			inline resolver_state_t &get_resolver_state(const lag_record &record)
			{
				return states[record.res_right][record.res_state];
			}

			inline void map_resolver_state(const lag_record &record, const resolver_state_t &state)
			{
#ifndef NDEBUG
				if (game->dbg.force_resolver_direction.get() != -1)
					return;
#endif

				// synchronize result to other states.
				if (record.res_state != resolver_shot)
					for (auto i = 0; i < resolver_state_max - 2; i++)
					{
						if (i == record.res_state)
							continue;

						auto &other = states[record.res_right][i];
						auto target = state.direction;

						// should we invert?
						if (i == resolver_shot || steadiness >= .5f)
							switch (state.direction)
							{
							case resolver_max_max:
								target = resolver_min_min;
								break;
							case resolver_max:
							case resolver_max_extra:
								target = resolver_min;
								break;
							case resolver_min:
							case resolver_min_extra:
								target = resolver_max;
								break;
							case resolver_min_min:
								target = resolver_max_max;
								break;
							default:
								target = resolver_networked;
								break;
							}

						if (!other.eliminated_positions.test(target))
							other.direction = target;
					}
			}

			enum resolver_state current_state
			{
			};
			float steadiness{}, continuous_momentum{}, pitch_timer{}, detected_yaw{};
			bool detected_layer{}, stop_detection{}, right{}, previous_freestanding{}, unreliable_extrapolation{};
			int8_t unreliable{};
		} resolver{};

		struct
		{
			int32_t last_target_tick{};
			float target_time = -1.f;
		} aimbot{};

		float spawntime{}, compensation_time{}, dt_interp{}, ground_accel_linear_frac_last_time{};
		int32_t addt{}, ground_accel_linear_frac_last_time_stamp{}, last_recv_data{};
		sdk::base_handle handle{};
		int32_t spread_miss{}, resolver_miss{}, dormant_miss{};
		bool pvs{}, hittable{}, danger{};
		int8_t pitch_alt{};
		uint32_t pitch_prev{};
		sdk::vec3 previous_angle{};

		float old_maxs_z = FLT_MAX;
		float view_delta = 0.f;
		float smooth_bounds_change_time = 0.f;
		bone_changes bones{};

		util::circular_buffer<lag_record> records{};
		util::circular_buffer<lag_record> scan_records{};
		util::circular_buffer<lag_record> shot_records{};
		lag_record dormant_record{};
		sdk::animation_layers server_layers{};
		float lower_body_yaw_target{};
		std::shared_ptr<aim_helper::rage_target> last_target{};
		sdk::interpolated_value<sdk::vec3> interpolated_target{};
	};

public:
	inline i_player_list::player_entry &get(sdk::cs_player_t *const player)
	{
		if (!player)
			INVALID_ARGUMENT("Player pointer was null.");

		const auto index = player->index();

		if (index < 1 || index > 65)
			RUNTIME_ERROR("Invalid player index");

		auto &entry = entries[index - 1];

		if (entry.handle != player->get_handle())
		{
			const auto first = !entry.handle;

			entry = player_entry(player);

			if (first)
				entry.server_layers = *player->get_animation_layers();
		}
		else if (entry.spawntime != player->get_spawn_time())
		{
			if (player->index() != game->engine_client->get_local_player())
				player->get_local().init_local_data();

			// copy old data.
			const auto server_layers = entry.server_layers;
			const auto resolver = entry.resolver;
			const auto next = player_entry(player);

			__asm mfence;

			entry = next;

			// restore old data.
			entry.server_layers = server_layers;
			entry.resolver = resolver;
			entry.resolver.unreliable = max(entry.resolver.unreliable - 1, 0);
			entry.resolver.unreliable_extrapolation = false;

			// clear positions.
			for (auto i = 0; i < 2; i++)
				for (auto j = 0; j < resolver_state_max; j++)
				{
					auto &state = entry.resolver.states[i][j];
					state.eliminated_positions.reset();
					if (state.direction != resolver_max_max && state.direction != resolver_min_min &&
						state.direction != resolver_min_extra && state.direction != resolver_max_extra)
					{
						state.eliminated_positions.set(resolver_max_max);
						state.eliminated_positions.set(resolver_max_extra);
						state.eliminated_positions.set(resolver_min_min);
						state.eliminated_positions.set(resolver_min_extra);
					}
				}
		}
		else if (entry.player != player)
		{
			entry.player = player;

			for (auto &record : entry.records)
				record.player = player;
		}

		return entry;
	}

	void reset();

	void on_update_end(sdk::cs_player_t *const player);
	void on_shot_resolve(shot &s, const sdk::vec3 &end, const bool spread_miss);
	void on_target_player(sdk::cs_player_t *const player);
	void refresh_local();
	void merge_local_animation(sdk::cs_player_t *const player, sdk::user_cmd *cmd);
	void update_addon_bits(sdk::cs_player_t *const player);
	void build_fake_animation(sdk::cs_player_t *const player, sdk::user_cmd *const cmd);
	void build_activity_modifiers(sdk::cs_player_t *const player, std::array<uint16_t, 3> &modifiers,
								  bool uh = false) const;
	float calculate_yaw_modifier(sdk::cs_player_t *const player, const sdk::vec3 &velocity, float duck,
								 float ground_fraction);
	void perform_animations(lag_record *const current, lag_record *const last = nullptr, bool long_path = true);
	void send_esp_data(sdk::cs_player_t *player);
	void recv_esp_data(uint8_t *data, int from);

	std::array<player_entry, 64> entries;

private:
	void update_resolver_state(lag_record *const current, lag_record *const last);
};

extern i_player_list player_list;
extern lag_record local_record, local_shot_record, local_fake_record;

struct animation_copy
{
	animation_copy() = default;

	animation_copy(int32_t sequence, sdk::cs_player_t *player)
	{
		this->sequence = sequence;
		checksum = game->input->verified_commands[sequence % sdk::input_max].crc;
		state = *player->get_anim_state();
		this->addon = local_record.addon;
		layers = *player->get_animation_layers();
		poses = player->get_pose_parameter();
		lower_body_yaw_target = player->get_lower_body_yaw_target();
		eye = *player->eye_angles();
		get_strafing = player->get_strafing();
		entire_body = player->get_snapshot_entire_body();
		upper_body = player->get_snapshot_upper_body();
		spawn_time = player->get_spawn_time();
	}

	void restore(sdk::cs_player_t *player) const
	{
		const auto bak = *player->get_anim_state();
		*player->get_anim_state() = state;
		bak.copy_meta(player->get_anim_state());
		local_record.addon = this->addon;
		*player->get_animation_layers() = layers;
		player->get_pose_parameter() = poses;
		player->get_lower_body_yaw_target() = lower_body_yaw_target;
		player->get_snapshot_entire_body() = entire_body;
		player->get_snapshot_upper_body() = upper_body;
	}

	int32_t sequence = INT_MAX;
	int32_t checksum = INT_MAX;
	sdk::anim_state state{};
	addon_t addon{};
	sdk::animation_layers layers{};
	sdk::pose_paramater poses{};
	float lower_body_yaw_target{};
	sdk::angle eye{};
	bool get_strafing{};
	sdk::bone_snapshot entire_body{};
	sdk::bone_snapshot upper_body{};
	float spawn_time{};
};
} // namespace detail

#define GET_PLAYER_ENTRY(player) detail::player_list.get(player)

#endif // DETAIL_PLAYER_LIST_H
