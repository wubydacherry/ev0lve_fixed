
#include <base/hook_manager.h>

using namespace sdk;

namespace hooks::game_movement
{
void __fastcall process_movement(uintptr_t movement, uint32_t edx, cs_player_t *player, move_data *data)
{
	data->game_code_moved_player = false;
	hook_manager.process_movement->call(movement, edx, player, data);
}
} // namespace hooks::game_movement
