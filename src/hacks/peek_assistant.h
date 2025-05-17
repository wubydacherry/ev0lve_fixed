
#ifndef HACKS_PEEK_ASSISTANT_H
#define HACKS_PEEK_ASSISTANT_H

#include <sdk/input.h>

namespace hacks
{
extern struct peek_assistant_t
{
	void on_before_move();
	void on_create_move(sdk::user_cmd *cmd);
	void on_post_move(sdk::user_cmd *cmd);
	void take_shot();

	std::optional<sdk::vec3> pos{};
	sdk::vec3 shot_pos{};
	float box_time{}, wall_dist{};
	bool has_shot{}, state{};
	uint32_t buttons_current{}, buttons_debounce{};
} peek_assistant;
} // namespace hacks

#endif // HACKS_PEEK_ASSISTANT_H
