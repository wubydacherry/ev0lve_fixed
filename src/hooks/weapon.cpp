
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>

using namespace sdk;
using namespace detail;

namespace hooks::weapon
{
void __fastcall send_weapon_anim(weapon_t *weapon, uint32_t edx, uint32_t act)
{
	const auto owner = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity_from_handle(weapon->get_owner_entity()));
	if (owner && owner->is_player() && owner->is_valid() && owner->index() == game->engine_client->get_local_player())
	{
		auto &context = owner->get_command_context();
		auto &animation = pred.info[(context.cmd.command_number - 1) % input_max].animation;
		const auto fresh = game->input->verified_commands[context.cmd.command_number % input_max].crc ==
						   *reinterpret_cast<int32_t *>(&game->globals->interval_per_tick);
		if (animation.sequence == context.cmd.command_number - 1 &&
			(fresh || context.cmd.command_number <= game->client_state->last_command))
			animation.addon.vm = act;
		if (fresh && (act == 192 || act == 194 || act == 831 || act == 220 || act == 221))
			owner->get_is_looking_at_weapon() = owner->get_is_holding_look_at_weapon() = false;
	}

	hook_manager.send_weapon_anim->call(weapon, edx, act);
}

bool __fastcall deploy(weapon_t *weapon, uint32_t edx)
{
	const auto world_model = reinterpret_cast<entity_t *const>(
		game->client_entity_list->get_client_entity_from_handle(weapon->get_weapon_world_model()));
	if (world_model)
		world_model->get_effects() |= ef_nodraw;

	return hook_manager.deploy->call(weapon, edx);
}

int32_t __fastcall get_weapon_type(weapon_t *wpn, uint32_t edx)
{
	if (uintptr_t(_ReturnAddress()) == game->client.at(0x3FB031) && //return_to_update_post_processing
		cfg.local_visuals.fov_changer.get() && game->input->camera_in_third_person &&
		(wpn->get_item_definition_index() == item_definition_index::weapon_sg556 ||
		 wpn->get_item_definition_index() == item_definition_index::weapon_aug))
		return weapontype_sniper_rifle;

	return hook_manager.get_weapon_type->call(wpn, edx);
}
} // namespace hooks::weapon
