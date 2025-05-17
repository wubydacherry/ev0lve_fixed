
#include <base/hook_manager.h>

using namespace sdk;

namespace hooks::trace_filter_for_player_head_collision
{
bool __fastcall should_hit_entity(uintptr_t filter, uint32_t edx, entity_t *entity, int32_t contents_mask)
{
	if (entity && game->local_player && entity->get_team_num() == game->local_player->get_team_num() &&
		entity->index() != game->local_player->index() &&
		fabsf(entity->get_abs_origin().z - game->local_player->get_abs_origin().z) < 10.f)
		return false;

	return hook_manager.should_hit_entity->call(filter, edx, entity, contents_mask);
}
} // namespace hooks::trace_filter_for_player_head_collision
