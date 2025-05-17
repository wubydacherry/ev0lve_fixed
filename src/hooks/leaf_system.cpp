
#include <base/cfg.h>
#include <base/hook_manager.h>

using namespace sdk;

namespace hooks::leaf_system
{
void __stdcall calc_renderable_world_space_aabb_bloated(renderable_info *info, vec3 *mins, vec3 *maxs)
{
	const auto ent = info && info->renderable && info->renderable->get_client_unknown()
						 ? reinterpret_cast<cs_player_t *>(info->renderable->get_client_unknown()->get_base_entity())
						 : nullptr;
	if (ent && (ent->is_player() || ent->get_class_id() == class_id::csragdoll) && game->local_player &&
		game->local_player->is_valid())
	{
		const auto old = info->renderable;
		info->renderable = game->local_player;
		hook_manager.calc_renderable_world_space_aabb_bloated->call(info, mins, maxs);
		info->renderable = old;
	}
	else
		hook_manager.calc_renderable_world_space_aabb_bloated->call(info, mins, maxs);
}

void __stdcall add_renderables_to_render_list(uintptr_t info, int32_t count, renderable_info **renderables,
											  uintptr_t rl_info, int32_t dcount, uintptr_t dinfo)
{
	for (auto i = 0; i < count; i++)
	{
		auto el = renderables[i];
		if (reinterpret_cast<size_t>(el) & 1)
			continue;

		if (el->model_type == 2 && !el->disable_csm_rendering)
			el->translucency_type = cfg.local_visuals.transparent_prop_value.get() > 0.f ? 1 : 2;

		if (el->model_type == 1)
		{
			const auto ent =
				el && el->renderable && el->renderable->get_client_unknown()
					? reinterpret_cast<cs_player_t *>(el->renderable->get_client_unknown()->get_base_entity())
					: nullptr;
			if (ent && ent->is_player())
				el->translucency_type = cfg.removals.no_smoke.get();
		}
	}

	hook_manager.add_renderables_to_render_list->call(info, count, renderables, rl_info, dcount, dinfo);
}

int32_t __stdcall extract_occluded_renderables(int32_t view_id, int32_t count, renderable_info **renderables,
											   uintptr_t rl_info)
{
	for (auto i = 0; i < count; i++)
	{
		auto el = renderables[i];
		if (reinterpret_cast<size_t>(el) & 1)
			continue;

		const auto ent = el && el->renderable && el->renderable->get_client_unknown()
							 ? reinterpret_cast<cs_player_t *>(el->renderable->get_client_unknown()->get_base_entity())
							 : nullptr;

		if (ent && ent->is_player())
			*(bool *)(rl_info + 0x14 + 0x1C * i + 7) = false;
	}

	return hook_manager.extract_occluded_renderables->call(view_id, count, renderables, rl_info);
}
} // namespace hooks::leaf_system
