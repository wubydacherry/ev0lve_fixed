#ifndef SDK_PLANTEDC4_H
#define SDK_PLANTEDC4_H

#include <detail/custom_tracing.h>
#include <sdk/cs_player.h>

namespace sdk
{
constexpr auto plant_time = 3.2f;

class planted_c4_t : public entity_t
{
public:
	VAR(offsets::planted_c4, bomb_ticking, bool);
	VAR(offsets::planted_c4, bomb_site, int);
	VAR(offsets::planted_c4, c4_blow, float);
	VAR(offsets::planted_c4, timer_length, float);
	VAR(offsets::planted_c4, defuse_count_down, float);
	VAR(offsets::planted_c4, bomb_defused, bool);
	VAR(offsets::planted_c4, defuse_length, float);
	VAR(offsets::planted_c4, bomb_defuser, base_handle);

	float calculate_damage_to_player(sdk::cs_player_t *player)
	{
		if (!player || !player->is_alive())
			return 0.f;

		// TODO: to make this more precise, take damage from map info
		float damage{500.f};

		// end_pos: game defaults this to "noisy", so it can be random - let's use something in the middle of hardcoded
		// 0.7-1.0
		vec3 pos{get_abs_origin()}, end_pos{player->get_abs_origin() + player->get_view_offset() * 0.85f},
			to_target{end_pos - pos};
		pos.z += 1.f; // "in case grenade is lying on the ground" (c) valve employee

		// apply gaussian damage calc
		const auto radius = damage * 3.5f;
		const auto distance = to_target.length();
		const auto sigma = radius / 3.f;
		const auto adjusted_damage = damage * expf(-distance * distance / (2.f * sigma * sigma));

		// no damage can be taken
		if (adjusted_damage <= 0.f)
			return 0.f;

		return ceilf(detail::trace.scale_damage(player, adjusted_damage, 1.f, hitgroup_generic)) - 1.f;
	}
};
} // namespace sdk

#endif // SDK_PLANTEDC4_H
