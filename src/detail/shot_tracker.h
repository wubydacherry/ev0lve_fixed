
#ifndef DETAIL_SHOT_TRACKER_H
#define DETAIL_SHOT_TRACKER_H

#include <detail/player_list.h>
#include <sdk/game_event_manager.h>

namespace detail
{
class shot_tracker
{
public:
	void register_shot(shot &&s);
	int32_t calculate_health_correction(sdk::cs_player_t *player) const;
	void on_player_hurt(sdk::game_event *event);
	void on_bullet_impact(sdk::game_event *event);
	void on_weapon_fire(sdk::game_event *event);
	void on_update_end();

	void on_shot(shot &s);

	std::deque<shot> shots{};
};

extern shot_tracker shot_track;
} // namespace detail

#endif // DETAIL_SHOT_TRACKER_H
