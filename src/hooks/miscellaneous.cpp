
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/entity_rendering.h>
#include <gui/gui.h>
#include <hacks/chams.h>
#include <hacks/grenade_prediction.h>
#include <hacks/misc.h>
#include <hacks/skinchanger.h>
#include <hacks/visuals.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

#undef xor

using namespace detail;
using namespace sdk;
using namespace hacks;
using namespace hacks::misc;

namespace hooks::miscellaneous
{
int32_t __stdcall get_module_handle_ex_a(uint32_t flags, const char *str, uint32_t *module)
{
	uintptr_t **_ebp;
	__asm mov _ebp, ebp;
	auto &ret_addr = _ebp[1];

	//if (*ret_addr == functions::return_to_is_module_whitelisted)
	if (*ret_addr == 0x2374c085)
	{
		ret_addr = (uintptr_t *)(uintptr_t(ret_addr) + 0x47);
		return 1;
	}

	return hook_manager.get_module_handle_ex_a->call(flags, str, module);
}

void __cdecl host_shutdown()
{
	hook_manager.host_shutdown->call();
	hook_manager.detach();

	cham.reload(true);
	dx_adapter::destroy_objects();
	dx_adapter::icon_textures.clear();
	evo::ren::draw.destroy_objects();
	evo::ren::draw = {};
	dispatch.decomission();
}

void __fastcall hud_draw_scope(uintptr_t hud, uint32_t edx)
{
	if (cfg.removals.scope->test(cfg_t::scope_default))
		return hook_manager.hud_draw_scope->call(hud, edx);
}

int32_t __fastcall particle_draw_model(uintptr_t particle, uint32_t edx, uint32_t flags, uint32_t instance)
{
	const auto render_context = game->material_system->get_render_context();
	const auto brightness =
		1.f + (cfg.local_visuals.night_mode.get() ? cfg.local_visuals.night_mode_value.get() * .01f : 0.f);
	vec3 def(brightness, brightness, brightness);
	auto org = render_context->get_tone_mapping_scale_linear();
	render_context->set_tone_mapping_scale_linear(&def);
	const auto ret = hook_manager.particle_draw_model->call(particle, edx, flags, instance);
	render_context->set_tone_mapping_scale_linear(&org);
	return ret;
}

void __fastcall ent_info_list_link_before(ent_info_list_t *list, uint32_t edx, ent_info_t *before, ent_info_t *element)
{
	before = game->client_entity_list->get_next_info(element);
	if (!before || list != game->client_entity_list->get_active_list())
		return hook_manager.ent_info_list_link_before->call(list, edx, before, element);

	if (element->prev != element)
	{
		if (element->prev)
			element->prev->next = element->next;
		else
			list->head = element->next;

		if (element->next)
			element->next->prev = element->prev;
		else
			list->tail = element->prev;

		element->prev = element->next = element;
	}

	element->next = before;
	element->prev = before->prev;
	before->prev = element;

	if (element->prev)
		element->prev->next = element;
	else
		list->head = element;
}

bool __fastcall process_input(uintptr_t overhead, uint32_t edx)
{
	if (cfg.player_esp.team.get().none())
		game->prediction->set_local_view_angles(game->engine_client->get_view_angles());
	else
		game->prediction->set_local_view_angles({0.f, -89.f, 0.f});

	return hook_manager.process_input->call(overhead, edx);
}

int32_t __fastcall start_sound_immediate(start_sound_params *params, uint32_t edx)
{
	static const auto player_footsteps = XOR_STR_STORE("player\\footsteps");
	static const auto zoom_wav = XOR_STR_STORE("zoom");

	if (game->cs_game_movement->is_processing || !params || !params->sfx)
		return -1;

	char name[MAX_PATH];
	params->sfx->get_name(name, MAX_PATH);

	if (params->from_server && params->sound_source == game->engine_client->get_local_player())
	{
		XOR_STR_STACK(footsteps, player_footsteps);
		XOR_STR_STACK(zoom, zoom_wav);
		if (strstr(name, footsteps) || strstr(name, zoom))
			return -1;
	}

	misc::on_sound(params, name);
	return hook_manager.start_sound_immediate->call(params, edx);
}

// this is probably to avoid console messages?
const char *__fastcall check_param(uintptr_t cmd, uint32_t edx, const char *sz, const char **values)
{
	// "E8 ?? ?? ?? ?? 83 C4 04 5E 8B E5 5D C3 80 7E 58 00"
	// S_StartSoundEntry: "55 8B EC 83 E4 F8 81 EC DC 01"
	if (game->cs_game_movement->is_processing &&
		uintptr_t(_ReturnAddress()) == game->engine.at(functions::return_to_start_sound_entry))
		return (const char *)1;

	return hook_manager.check_param->call(cmd, edx, sz, values);
}

void __fastcall move_helper_start_sound(uintptr_t move_helper, uint32_t edx, sdk::vec3 *origin, const char *name)
{
	// fixes splash sound crash in prediction rules.
	if (!game->cs_game_movement->is_processing)
		hook_manager.move_helper_start_sound->call(move_helper, edx, origin, name);
}

void __cdecl get_tonemap_settings_from_env_tonemap_controller()
{
	static constexpr auto offsets =
		std::array{0xCE, 0xEB, 0x108, 0x00, 0x70, 0x70, 0x70, 0x70, 0x80, 0x80, 0x80, 0x80, 0x90, 0x90, 0x90, 0x90};

	hook_manager.get_tonemap_settings_from_env_tonemap_controller->call();

	if (cfg.local_visuals.night_mode.get())
	{
		struct
		{
			bool use_custom_auto_exposure_min, use_custom_auto_exposure_max, use_custom_bloom_scale;
			float custom_auto_exposure_min, custom_auto_exposure_max, custom_bloom_scale;
		} tonemap_settings;

		tonemap_settings.use_custom_auto_exposure_min = tonemap_settings.use_custom_auto_exposure_max = true;
		tonemap_settings.use_custom_bloom_scale = !cfg.local_visuals.disable_bloom;

		const auto factor = std::clamp(.5f - (cfg.local_visuals.night_mode_value.get() * .01f) * .475f, 0.01f, 1.f);
		tonemap_settings.custom_auto_exposure_min = tonemap_settings.custom_auto_exposure_max = factor;
		if (!cfg.local_visuals.disable_bloom)
			tonemap_settings.custom_bloom_scale =
				.2f + std::clamp(cfg.local_visuals.night_mode_value.get() * .01f, 0.f, 1.f) * 3.75f;
		else
			tonemap_settings.custom_bloom_scale = 0.f;

		for (auto i = 0u; i < offsets.size(); i++)
		{
			const auto offset = offsets[i];
			auto rollback = 0u;

			if (offset == 0)
				continue;
			else if (i > 0)
				do
				{
					if (offsets[i - rollback - 1] != offset)
						break;
				} while (rollback++ < i);

			*reinterpret_cast<uint8_t *>(
				*reinterpret_cast<uintptr_t *>(
					uintptr_t(hook_manager.get_tonemap_settings_from_env_tonemap_controller->get_target()) + offset) +
				rollback) = *reinterpret_cast<uint8_t *>(uintptr_t(&tonemap_settings) + i);
		}
	}
}

void __fastcall draw_mesh(uintptr_t dx8_api, uint32_t edx, uint32_t mesh, uint32_t count, mesh_instance_data *instances,
						  uint32_t compression, uint32_t compiled_state, uint32_t info)
{
	auto &mat = *reinterpret_cast<material **>(dx8_api + 0x2FA4);

	if (mat && (mat->get_group()[6] == 'P' ||
				(mat->get_name()[7] == 'p' && mat->get_name()[8] == 'r' && mat->get_name()[9] == 'o' &&
				 mat->get_name()[17] == 'c' && mat->get_name()[18] == 'h' && mat->get_name()[19] == 'i')))
		for (auto i = 0; i < count; i++)
			instances[i].diffuse_modulation.w = 1.f - .01f * cfg.local_visuals.transparent_prop_value.get();

	hook_manager.draw_mesh->call(dx8_api, edx, mesh, count, instances, compression, compiled_state, info);
}

void __fastcall draw_3d_debug_overlays(uintptr_t view, uint32_t edx)
{
	const auto render_context = game->material_system->get_render_context();
	vec3 def(1.f, 1.f, 1.f);
	auto org = render_context->get_tone_mapping_scale_linear();
	render_context->set_tone_mapping_scale_linear(&def);
	hook_manager.draw_3d_debug_overlays->call(view, edx);
	render_context->set_tone_mapping_scale_linear(&org);
}

void __fastcall render_glow_boxes(glow_manager_t *glow, uintptr_t edx, int32_t pass, mat_render_context **ctx)
{
	static auto diff = 0;
	auto &count = *(uint32_t *)(uintptr_t(glow) + 0x24);

	// this will cause problems in the future, but i don't care.
	count -= diff;
	diff = 0;

	if (glow->glow_object_definitions.count() > 0)
	{
		hook_manager.render_glow_boxes->call(glow, edx, pass, ctx);
		return;
	}

	game->client_entity_list->for_each_player(
		[&](cs_player_t *player)
		{
			if (player->index() == game->engine_client->get_local_player())
				return;

			auto &entry = GET_PLAYER_ENTRY(player);

			if (!player->get_studio_hdr() || !player->is_alive() || !entry.last_target ||
				entry.last_target->record->recv_time + 2.f < game->globals->curtime)
				return;

			const auto hdr = player->get_studio_hdr()->hdr;

			if (!hdr || !player->get_bone_accessor().out)
				return;

			const auto set = hdr->get_hitbox_set(player->get_hitbox_set());
			if (!set)
				return;

			const auto hitbox = set->get_hitbox(int32_t(entry.last_target->pen.hitbox));
			if (!hitbox)
				return;

			const auto clr = entry.last_target->pen.safety != custom_tracing::bullet_safety::none
								 ? cfg.player_esp.target_scan_secure.get()
								 : cfg.player_esp.target_scan_aim.get();
			const auto rad = max(hitbox->radius, 2.5f);
			const auto pos =
				player->get_abs_origin() + entry.interpolated_target.advance(.5f * game->globals->frametime);

			if (pass == 1)
			{
				stencil_state state{};
				(*ctx)->set_stencil_state(&state);
				draw_sphere(pos, rad, clr);
				count++;
				diff++;
			}
			else if (pass == 0)
				draw_sphere(pos, rad, clr);
		});
}

void __fastcall spawn_smoke_effect(uintptr_t smoke, uint32_t edx)
{
	if (!cfg.removals.no_smoke.get())
		hook_manager.spawn_smoke_effect->call(smoke, edx);
}

void __fastcall render_iron_sight_scope_effect(uintptr_t iron_sight, uint32_t edx, int32_t x, int32_t y, int32_t w,
											   int32_t h, uintptr_t view_setup)
{
	if (!cfg.local_visuals.fov_changer.get() || !game->input->camera_in_third_person)
		hook_manager.render_iron_sight_scope_effect->call(iron_sight, edx, x, y, w, h, view_setup);
}

void __cdecl convar_network_change_callback(convar *var, const char *old1, float old2)
{
	const auto var2 = reinterpret_cast<convar *>(uintptr_t(var) - 0x18);

	if (var->has_flag(1 << 9))
	{
		if (FNV1A_CMP(var->get_base_name(), "cl_lagcompensation"))
			game->cl_lagcompensation = !FNV1A_CMP(var2->value.string, "0");
		if (FNV1A_CMP(var->get_base_name(), "cl_predict"))
			game->cl_predict = !FNV1A_CMP(var2->value.string, "0");
	}

	hook_manager.convar_network_change_callback->call(var, old1, old2);
}

void __fastcall tefirebullets_post_data_update(uintptr_t ent, uint32_t edx, uint32_t type)
{
	if (*reinterpret_cast<int32_t *>(ent + 12) + 1 == game->engine_client->get_local_player())
		return;

	hook_manager.tefirebullets_post_data_update->call(ent, edx, type);
}

NTSTATUS NTAPI nt_query_virtual_memory(HANDLE process_handle, PVOID base_address, int memory_information_class,
									   PMEMORY_BASIC_INFORMATION mem_information, ULONG length, PULONG result_length)
{
	const auto status = hook_manager.nt_query_virtual_memory->call(
		process_handle, base_address, memory_information_class, mem_information, length, result_length);
	if (!status && !memory_information_class &&
		reinterpret_cast<uintptr_t>(mem_information->AllocationBase) == game->base)
		mem_information->Type = MEM_IMAGE;
	return status;
}

void __cdecl enforce_competitive_cvar(const char *name, float min_value, float max_value, int32_t args, ...)
{
	// stub this.
}
} // namespace hooks::miscellaneous
