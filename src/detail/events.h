
#ifndef DETAIL_EVENTS_H
#define DETAIL_EVENTS_H

#include <sdk/game_event_manager.h>

namespace detail
{
class events : public sdk::game_event_listener
{
public:
	void fire_game_event(sdk::game_event *event) override;
	int get_event_debug_id() override;

	bool is_round_end{}, is_game_end{}, is_freezetime{};
};

extern events evnt;
} // namespace detail

#endif // DETAIL_EVENTS_H
