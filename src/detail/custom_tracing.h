
#ifndef DETAIL_CUSTOM_TRACING_H
#define DETAIL_CUSTOM_TRACING_H

#include <detail/player_list.h>
#include <mutex>
#include <sdk/cs_player.h>
#include <sdk/engine_trace.h>
#include <sdk/weapon_system.h>
#include <thread>

namespace detail
{
class custom_tracing
{
public:
	enum class bullet_safety
	{
		none,
		no_roll,
		full
	};

	struct wall_pen
	{
		int32_t damage{}, potential_damage{};
		sdk::cs_player_t::hitbox hitbox{};
		int32_t hitgroup{};
		std::array<sdk::vec3, 6> impacts{};
		uint8_t impact_count{};
		sdk::vec3 direction{}, end{};
		bool did_hit{};
		bullet_safety safety{};
	};

	wall_pen wall_penetration(sdk::vec3 src, sdk::vec3 end, lag_record *target,
							  bullet_safety scan_secure = bullet_safety::none,
							  std::optional<resolver_direction> override_direction = std::nullopt,
							  sdk::cs_player_t *override_player = nullptr, bool no_opt = false,
							  sdk::cs_weapon_data *override_data = nullptr, int32_t expected_hitgroup = -1);
	bool trace_to_studio_csgo_hitgroups_priority(sdk::cs_player_t *player, uint32_t contents_mask, sdk::vec3 *origin,
												 sdk::game_trace *tr, sdk::ray *r, sdk::mat3x4 **mat) const;
	float scale_damage(sdk::cs_player_t *target, float damage, float weapon_armor_ratio, int hitgroup) const;

	void check_wallbang();
	[[nodiscard]] bool can_wallbang() const { return wallbang; }

	inline static constexpr auto ray_extension = 40.f;

private:
	wall_pen fire_bullet(sdk::cs_weapon_data *data, sdk::vec3 src, sdk::vec3 pos, sdk::trace_filter *filter,
						 lag_record *target, bullet_safety scan_secure,
						 std::optional<resolver_direction> override_direction, bool is_zeus, bool no_opt,
						 int32_t expected_hitgroup);
	bool handle_bullet_penetration(const sdk::cs_weapon_data *weapon_data, sdk::game_trace &enter_trace,
								   sdk::vec3 &eye_position, sdk::vec3 direction, int &penetrate_count,
								   float &current_damage, float penetration_power) const;
	bool trace_to_exit(sdk::game_trace &enter_trace, sdk::game_trace &exit_trace, sdk::vec3 start_position,
					   sdk::vec3 direction) const;

	bool wallbang = false;
};

extern custom_tracing trace;
} // namespace detail

#endif // DETAIL_CUSTOM_TRACING_H
