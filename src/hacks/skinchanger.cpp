
#include <base/event_log.h>
#include <base/hook_manager.h>
#include <detail/custom_prediction.h>
#include <gui/controls.h>
#include <gui/gui.h>
#include <hacks/skinchanger.h>
#include <sdk/client_entity_list.h>
#include <sdk/cs_player.h>
#include <sdk/misc.h>
#include <util/misc.h>
#include <utility>

using namespace sdk;
using namespace detail;
using namespace evo;
using namespace gui;
using namespace ren;
using json = nlohmann::json;

namespace hacks
{
std::shared_ptr<skinchanger> skin_changer = std::make_shared<skinchanger>();

void skinchanger::on_player_death(sdk::game_event *event)
{
	// TODO: doesn't work if we inject in game. Hook game event handler instead.

	const auto attacker = game->engine_client->get_player_for_user_id(event->get_int(XOR_STR("attacker")));

	if (!game->local_player || attacker != game->local_player->index())
		return;

	const std::string event_weapon_name = event->get_string(XOR_STR("weapon"));

	int32_t event_weapon_index = 0;

	for (auto &weap : weapon_infos)
	{
		if (weap.second.item_name.find(event_weapon_name) != std::string::npos)
		{
			event_weapon_index = weap.first;
			break;
		}
	}

	auto weapon_skin = get_skin_data(event_weapon_index, game->local_player->get_team_num());

	if (weapon_skin.dyn_stattrak)
	{
		weapon_skin.stattrak++;
		set_skin_data(weapon_skin.index, game->local_player->get_team_num(), weapon_skin);
	}

	// not a knife, don't change icon
	if (event_weapon_name.find(XOR_STR("knife")) == std::string::npos &&
		event_weapon_name.find(XOR_STR("bayonet")) == std::string::npos)
		return;

	auto weapon_name = weapon_infos[weapon_skin.index].item_name;
	const auto it = weapon_name.find_first_of(XOR_STR("weapon_"));

	if (it != std::string::npos)
		weapon_name.erase(0, 7);

	event->set_string(XOR_STR("weapon"), weapon_name.c_str());
}

void skinchanger::on_post_data_update()
{
	player_info player_info;

	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	if (!player || !game->engine_client->get_player_info(player->index(), &player_info))
		return;

	game->mdl_cache->begin_lock();

	handle_gloves(player_info);
	handle_player_model();

	handle_skins(player_info);
	handle_viewmodel();

	game->mdl_cache->end_lock();
}

void skinchanger::on_addon_bits(cs_player_t *player, int32_t &bits)
{
	auto &paint = get_skin_data(-4, player->get_team_num()).data.paint_kit;
	if (paint != -1)
	{
		auto &model = models[paint];
		auto &current = *(const char **)game->client.at(globals::client_side_addons_mask);

		if (!FNV1A_CMP_IM(current, util::fnv1a(std::get<2>(model).c_str())) ||
			!game->client_entity_list->get_client_entity_from_handle(player->get_assassination_target_addon()))
		{
			game->string_table->precache_model(std::get<2>(model).c_str());
			current = std::get<2>(model).c_str();
			player->get_addon_bits() = bits;
			player->update_addon_models(true);
		}

		bits |= 0x10000;
	}
}

void skinchanger::on_uninject()
{
	// TODO: this, properly
	should_update_hud = true;
	update_hud();
}

void skinchanger::update_hud()
{
	return; // temp solution
	// no update needed
	if (!should_update_hud)
		return;

	auto hud_weapon_selection = find_hud_element(XOR_STR("CCSGO_HudWeaponSelection"));

	if (!hud_weapon_selection)
		return;

	const auto hud_weapons = reinterpret_cast<hud_weapons_t *>(hud_weapon_selection - 0xA0);

	if (!hud_weapons)
		return;

	if (!*hud_weapons->get_weapon_count())
		return;

	for (int32_t j = 0; j < *hud_weapons->get_weapon_count(); j++)
		j = reinterpret_cast<int32_t(__thiscall *)(void *, int32_t)>(game->client.at(functions::clear_hud_weapon_icon))(
			hud_weapons, j);

	should_update_hud = false;
}
void skinchanger::update_sequence_table(sdk::weapon_t *wpn)
{
	if (!original_sequences.empty())
		return;

	for (int32_t idx = 0; idx < wpn->get_studio_hdr()->get_numseq(); idx++)
		original_sequences[idx] = wpn->get_sequence_activity(idx);
}
int32_t skinchanger::fix_sequence(weapon_t *viewmodel, int32_t current_sequence)
{
	// no cached sequences, return original
	if (original_sequences.empty() || !viewmodel->get_studio_hdr())
		return current_sequence;

	// what act did the original knife trigger
	const auto wanted_act = original_sequences[current_sequence];
	std::optional<int32_t> min_sequence = std::nullopt, max_sequence = std::nullopt;

	// loop through all sequences.
	for (int32_t i = 0; i < viewmodel->get_studio_hdr()->get_numseq(); i++)
	{
		// get act for sequence
		const auto cur_act = viewmodel->get_sequence_activity(i);

		// wanted and current act are the same. second check is for lookat animations (default lookat has act -1)
		if ((wanted_act != -1 && cur_act == wanted_act) || (cur_act == XOR_32(213) && wanted_act == -1))
		{
			// adjust min sequence
			min_sequence = max_sequence = i;

			// find how many other sequences trigger the same animation
			for (int32_t j = 1; j < 4; j++)
			{
				// break if it doesn't
				if (cur_act != viewmodel->get_sequence_activity(i + j))
					break;

				// adjust max sequence
				max_sequence = i + j;
			}

			// stop
			break;
		}
	}

	// init ret value
	auto new_sequence = current_sequence;

	// we found a valid sequence
	if (min_sequence.has_value())
	{
		// we found multiple sequences, pick a random one
		// TODO: keep rare animations rare
		if (min_sequence.value() < max_sequence.value())
			new_sequence = random_int(min_sequence.value(), max_sequence.value());
		else
			new_sequence = min_sequence.value();
	}

	// return new_sequence
	return new_sequence;
}

bool skinchanger::weapon_has_skin(int32_t index)
{
	const auto skin_ct = get_skin_data(index, 3);
	const auto skin_t = get_skin_data(index, 2);

	return (skin_ct.index == index && skin_ct.data.paint_kit > 0) ||
		   (skin_t.index == index && skin_t.data.paint_kit > 0);
}
skin_data_s &skinchanger::get_skin_data(int32_t index, int32_t team)
{
	switch (get_type_by_index(index))
	{
	case 1:
		return (team == 2) ? equipped_knife_t : equipped_knife_ct;
	case 2:
		return (team == 2) ? equipped_glove_t : equipped_glove_ct;
	case 3:
		return (team == 2) ? equipped_model_t : equipped_model_ct;
	case 4:
		return (team == 2) ? equipped_mask_t : equipped_mask_ct;
	default:
		return (team == 2 ? equipped_skins_t : equipped_skins_ct)[index];
	}
}
void skinchanger::set_skin_data(int32_t index, int32_t team, skin_data_s skin)
{
	const auto &skin_old = get_skin_data(index, team);
	skin.old_entity_id = skin_old.old_entity_id;

	switch (get_type_by_index(index))
	{
	case 1:
		(team == 2 ? equipped_knife_t : equipped_knife_ct) = std::move(skin);
		break;
	case 2:
		(team == 2 ? equipped_glove_t : equipped_glove_ct) = std::move(skin);
		should_clear_glove = true;
		break;
	case 3:
		(team == 2 ? equipped_model_t : equipped_model_ct) = std::move(skin);
		break;
	case 4:
		(team == 2 ? equipped_mask_t : equipped_mask_ct) = std::move(skin);
		break;
	default:
		(team == 2 ? equipped_skins_t[index] : equipped_skins_ct[index]) = std::move(skin);
		break;
	}

	should_update = true;
	skins_changed = true;
}
int32_t skinchanger::get_skin_equip_state(int32_t index, int32_t paint_kit)
{
	skin_data_s skin_ct{};
	skin_data_s skin_t{};

	const auto type = get_type_by_index(index);

	switch (type)
	{
	case 1:
		skin_t = equipped_knife_t;
		skin_ct = equipped_knife_ct;
		break;
	case 2:
		skin_t = equipped_glove_t;
		skin_ct = equipped_glove_ct;
		break;
	case 3:
		skin_t = equipped_model_t;
		skin_ct = equipped_model_ct;
		break;
	case 4:
		skin_t = equipped_mask_t;
		skin_ct = equipped_mask_ct;
		break;
	default:
		skin_t = equipped_skins_t[index];
		skin_ct = equipped_skins_ct[index];
		break;
	}

	const auto equipped_ct = (skin_ct.index == index || type == 3 || type == 4) && skin_ct.data.paint_kit == paint_kit;
	const auto equipped_t = (skin_t.index == index || type == 3 || type == 4) && skin_t.data.paint_kit == paint_kit;

	if (equipped_ct && equipped_t)
		return 1;
	else if (equipped_ct)
		return 3;
	else if (equipped_t)
		return 2;

	return 0;
}

void skinchanger::equip(int32_t index, int32_t team, int32_t paint_kit, float wear, int32_t seed, int32_t stattrak,
						bool dyn_stattrak)
{
	current_skin.index = index;
	current_skin.data = get_paint_kit_data(paint_kit);
	current_skin.wear = wear;
	current_skin.seed = seed;
	current_skin.stattrak = stattrak;
	current_skin.dyn_stattrak = dyn_stattrak;

	if (team == 1)
	{
		set_skin_data(index, 2, current_skin);
		set_skin_data(index, 3, current_skin);
	}
	else
		set_skin_data(index, team, current_skin);

	fill_weapon_list();
}
void skinchanger::unequip(int32_t index, int32_t team)
{
	if (team == 1)
	{
		set_skin_data(index, 2, skin_data_s());
		set_skin_data(index, 3, skin_data_s());
	}
	else
		set_skin_data(index, team, skin_data_s());

	fill_weapon_list();
}

void skinchanger::parse_game_items()
{
	auto sys = *reinterpret_cast<econ_item_system **>(game->client.at(globals::econ_item_system));

	if (!sys)
		return;

	auto item_schema = sys->get_item_schema_interface();

	if (!item_schema)
		return;

	auto items = &item_schema->items_sorted;

	for (int iterator = 0; iterator < items->max_element(); ++iterator)
	{
		if (!items->is_valid_index(iterator))
			continue;

		auto elem = items->element(iterator);

		if (!elem || !elem->kv_item)
			continue;

		auto prefabKv = elem->kv_item->find_key(XOR_STR("prefab"));

		if (!prefabKv)
			continue;

		const auto prefabHash = util::fnv1a(static_cast<const char *>(prefabKv->get_ptr()));

		// NOTE: add knife, glove and player models into models
		// melee & weapon_melee_prefab = default knives, why does valve have 2 types of prefabs for the default
		// knives... melee_unusual = special knives hands_paintable = gloves customplayertradable = agents

		if (prefabHash != FNV1A("melee") && prefabHash != FNV1A("weapon_melee_prefab") &&
			prefabHash != FNV1A("melee_unusual") && prefabHash != FNV1A("hands_paintable") &&
			prefabHash != FNV1A("customplayer") && prefabHash != FNV1A("customplayertradable"))
			continue;

		// TODO: encrypt this stuff.

		std::string localized_name = util::utf8_encode(game->localize->find_safe(elem->item_base_name));

		if (prefabHash == FNV1A("hands_paintable"))
		{
			glove_def_indices.emplace_back(int32_t(elem->def_index));

			weapon_infos[int32_t(elem->def_index)].item_name = elem->item_base_name;
			weapon_infos[int32_t(elem->def_index)].name = localized_name;
		}

		if (prefabHash == FNV1A("customplayer") || prefabHash == FNV1A("customplayertradable"))
		{
			// ignore default skins but not dangerzone and bot skins.
			const auto def = uint16_t(elem->def_index);
			switch (def)
			{
			case 5043:
				localized_name = XOR_STR("Pirate 1");
				break;
			case 5044:
				localized_name = XOR_STR("Pirate 2");
				break;
			case 5045:
				localized_name = XOR_STR("Pirate 3");
				break;
			case 5046:
				localized_name = XOR_STR("Pirate 4");
				break;
			case 5047:
				localized_name = XOR_STR("Pirate 5");
				break;
			case 5093:
				localized_name = XOR_STR("Dangerzone 1");
				break;
			case 5094:
				localized_name = XOR_STR("Dangerzone 2");
				break;
			case 5095:
				localized_name = XOR_STR("Dangerzone 3");
				break;
			case 5096:
				localized_name = XOR_STR("Heavy Attacker");
				break;
			case 5097:
				localized_name = XOR_STR("Heavy Defender");
				break;
			}

			if (elem->item_rarity != 1 || (def >= 5043 && def <= 5047) || (def >= 5093 && def <= 5097))
			{
				if (std::string(elem->item_base_name).find(XOR("_t")) != std::string::npos ||
					(uint16_t(elem->def_index) >= 5093 && uint16_t(elem->def_index) <= 5096))
					weapon_infos[int32_t(-2)].paint_kits.emplace_back(
						paint_kit_data_s{int32_t(elem->def_index), elem->item_rarity, localized_name});
				else if (std::string(elem->item_base_name).find(XOR("_ct")) != std::string::npos ||
						 uint16_t(elem->def_index) == 5097)
					weapon_infos[int32_t(-3)].paint_kits.emplace_back(
						paint_kit_data_s{int32_t(elem->def_index), elem->item_rarity, localized_name});
			}
		}

		models[int32_t(elem->def_index)] =
			std::make_tuple(std::string(elem->definition_name), std::string(elem->base_display_model),
							elem->world_display_model ? std::string(elem->world_display_model) : std::string());
	}

	std::unordered_map<int32_t, std::vector<std::string>> knife_skins;

	auto *alternate_icon_data = &item_schema->alternate_icons;

	// parse knife skins
	for (int i = 0; i < alternate_icon_data->max_element(); ++i)
	{
		if (!alternate_icon_data->is_valid_index(i))
			continue;

		auto elem = &alternate_icon_data->element(i);

		if (!elem || !elem->simple_name.get() || !elem->simple_name.get()[0])
			continue;

		std::string name = elem->simple_name.get();

		const auto slash_pos = name.rfind('/');

		if (slash_pos != std::string::npos)
			name.erase(0, slash_pos + 1);

		// each skin exists 3 times (light, medium, heavy) in that list, we only need one
		if (name.find(XOR_STR("light")) == std::string::npos)
			continue;

		// erase "_light" from the end
		name.erase(name.length() - 6, 6);

		auto item_it = std::find_if(models.begin(), models.end(),
									[name](const auto &first)
									{
										if (first.first == size_t(item_definition_index::weapon_knife_t) ||
											first.first == size_t(item_definition_index::weapon_knife))
											return false;

										return strstr(name.data(), std::get<0>(first.second).data()) != nullptr;
									});

		if (item_it == models.end())
			continue;

		name.erase(0, std::get<0>(item_it->second).length() + 1);

		knife_skins[item_it->first].emplace_back(name);
	}

	std::unordered_map<uint32_t, std::pair<int32_t, std::string>> skin_ids;

	auto paint_kits = &item_schema->paint_kits;

	for (auto i = 0; i < paint_kits->max_element(); ++i)
	{
		if (!paint_kits->is_valid_index(i))
			continue;

		auto elem = paint_kits->element(i);

		if (!elem || !elem->name.get())
			continue;

		std::string skin_name = elem->name.get();
		std::string localized_name = util::utf8_encode(game->localize->find_safe(elem->description_tag.get()));

		if (skin_name.find(XOR_STR("ruby")) != std::string::npos)
			localized_name.append(XOR_STR(" Ruby"));

		if (skin_name.find(XOR_STR("sapphire")) != std::string::npos)
			localized_name.append(XOR_STR(" Sapphire"));

		if (skin_name.find(XOR_STR("emerald")) != std::string::npos)
			localized_name.append(XOR_STR(" Emerald"));

		if (skin_name.find(XOR_STR("blackpearl")) != std::string::npos)
			localized_name.append(XOR_STR(" Blackpearl"));

		const auto phasePos = skin_name.find(XOR_STR("phase"));

		// NOTE: this thing converts the ascii number to a number and back to a widestring to append it
		if (phasePos != std::string::npos)
			localized_name.append(
				XOR_STR(" Phase ") +
				std::to_string(std::strtol(skin_name.substr(phasePos + 5, phasePos + 6).data(), nullptr, 10)));

		skin_ids.emplace(std::pair<uint32_t, std::pair<int32_t, std::string>>(
			elem->id, std::pair<int32_t, std::string>(elem->rarity, localized_name)));
	}

	auto item_sets = &item_schema->item_sets;

	for (auto i = 0; i < item_sets->max_element(); ++i)
	{
		if (!item_sets->is_valid_index(i))
			continue;

		auto item_set = &item_sets->element(i);

		if (!item_set)
			continue;

		for (auto y = 0; y < item_set->item_entries.count(); ++y)
		{
			const auto &item = item_set->item_entries[y];

			if (item.item_def <= 0 || item.item_def > size_t(item_definition_index::studded_hydra_gloves) ||
				(item.item_def > size_t(item_definition_index::weapon_knife_skeleton) &&
				 item.item_def <= size_t(item_definition_index::studded_bloodhound_gloves)))
				continue;

			weapon_infos[int32_t(item.item_def)].paint_kits.emplace_back(
				paint_kit_data_s{item.paint_kit, skin_ids[item.paint_kit].first, skin_ids[item.paint_kit].second});
		}
	}

	// NOTE: apply fixes
	if (!skin_ids.empty())
	{
		// NOTE: howl fix
		weapon_infos[size_t(item_definition_index::weapon_m4a1)].paint_kits.emplace_back(
			paint_kit_data_s{309, 7, skin_ids[309].second});

		// NOTE: all of those listed weapons have fucked rarities
		std::vector<item_definition_index> items_to_fix = {
			item_definition_index::weapon_deagle,		 item_definition_index::weapon_glock,
			item_definition_index::weapon_ak47,			 item_definition_index::weapon_awp,
			item_definition_index::weapon_m4a1,			 item_definition_index::weapon_hkp2000,
			item_definition_index::weapon_m4a1_silencer, item_definition_index::weapon_usp_silencer};

		for (const auto &id : items_to_fix)
		{
			for (auto &skin : weapon_infos[size_t(id)].paint_kits)
			{
				if (skin.rarity > 0 && skin.rarity < 6)
					skin.rarity++;
			}
		}

		// NOTE: knife fix
		for (auto &wep : knife_skins)
		{
			// NOTE: also add vanilla knives
			if (wep.first <= int32_t(item_definition_index::weapon_knife_skeleton))
				weapon_infos[wep.first].paint_kits.emplace_back(paint_kit_data_s{0, 6, "-"});

			for (auto &skinName : wep.second)
			{
				for (auto i = 0; i < paint_kits->max_element(); ++i)
				{
					if (!paint_kits->is_valid_index(i))
						continue;

					auto elem = paint_kits->element(i);

					if (!elem || !elem->name.get())
						continue;

					if (skinName == elem->name.get())
					{
						weapon_infos[wep.first].paint_kits.emplace_back(
							paint_kit_data_s{elem->id, 6, skin_ids[elem->id].second});
						break;
					}
				}
			}
		}
	}

	models[int32_t(-2)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_hoxton.mdl"));
	models[int32_t(-3)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/porcelain_doll.mdl"));
	models[int32_t(-4)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_skull.mdl"));
	models[int32_t(-5)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_samurai.mdl"));
	models[int32_t(-6)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/evil_clown.mdl"));
	models[int32_t(-7)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_wolf.mdl"));
	models[int32_t(-8)] = std::make_tuple(std::string(), std::string(),
										  XOR_STR("models/player/holiday/facemasks/facemask_sheep_model.mdl"));
	models[int32_t(-9)] = std::make_tuple(std::string(), std::string(),
										  XOR_STR("models/player/holiday/facemasks/facemask_bunny_gold.mdl"));
	models[int32_t(-10)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_anaglyph.mdl"));
	models[int32_t(-11)] = std::make_tuple(
		std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_porcelain_doll_kabuki.mdl"));
	models[int32_t(-12)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_dallas.mdl"));
	models[int32_t(-13)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_pumpkin.mdl"));
	models[int32_t(-14)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_sheep_bloody.mdl"));
	models[int32_t(-15)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_devil_plastic.mdl"));
	models[int32_t(-16)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_boar.mdl"));
	models[int32_t(-17)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_chains.mdl"));
	models[int32_t(-18)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_tiki.mdl"));
	models[int32_t(-19)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_bunny.mdl"));
	models[int32_t(-20)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_sheep_gold.mdl"));
	models[int32_t(-21)] = std::make_tuple(
		std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_zombie_fortune_plastic.mdl"));
	models[int32_t(-22)] =
		std::make_tuple(std::string(), std::string(), XOR_STR("models/player/holiday/facemasks/facemask_chicken.mdl"));
	models[int32_t(-23)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_skull_gold.mdl"));
	models[int32_t(-24)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_demo_model.mdl"));
	models[int32_t(-25)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_engi_model.mdl"));
	models[int32_t(-26)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_heavy_model.mdl"));
	models[int32_t(-27)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_medic_model.mdl"));
	models[int32_t(-28)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_pyro_model.mdl"));
	models[int32_t(-29)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_scout_model.mdl"));
	models[int32_t(-30)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_sniper_model.mdl"));
	models[int32_t(-31)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_soldier_model.mdl"));
	models[int32_t(-32)] = std::make_tuple(std::string(), std::string(),
										   XOR_STR("models/player/holiday/facemasks/facemask_tf2_spy_model.mdl"));

	weapon_infos[int32_t(-4)].paint_kits = {
		{-2, 1, XOR_STR("Hoxton")},		   {-3, 1, XOR_STR("Porcelain Doll")},
		{-4, 1, XOR_STR("Skull")},		   {-5, 1, XOR_STR("Samurai")},
		{-6, 1, XOR_STR("Evil Clown")},	   {-7, 1, XOR_STR("Wolf")},
		{-8, 1, XOR_STR("Sheep")},		   {-9, 1, XOR_STR("Bunny Gold")},
		{-10, 1, XOR_STR("Anaglyph")},	   {-11, 1, XOR_STR("Porcelain Doll Kabuki")},
		{-12, 1, XOR_STR("Dallas")},	   {-13, 1, XOR_STR("Pumpkin")},
		{-14, 1, XOR_STR("Sheep Bloody")}, {-15, 1, XOR_STR("Devil Plastic")},
		{-16, 1, XOR_STR("Boar")},		   {-17, 1, XOR_STR("Chains")},
		{-18, 1, XOR_STR("Tiki")},		   {-19, 1, XOR_STR("Bunny")},
		{-20, 1, XOR_STR("Sheep Gold")},   {-21, 1, XOR_STR("Zombie Fortune Plastic")},
		{-22, 1, XOR_STR("Chicken")},	   {-23, 1, XOR_STR("Skull Gold")},
		{-24, 1, XOR_STR("TF2 Demoman")},  {-25, 1, XOR_STR("TF2 Engineer")},
		{-26, 1, XOR_STR("TF2 Heavy")},	   {-27, 1, XOR_STR("TF2 Medic")},
		{-28, 1, XOR_STR("TF2 Pyro")},	   {-29, 1, XOR_STR("TF2 Scout")},
		{-30, 1, XOR_STR("TF2 Sniper")},   {-31, 1, XOR_STR("TF2 Soldier")},
		{-32, 1, XOR_STR("TF2 Spy")}};
	weapon_infos[int32_t(-5)].paint_kits = weapon_infos[int32_t(-4)].paint_kits;
}
void skinchanger::parse_weapon_names()
{
	weapon_infos[int32_t(-5)].item_name = XOR("ct_face_mask");
	weapon_infos[int32_t(-5)].name = XOR("CT Mask");
	weapon_infos[int32_t(-4)].item_name = XOR("t_face_mask");
	weapon_infos[int32_t(-4)].name = XOR("T Mask");
	weapon_infos[int32_t(-3)].item_name = XOR("ct_player_model");
	weapon_infos[int32_t(-3)].name = XOR("CT Player Model");
	weapon_infos[int32_t(-2)].item_name = XOR("t_player_model");
	weapon_infos[int32_t(-2)].name = XOR("T Player Model");

	for (size_t id = 0; id <= int32_t(item_definition_index::weapon_knife_skeleton); id++)
	{
		if (weapon_infos.find(id) == weapon_infos.end())
			continue;

		const auto weapon_data = game->weapon_system->get_weapon_data(item_definition_index(id));

		if (!weapon_data)
			continue;

		std::wstring localized_name = game->localize->find_safe(weapon_data->szhudname);

		weapon_infos[id].item_name = weapon_data->consolename;

		if (!localized_name.empty())
			weapon_infos[id].name = util::utf8_encode(localized_name);
	}
}

void skinchanger::load_skins()
{
	const std::string filename = XOR_STR("ev0lve/skins");

	std::fstream i;
	i.open(filename, std::fstream::in | std::fstream::out);
	const auto root = json::parse(i, nullptr, false);
	i.close();

	if (!root.is_discarded())
	{
		json ct_skins = root[XOR_STR("CT")][XOR_STR("Skins")];

		for (auto &j_skin : ct_skins)
		{
			skin_data_s skin;

			if (j_skin.find(XOR_STR("weapon")) != j_skin.end())
				skin.index = j_skin[XOR_STR("weapon")];
			if (j_skin.find(XOR_STR("wear")) != j_skin.end())
				skin.wear = j_skin[XOR_STR("wear")];
			if (j_skin.find(XOR_STR("seed")) != j_skin.end())
				skin.seed = j_skin[XOR_STR("seed")];
			if (j_skin.find(XOR_STR("stattrak")) != j_skin.end())
				skin.stattrak = j_skin[XOR_STR("stattrak")];
			if (j_skin.find(XOR_STR("dyn_stattrak")) != j_skin.end())
				skin.dyn_stattrak = j_skin[XOR_STR("dyn_stattrak")];
			if (j_skin.find(XOR_STR("paint_kit")) != j_skin.end())
				skin.data = get_paint_kit_data(j_skin[XOR_STR("paint_kit")]);

			const auto type = get_type_by_index(skin.index);

			if (type == 1)
				equipped_knife_ct = skin;
			else if (type == 2)
				equipped_glove_ct = skin;
			else if (type == 3)
				equipped_model_ct = skin;
			else if (type == 4)
				equipped_mask_ct = skin;
			else
				equipped_skins_ct[skin.index] = skin;
		}

		json t_skins = root[XOR_STR("T")][XOR_STR("Skins")];

		for (auto &j_skin : t_skins)
		{
			skin_data_s skin;

			if (j_skin.find(XOR_STR("weapon")) != j_skin.end())
				skin.index = j_skin[XOR_STR("weapon")];
			if (j_skin.find(XOR_STR("wear")) != j_skin.end())
				skin.wear = j_skin[XOR_STR("wear")];
			if (j_skin.find(XOR_STR("seed")) != j_skin.end())
				skin.seed = j_skin[XOR_STR("seed")];
			if (j_skin.find(XOR_STR("stattrak")) != j_skin.end())
				skin.stattrak = j_skin[XOR_STR("stattrak")];
			if (j_skin.find(XOR_STR("dyn_stattrak")) != j_skin.end())
				skin.dyn_stattrak = j_skin[XOR_STR("dyn_stattrak")];
			if (j_skin.find(XOR_STR("paint_kit")) != j_skin.end())
				skin.data = get_paint_kit_data(j_skin[XOR_STR("paint_kit")]);

			const auto type = get_type_by_index(skin.index);

			if (type == 1)
				equipped_knife_t = skin;
			else if (type == 2)
				equipped_glove_t = skin;
			else if (type == 3)
				equipped_model_t = skin;
			else if (type == 4)
				equipped_mask_t = skin;
			else
				equipped_skins_t[skin.index] = skin;
		}
	}
}
void skinchanger::save_skins()
{
	if (!skins_changed)
		return;

	skins_changed = false;

	const std::string filename = XOR_STR("ev0lve/skins");

	json root;
	json ct_skins;
	json t_skins;

	auto knife_skin = equipped_knife_ct;

	if (knife_skin.index != 0)
	{
		json j_skin;
		serialize_skin(j_skin, knife_skin);
		ct_skins.push_back(j_skin);
	}

	auto glove_skin = equipped_glove_ct;

	if (glove_skin.index != 0)
	{
		json j_skin;
		serialize_skin(j_skin, glove_skin);
		ct_skins.push_back(j_skin);
	}

	auto agent_skin = equipped_model_ct;

	if (agent_skin.data.paint_kit != -1)
	{
		json j_skin;
		serialize_skin(j_skin, agent_skin);
		ct_skins.push_back(j_skin);
	}

	auto mask_skin = equipped_mask_ct;

	if (mask_skin.data.paint_kit != -1)
	{
		json j_skin;
		serialize_skin(j_skin, mask_skin);
		ct_skins.push_back(j_skin);
	}

	for (auto &skin : equipped_skins_ct)
	{
		if (skin.first == 0 || skin.second.data.paint_kit == 0)
			continue;

		json j_skin;
		serialize_skin(j_skin, skin.second);
		ct_skins.push_back(j_skin);
	}

	knife_skin = equipped_knife_t;

	if (knife_skin.index != 0)
	{
		json j_skin;
		serialize_skin(j_skin, knife_skin);
		t_skins.push_back(j_skin);
	}

	glove_skin = equipped_glove_t;

	if (glove_skin.index != 0)
	{
		json j_skin;
		serialize_skin(j_skin, glove_skin);
		t_skins.push_back(j_skin);
	}

	agent_skin = equipped_model_t;

	if (agent_skin.data.paint_kit != -1)
	{
		json j_skin;
		serialize_skin(j_skin, agent_skin);
		t_skins.push_back(j_skin);
	}

	mask_skin = equipped_mask_t;

	if (mask_skin.data.paint_kit != -1)
	{
		json j_skin;
		serialize_skin(j_skin, mask_skin);
		t_skins.push_back(j_skin);
	}

	for (auto &skin : equipped_skins_t)
	{
		if (skin.first == 0 || skin.second.data.paint_kit == 0)
			continue;

		json j_skin;
		serialize_skin(j_skin, skin.second);
		t_skins.push_back(j_skin);
	}

	root[XOR_STR("CT")][XOR_STR("Skins")] = ct_skins;
	root[XOR_STR("T")][XOR_STR("Skins")] = t_skins;

	std::ofstream o(filename);
	o << std::setw(4) << root << std::endl;
	o.close();
}

void skinchanger::serialize_skin(json &j, const skin_data_s &skin)
{
	j[XOR_STR("weapon")] = skin.index;
	j[XOR_STR("paint_kit")] = skin.data.paint_kit;
	j[XOR_STR("wear")] = skin.wear;
	j[XOR_STR("seed")] = skin.seed;
	j[XOR_STR("stattrak")] = skin.stattrak;
	j[XOR_STR("dyn_stattrak")] = skin.dyn_stattrak;
}

void skinchanger::fill_weapon_list()
{
	static auto last_item_type = 0;

	const int item_type = cfg.skins.selected_type.get().first_set_bit().value_or(0);
	const auto last_selection = cfg.skins.selected_entity.get();

	const auto settings_content = ctx->find<layout>(GUI_HASH("skins.settings_content"));

	if (settings_content)
		settings_content->set_visible(false);

	const auto weapon_list = ctx->find<combo_box>(GUI_HASH("skins.select.weapon_list"));
	if (!weapon_list)
		return;
	weapon_list->remove_all();

	auto first_index = 0;

	for (const auto &wpn : weapon_infos)
	{
		auto type = get_type_by_index(wpn.first);

		if (type != item_type)
			continue;

		const auto sel =
			std::make_shared<selectable>(evo::gui::control_id{gui::hash(wpn.second.item_name), ""}, wpn.second.name);
		sel->value = wpn.first;
		if (weapon_has_skin(wpn.first))
			sel->is_loaded = true;

		weapon_list->add(sel);

		if (!first_index)
			first_index = wpn.first;
	}

	// TODO: nicer?
	if (last_item_type != item_type || last_selection == 0)
	{
		cfg.skins.selected_entity = first_index;
		last_item_type = item_type;
	}
	else
		cfg.skins.selected_entity = last_selection;

	const auto skin_list = ctx->find<list>(GUI_HASH("skins.select.skin_list"));

	if (!skin_list)
		return;

	// fix shit
	weapon_list->process_queues();

	const auto scroll_bak = skin_list->get_scroll();
	weapon_list->callback(cfg.skins.selected_entity.get());
	skin_list->set_scroll(scroll_bak);
}
void skinchanger::fill_skin_list(int id, std::string &search)
{
	static const auto find_case_insensitive = [](std::string data, std::string search, size_t pos = 0)
	{
		std::transform(data.begin(), data.end(), data.begin(), ::tolower);
		std::transform(search.begin(), search.end(), search.begin(), ::tolower);

		return data.find(search, pos);
	};
	static uint64_t last_selected_entity = 0;

	const auto skin_list = ctx->find<list>(GUI_HASH("skins.select.skin_list"));

	if (!skin_list)
		return;

	// backup old selected value for user experience
	const auto selected_skin = skin_list->value.get();

	// backup scroll
	const auto scroll_bak = skin_list->get_scroll();

	// clear gui element
	skin_list->remove_all();

	// fix shit
	skin_list->process_queues();

	// init temp skin list for sorting
	auto skins_to_add = std::vector<std::shared_ptr<selectable>>();

	// loop all entities with skins
	for (auto &skins : weapon_infos)
	{
		// skip skins not meant for the selected entity
		if (cfg.skins.filter_by_item.get() && skins.first != id)
			continue;

		// skip skins not meant for the selected entity type (always on)
		if (get_type_by_index(skins.first) != get_type_by_index(id))
			continue;

		// loop all paint kits for entity
		for (auto &skin : skins.second.paint_kits)
		{
			// skip fucked up ones
			if (skin.name.empty())
				continue;

			// some skins are used on multiple occasions
			auto skin_not_in_list = std::find_if(skins_to_add.begin(), skins_to_add.end(),
												 [&](std::shared_ptr<selectable> &s)
												 { return s->value == skin.paint_kit; }) == skins_to_add.end();

			if (!skin_not_in_list)
				continue;

			// create selectable and set color by rarity
			const auto sel = std::make_shared<selectable>(
				evo::gui::control_id{gui::hash(skin.name + std::to_string(skin.rarity) +
											   std::to_string(skin.paint_kit) + skins.second.item_name)},
				skin.name, rarity_colors[skin.rarity == 7 ? skin.rarity : skin.rarity - 1]);

			// set internals
			sel->value = skin.paint_kit;
			sel->order = skin.rarity;

			std::string text_addition{};

			const auto skin_ct = get_skin_data(id, 3);
			const auto skin_t = get_skin_data(id, 2);

			// TODO: make nicer
			if (skin_ct.data.paint_kit == skin.paint_kit && skin_ct.index == id)
				text_addition += XOR(" (CT)");

			if (skin_t.data.paint_kit == skin.paint_kit && skin_t.index == id)
				text_addition += XOR(" (T)");

			if (!text_addition.empty())
			{
				sel->text += text_addition;
				sel->is_highlighted = true;
			}

			// skip existing skins
			if (skin_not_in_list && (search.empty() || find_case_insensitive(sel->text, search) != std::string::npos))
				skins_to_add.emplace_back(sel);
		}
	}

	// sort by order (rarity) and alphabetically
	std::sort(skins_to_add.begin(), skins_to_add.end(),
			  [](const std::shared_ptr<selectable> &l, const std::shared_ptr<selectable> &r)
			  {
				  if (l->order < r->order)
					  return true;
				  if (l->order > r->order)
					  return false;

				  if (l->text < r->text)
					  return true;
				  return false;
			  });

	if (skins_to_add.empty())
		return;

	auto select = skins_to_add.front();

	// loop sorted array
	for (auto &sel : skins_to_add)
	{
		// add skins to list
		skin_list->add(sel);

		// find the item we want to select based on the saved selection
		if (sel->value == selected_skin)
			select = sel;
	}

	// fix shit
	skin_list->process_queues();

	// entity changed
	if (last_selected_entity != cfg.skins.selected_entity.get())
	{
		// reset skin list (scroll and selections)
		skin_list->reset();

		// update selection
		last_selected_entity = cfg.skins.selected_entity.get();
	}

	// select entry
	if (select)
	{
		select->select();
		skin_list->update_selected(select);
		skin_list->value = select->value;
	}

	// reset scroll if we're not in search
	if (search == "")
		skin_list->set_scroll(scroll_bak);
}

sdk::weapon_t *skinchanger::create_wearable(int32_t entry, int32_t serial)
{
	if (!create_fn)
	{
		for (auto data = game->hl_client->get_all_classes(); data; data = data->next)
		{
			if (FNV1A_CMP(data->network_name, "CEconWearable"))
				create_fn = data->create_fn;
		}
	}

	create_fn(entry, serial);
	return reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity(entry));
}

void skinchanger::apply_skin(weapon_t *weapon, skin_data_s *skin, const int owner_id)
{
	// no skin set
	if (skin->data.paint_kit < 0)
	{
		// we reset this one already, skip
		if (weapon->get_item_idhigh() == skin->old_entity_id || skin->old_entity_id < 0)
			return;

		// reset
		game->client_state->request_full_update();

		should_update_hud = true;
		update_hud();

		return;
	}
	else if (skin->old_entity_id < 0)
		skin->old_entity_id = weapon->get_item_idhigh();

	const auto type = get_type_by_index(skin->index);

	// check for changes
	const auto paint_kit_changed = weapon->get_fallback_paint_kit() != skin->data.paint_kit;
	const auto seed_changed = weapon->get_fallback_seed() != skin->seed;
	const auto wear_changed = weapon->get_fallback_wear() != max(skin->wear, .001f);
	const auto stattrak_changed = weapon->get_fallback_stat_trak() != skin->stattrak;
	const auto stattrak_enable_changed =
		(skin->stattrak == -1 || weapon->get_fallback_stat_trak() == -1) && stattrak_changed;

	should_update = should_update || (paint_kit_changed || seed_changed || wear_changed || stattrak_changed);
	should_update_hud = should_update_hud || paint_kit_changed || stattrak_enable_changed;

	if (should_update)
	{
		// force fallback values
		weapon->get_initialized() = true;
		weapon->get_item_idhigh() = -1;

		// apply skin
		weapon->get_fallback_seed() = skin->seed;
		weapon->get_fallback_paint_kit() = skin->data.paint_kit;
		weapon->get_fallback_wear() = max(skin->wear, 0.001f);
		weapon->get_fallback_stat_trak() = skin->stattrak;

		// fix knife quality if needed
		if (type == 1)
			weapon->get_entity_quality() = 3;

		// fix owner
		weapon->get_account_id() = owner_id;

		// weapon id of skin doesn't match actual weapon id
		if (skin->index != size_t(weapon->get_item_definition_index()))
		{
			// update definition index
			weapon->get_item_definition_index() = item_definition_index(skin->index);

			const auto model = game->model_info_client->get_model_index(std::get<1>(models[skin->index]).c_str());

			// update model
			weapon->set_model_index(model);

			// apply
			weapon->pre_data_update(0);
		}

		// force materials to update
		if (paint_kit_changed)
		{
			weapon->get_custom_materials_initialized() = skin->data.paint_kit <= 0;
			clear_ref_counted(weapon->get_custom_materials());
			clear_ref_counted(weapon->get_econ_item_view()->get_custom_materials());
			clear_ref_counted(weapon->get_econ_item_view()->get_visual_data_processors());
		}
		weapon->post_data_update(0);
		weapon->on_data_changed(0);

		// mark update done
		should_update = false;
	}

	// update hud weapons
	update_hud();
}

void skinchanger::apply_glove()
{
	if (should_clear_glove)
		return;

	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	if (!player)
		return;

	const auto glove = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_my_wearables()[0]));

	if (!glove)
		return;

	// get potential glove skin for team (definition index can be any glove)
	const auto skin = &get_skin_data(size_t(item_definition_index::studded_bloodhound_gloves), player->get_team_num());

	// no skin, stop
	if (skin->data.paint_kit <= 0)
	{
		should_clear_glove = true;
		return;
	}

	glove->get_initialized() = true;
	glove->get_item_idhigh() = -1;

	glove->get_fallback_paint_kit() = skin->data.paint_kit;
	glove->get_fallback_seed() = skin->seed;
	glove->get_fallback_stat_trak() = skin->stattrak;
	glove->get_fallback_wear() = max(skin->wear, .001f);

	player->get_animating_body() = 1;

	// weapon id of skin doesn't match actual weapon id
	if (skin->index != size_t(glove->get_item_definition_index()))
	{
		// update definition index
		glove->get_item_definition_index() = item_definition_index(skin->index);

		const auto model = game->model_info_client->get_model_index(std::get<2>(models[skin->index]).c_str());

		// update model
		glove->set_model_index(model);

		// make gloves visible
		glove->get_effects() &= ~ef_nodraw;

		// apply
		glove->pre_data_update(0);
		glove->post_data_update(0);
		glove->on_data_changed(0);

		// equip econ wearable
		glove->equip_glove(player);

		const auto viewmodel_entity = reinterpret_cast<entity_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_view_model_0()));
		if (!viewmodel_entity)
			return;

		player->remove_viewmodel_arm_models();
		viewmodel_entity->update_viewmodel_addons();
	}
}
void skinchanger::remove_glove()
{
	if (!should_clear_glove)
		return;

	should_clear_glove = false;

	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));
	const auto our_glove =
		reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(glove_handle));

	if (!our_glove)
		return;

	our_glove->get_effects() |= ef_nodraw;
	our_glove->on_data_changed(0);

	if (player)
	{
		player->get_animating_body() = 0;
		our_glove->unequip_wearable(player);
		glove_handle = player->get_my_wearables()[0] = -1;

		const auto viewmodel_entity = reinterpret_cast<entity_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_view_model_0()));
		if (viewmodel_entity)
		{
			player->remove_viewmodel_arm_models();
			viewmodel_entity->update_viewmodel_addons();
		}
		game->leaf_system->remove_renderable(our_glove->render_handle());
		our_glove->set_destroyed_on_recreate_entities();
		our_glove->release();
	}
}

void skinchanger::handle_player_model()
{
	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	if (!player->get_studio_hdr())
	{
		previous_real_model = detail::pred.real_model;
		return;
	}

	// make sure we give the game enough time to setup bone flags.
	uint32_t total_flags{};
	for (auto i = 0; i < player->get_studio_hdr()->numbones(); i++)
		total_flags |= player->get_studio_hdr()->bone_flags[i];
	if (!(total_flags & bone_used_by_bone_merge) || !(total_flags & bone_always_setup))
		return;

	auto &target_model =
		player->get_team_num() == 2 ? equipped_model_t.data.paint_kit : equipped_model_ct.data.paint_kit;
	auto model = player->get_model_index();

	// update model in prediction if needed
	if (detail::pred.real_model <= 0)
		detail::pred.real_model = model;

	if (previous_real_model != detail::pred.real_model)
	{
		player->set_model_index(detail::pred.real_model);
		previous_real_model = detail::pred.real_model;
		return;
	}

	// no model selected
	if (target_model == -1)
	{
		// reset if needed
		if (model != detail::pred.real_model)
			player->set_model_index(detail::pred.real_model);
	}
	else if (target_model > 0)
	{
		// get model index from model name
		const auto &model_name = std::get<1>(models[target_model]);
		model = game->model_info_client->get_model_index(model_name.c_str());

		// model not found
		if (model <= 0)
		{
			// precache the viewmodel first
			const auto arm_cfg = ((viewmodel_arm_config * (__thiscall *)(const char *)) game->client.at(
				functions::get_player_viewmodel_arm_config_for_player_model))(model_name.c_str());
			if (arm_cfg)
			{
				game->string_table->precache_model(arm_cfg->glove_model);
				game->string_table->precache_model(arm_cfg->sleeve_model);
			}

			// precache and read again
			game->string_table->precache_model(model_name.c_str());
			model = game->model_info_client->get_model_index(model_name.c_str());
		}
	}

	// handle when dead
	if (!player->is_alive())
	{
		const auto ragdoll = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_ragdoll()));
		if (ragdoll && ragdoll->get_model_index() != model && model > 0 && target_model >= 0)
			ragdoll->set_model_index(model);
	}

	// set model index if needed
	if (player->get_model_index() != model && model > 0 && target_model >= 0)
	{
#if 0
		if (player->get_model_index() != detail::pred.real_model)
			player->set_model_index(detail::pred.real_model);

		if (detail::pred.real_hdr /* TODO */)
		{
			const auto mdl = player->get_studio_hdr();
			const auto handle = player->get_hdr_handle();
			player->get_studio_hdr() = detail::pred.real_hdr;
			player->get_hdr_handle() = detail::pred.real_handle;
			hook_manager.invalidate_mdl_cache->call(player, 0);
			player->get_hdr_handle() = handle;
			player->get_studio_hdr() = mdl;
		}
#endif
		player->set_model_index(model);
		player->set_model_index(detail::pred.real_model);

		detail::pred.real_hdr = player->get_studio_hdr();
		detail::pred.real_handle = player->get_hdr_handle();
		player->get_flags() |= (1 << 15);
		player->set_model_index(model);
		player->get_flags() &= ~(1 << 15);
	}
}

void skinchanger::handle_gloves(sdk::player_info &p)
{
	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	// get wearables and glove
	auto &wearable = player->get_my_wearables()[0];
	auto glove = reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(wearable));
	const auto our_glove =
		reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(glove_handle));
	const auto skin = get_skin_data(size_t(item_definition_index::studded_bloodhound_gloves), player->get_team_num());

	// player is dead or no skin selected
	if (!player->is_alive() || skin.data.paint_kit <= 0)
	{
		// hide gloves if exist
		if (our_glove)
			our_glove->get_effects() |= ef_nodraw;

		// mark glove to be cleared
		should_clear_glove = true;
	}

	// set gloves aren't ours
	if (glove != our_glove && our_glove)
	{
		// clear our glove if gloves exist
		if (glove)
			should_clear_glove = true;
		// set our gloves if no gloves are set
		else
		{
			wearable = glove_handle;
			glove = our_glove;
		}
	}

	// no glove exists but should
	if (!glove && !should_clear_glove)
	{
		// get next entity index and random serial
		const auto entry = game->client_entity_list->get_highest_entity_index() + 1;
		const auto serial = rand() % 0x1000;
		glove = create_wearable(entry, serial);
		glove_handle = wearable = entry | serial << 16;
		game->leaf_system->create_renderable_handle(glove);
	}

	apply_glove();

	if (glove)
	{
		game->client_entity_list->for_each(
			[&](entity_t *ent)
			{
				if (ent->get_class_id() != class_id::econ_wearable || ent == glove)
					return;

				if (reinterpret_cast<weapon_t *>(ent)->get_account_id() == p.xuid_low)
				{
					game->leaf_system->remove_renderable(ent->render_handle());
					ent->set_destroyed_on_recreate_entities();
					ent->release();
				}
			});
	}

	remove_glove();
}

void skinchanger::handle_skins(sdk::player_info &p)
{
	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	for (const auto &handle : player->get_my_weapons())
	{
		// skip invalid handles
		if (handle == -1)
			continue;

		// get weapon_t from handle
		const auto weapon =
			reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(handle));

		// not a weapon_t, skip
		if (!weapon || !weapon->is_base_combat_weapon())
			continue;

		const auto definition_index = size_t(weapon->get_item_definition_index());

		// still not a weapon_t, skip
		if (definition_index == 0)
			continue;

		// check for ownership
		const auto our_weapon = weapon->get_original_owner_xuid_low() == (p.steam_id64 & 0xffffffff) &&
								weapon->get_original_owner_xuid_high() == (p.steam_id64 >> 32 & 0xffffffff);

		// not our gun, skip
		if (!our_weapon)
			continue;

		const auto skin = &get_skin_data(definition_index, player->get_team_num());
		apply_skin(weapon, skin, p.xuid_low);
	}
}

void skinchanger::handle_viewmodel()
{
	const auto player = reinterpret_cast<cs_player_t *>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	// get viewmodel
	const auto viewmodel_entity = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_view_model_0()));
	if (!viewmodel_entity)
		return;

	// get viewmodel weapon if it's a knife
	const auto weapon_entity = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(viewmodel_entity->get_viewmodel_weapon()));
	if (!weapon_entity || !weapon_entity->is_knife())
		return;

	update_sequence_table(viewmodel_entity);

	// update model index
	const auto view_model_index = game->model_info_client->get_model_index(
		std::get<1>(models[size_t(weapon_entity->get_item_definition_index())]).c_str());
	const auto world_model_index = game->model_info_client->get_model_index(
		std::get<2>(models[size_t(weapon_entity->get_item_definition_index())]).c_str());
	if (previous_knife_index != view_model_index)
	{
		should_update = should_update_hud = true;
		previous_knife_index = view_model_index;
	}

	viewmodel_entity->set_model_index(view_model_index);

	if (weapon_entity->get_model_index() != view_model_index)
		weapon_entity->set_model_index(view_model_index);

	// update world model
	auto weapon_world_entity = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(weapon_entity->get_weapon_world_model()));
	if (weapon_world_entity && weapon_world_entity->get_model_index() != world_model_index)
		weapon_world_entity->set_model_index(world_model_index);
}

void skinchanger::update_gui_components()
{
	bool visibility_changed = false;

	const auto weapon = static_cast<int32_t>(cfg.skins.selected_entity.get());
	const auto paint_kit = static_cast<int32_t>(cfg.skins.selected_skin.get());

	const auto equipped_to = hacks::skin_changer->get_skin_equip_state(weapon, paint_kit);
	const auto available_teams = hacks::skin_changer->get_available_teams(weapon);

	const auto equip_ct = ctx->find<button>(GUI_HASH("skins.settings.equip_ct"));
	const auto equip_t = ctx->find<button>(GUI_HASH("skins.settings.equip_t"));
	const auto equip_both = ctx->find<button>(GUI_HASH("skins.settings.equip_both"));

	if (!equip_ct || !equip_t || !equip_both)
		return;

	equip_ct->set_visible(available_teams == 1 || available_teams == 3 || !cfg.skins.filter_by_team.get());
	equip_t->set_visible(available_teams == 1 || available_teams == 2 || !cfg.skins.filter_by_team.get());
	equip_both->set_visible(available_teams == 1 || !cfg.skins.filter_by_team.get());

	equip_both->text = (equipped_to == 1) ? XOR("Unequip Both") : XOR("Equip Both");
	equip_ct->text = (equipped_to == 1 || equipped_to == 3) ? XOR("Unequip CT") : XOR("Equip CT");
	equip_t->text = (equipped_to == 1 || equipped_to == 2) ? XOR("Unequip T") : XOR("Equip T");

	const auto stattrak_type = cfg.skins.stattrak_type.get().first_set_bit().value_or(-1);
	const auto custom_seed = cfg.skins.custom_seed.get();
	const auto selected_type = cfg.skins.selected_type.get();

	const auto custom_seed_checkbox = ctx->find<checkbox>(GUI_HASH("skins.settings.custom_seed"));
	if (custom_seed_checkbox)
	{
		const auto callback = custom_seed_checkbox->callback;
		custom_seed_checkbox->callback = {};
		custom_seed_checkbox->reset();
		custom_seed_checkbox->callback = callback;
	}

	if (ctx->find<layout>(GUI_HASH("skins.settings.stattrak_content")))
		ctx->find<layout>(GUI_HASH("skins.settings.stattrak_content"))
			->set_visible(stattrak_type != 0 && selected_type <= 3);

	if (ctx->find<layout>(GUI_HASH("skins.settings.seed_content")))
		ctx->find<layout>(GUI_HASH("skins.settings.seed_content"))->set_visible(custom_seed && selected_type <= 4);

	if (ctx->find<layout>(GUI_HASH("Wear_content")))
		ctx->find<layout>(GUI_HASH("Wear_content"))->set_visible(selected_type <= 4);

	if (ctx->find<layout>(GUI_HASH("Custom seed_content")))
		ctx->find<layout>(GUI_HASH("Custom seed_content"))->set_visible(selected_type <= 4);

	if (ctx->find<layout>(GUI_HASH("StatTrak_content")))
		ctx->find<layout>(GUI_HASH("StatTrak_content"))->set_visible(selected_type <= 3);
}

void skinchanger::update_gui_values()
{
	const auto weapon = static_cast<int32_t>(cfg.skins.selected_entity.get());
	const auto paint_kit = static_cast<int32_t>(cfg.skins.selected_skin.get());

	const auto equipped_to = hacks::skin_changer->get_skin_equip_state(weapon, paint_kit);
	const auto equipped_skin = hacks::skin_changer->get_skin_data(weapon, equipped_to == 1 ? 3 : equipped_to);

	const auto stattrak_text = ctx->find<text_input>(GUI_HASH("skins.settings.stattrak"));
	const auto seed_text = ctx->find<text_input>(GUI_HASH("skins.settings.seed"));

	if (!stattrak_text || !seed_text)
		return;

	if (!equipped_to || weapon < 0)
	{
		cfg.skins.wear = 0.f;
		cfg.skins.custom_seed = false;
		cfg.skins.stattrak_type.set(1);
		stattrak_text->set_value("");
		seed_text->set_value("");
		return;
	}

	cfg.skins.wear = equipped_skin.wear;
	cfg.skins.custom_seed = equipped_skin.seed != 1;

	if (cfg.skins.custom_seed)
		seed_text->set_value(std::to_string(equipped_skin.seed));

	cfg.skins.stattrak_type.set(0);
	cfg.skins.stattrak_type.get().set(static_cast<char>((equipped_skin.stattrak >= 0) + equipped_skin.dyn_stattrak));

	if (equipped_skin.stattrak >= 0 || equipped_skin.dyn_stattrak)
		stattrak_text->set_value(std::to_string(equipped_skin.stattrak));
}
} // namespace hacks
