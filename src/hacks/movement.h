
#ifndef HACKS_MOVEMENT_H
#define HACKS_MOVEMENT_H

#include <base/cfg.h>
#include <sdk/cs_player.h>
#include <sdk/engine_client.h>
#include <sdk/math.h>

namespace hacks
{
class movement
{
public:
	void bhop(sdk::user_cmd *cmd);
	void fix_bhop(sdk::user_cmd *cmd);
	void edge_jump(sdk::user_cmd *cmd);

private:
	void autostrafe(sdk::user_cmd *cmd, bool can_jump);

	bool disabled_this_interval{}, side_switch{};
	uint32_t flags_backup{};
};

extern movement mov;
} // namespace hacks

#endif // HACKS_MOVEMENT_H
