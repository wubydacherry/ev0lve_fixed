
#include <base/hook_manager.h>
#include <hacks/chams.h>
#include <hacks/grenade_prediction.h>
#include <hacks/misc.h>
#include <hacks/visuals.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

using namespace sdk;
using namespace detail;
using namespace hacks;

namespace hooks::client_mode
{
void __fastcall override_view(client_mode_t *client_mode, uint32_t edx, view_setup *setup)
{
	if (game->engine_client->is_ingame() && game->engine_client->is_connected())
	{
		hacks::misc::on_override_view();

		if (!game->local_player || !game->local_player->is_valid())
		{
			game->input->camera_in_third_person = false;

			// fixup fov while dead, so whenever people are scoping it doesn't make the view look ugly
			if (cfg.local_visuals.thirdperson_dead.get())
			{
				const auto target_fov = cfg.local_visuals.fov_changer.get() ? cfg.local_visuals.fov.get() : 0.f;
				setup->fov = 90.f + target_fov;
			}

			return hook_manager.override_view->call(client_mode, edx, setup);
		}

		vis->on_override_view(setup, true);
		hook_manager.override_view->call(client_mode, edx, setup);
		vis->on_override_view(setup);
	}
	else
		hook_manager.override_view->call(client_mode, edx, setup);
}

void __fastcall do_post_screen_space_effects(client_mode_t *client_mode, uint32_t edx, view_setup *setup)
{
	cham.extend_chams();
	if (game->local_player && game->local_player->is_valid() && game->local_player->is_alive())
		game->glow_manager->clear();
	vis->draw_glow();
	grenade_predict->draw();

	// draw our own glow before the game glow.
	if (cfg.player_esp.target_scan.get())
	{
		auto &object_definition_count = *(uint32_t *)(uintptr_t(game->glow_manager()) + 12);
		const auto backup_count = object_definition_count;
		object_definition_count = 0;
		hook_manager.do_post_screen_space_effects->call(client_mode, edx, setup);
		object_definition_count = backup_count;
	}

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_do_post_screen_space_effects"));
#endif

	// finally, draw the game glow.
	hook_manager.do_post_screen_space_effects->call(client_mode, edx, setup);
}
} // namespace hooks::client_mode
