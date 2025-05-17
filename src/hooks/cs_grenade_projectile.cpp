
#include <base/cfg.h>
#include <base/hook_manager.h>
#include <hacks/grenade_prediction.h>

using namespace sdk;
using namespace hacks;

namespace hooks::cs_grenade_projectile
{
void __fastcall client_think(weapon_t *wpn, uint32_t edx)
{
	if (!game->local_player || !cfg.world_esp.grenade_warning.get())
		return hook_manager.client_think->call(wpn, edx);

	reinterpret_cast<weapon_t *>(uintptr_t(wpn) - 0xC)->set_next_client_think(game->globals->curtime);
}

void __fastcall post_data_update(weapon_t *wpn, uint32_t edx, uint32_t type)
{
	hook_manager.projectile_post_data_update->call(wpn, edx, type);

	if (game->local_player && cfg.world_esp.grenade_warning.get())
	{
		wpn = reinterpret_cast<weapon_t *>(uintptr_t(wpn) - 8);
		const auto thrower = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity_from_handle(wpn->get_thrower()));

		if (thrower && thrower->is_player())
		{
			const auto old = wpn->get_last_trail_pos();

			wpn->get_last_trail_pos() = grenade_predict
											->to_end(wpn, thrower, wpn->get_class_id() == class_id::molotov_projectile,
													 &wpn->get_next_trail_line())
											.value_or(vec3(FLT_MAX, FLT_MAX, FLT_MAX));

			if (type != 0 && (old - wpn->get_last_trail_pos()).length() < 8.f)
				wpn->get_last_trail_pos() = old;
		}
	}
}
} // namespace hooks::cs_grenade_projectile
