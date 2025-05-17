
#ifndef SDK_GAME_RULES_H
#define SDK_GAME_RULES_H

#include <sdk/macros.h>

#define sv_maxusrcmdprocessticks ((game->rules->data && game->rules->data->get_is_valve_ds()) ? 7 : 16)

namespace sdk
{
struct view_vectors
{
	vec3 view;

	vec3 hull_min;
	vec3 hull_max;

	vec3 duck_hull_min;
	vec3 duck_hull_max;
	vec3 duck_view;

	vec3 obs_hull_min;
	vec3 obs_hull_max;

	vec3 dead_view_height;
};

class cs_game_rules_t
{
public:
	struct
	{
		VAR(offsets::csgame_rules, freeze_period, bool);
		VAR(offsets::csgame_rules, warmup_period, bool);
		VAR(offsets::csgame_rules, match_waiting_for_resume, bool);
		VAR(offsets::csgame_rules, is_valve_ds, bool);
		VAR(offsets::csgame_rules, play_all_step_sounds_on_server, bool);

		VIRTUAL(30, get_view_vectors(), view_vectors *(__thiscall *)(void *))();
	} * data;
};
} // namespace sdk

#endif // SDK_GAME_RULES_H
