
#ifndef SDK_GAME_MOVEMENT_H
#define SDK_GAME_MOVEMENT_H

#include <sdk/cs_player.h>
#include <sdk/input.h>

namespace sdk
{
class move_data
{
public:
	bool first_run_of_functions : 1;
	bool game_code_moved_player : 1;
	bool no_air_control : 1;

	unsigned long player_handle{};
	int impulse_command{};
	angle view_angles;
	angle abs_view_angles;
	int buttons{};
	int old_buttons{};
	float forward_move{};
	float side_move{};
	float up_move{};

	float max_speed{};
	float client_max_speed{};

	vec3 velocity;
	vec3 old_velocity;
	float somefloat{};
	angle angles;
	angle old_angles;

	float out_step_height{};
	vec3 out_wish_velocity;
	vec3 out_jump_velocity;

	vec3 constraint_center;
	float constraint_radius{};
	float constraint_width{};
	float constraint_speed_factor{};
	bool constraint_past_radius{};

	vec3 abs_origin;
};

struct move_delta_t;

class cs_game_movement_t
{
	VIRTUAL(1, process_movement_int(cs_player_t *const player, move_data *const data),
			void(__thiscall *)(void *, cs_player_t *, move_data *))
	(player, data);

public:
	inline static auto is_processing = false;

	VIRTUAL(3, start_track_prediction_errors(cs_player_t *const player), void(__thiscall *)(void *, cs_player_t *))
	(player);
	VIRTUAL(4, finish_track_prediction_errors(cs_player_t *const player), void(__thiscall *)(void *, cs_player_t *))
	(player);
	VIRTUAL(8, get_view_offset(const bool ducked), const vec3 &(__thiscall *)(void *, bool))(ducked);
	VIRTUAL(12, setup_movement_bounds(move_data *const data), void(__thiscall *)(void *, move_data *))(data);

	move_data setup_move(cs_player_t *const player, user_cmd *cmd);
	move_delta_t process_movement(cs_player_t *const player, move_data *const data);
	static void air_move(cs_player_t *const player, move_data *const data);
	static void air_accelerate(vec3 const &wishdir, float wishspeed, move_data *mv, float friction);
	static void check_parameters(move_data *mv, cs_player_t *player);
	static void walk_move(move_data *mv, cs_player_t *player);
	static void accelerate(vec3 const &wishdir, float wishspeed, float accel, move_data *mv, cs_player_t *const player);
	static void friction(move_data *mv, cs_player_t *const player);
};
} // namespace sdk

#endif // SDK_GAME_MOVEMENT_H
