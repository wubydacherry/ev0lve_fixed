
#ifndef HACKS_SKINCHANGER_H
#define HACKS_SKINCHANGER_H

#include <base/cfg.h>
#include <ren/types/pos.h>
#include <sdk/cs_player.h>
#include <sdk/entity.h>
#include <sdk/mdl_cache.h>

#include <nlohmann/json.hpp>

namespace sdk
{
class game_event;
}

namespace hacks
{
struct viewmodel_arm_config
{
	const char *player_model_sub_string;
	const char *skintone_index;
	const char *glove_model;
	const char *sleeve_model;
	const char *sleeve_model_econ_override;
};

struct paint_kit_data_s
{
	int32_t paint_kit = -1;
	int32_t rarity{};
	std::string name{};
};

struct skin_data_s
{
	int32_t index = 0;
	paint_kit_data_s data{};
	int32_t seed{};
	bool dyn_stattrak{};
	int32_t stattrak = -1;
	int32_t old_entity_id = -1;
	float wear{};
};

struct weapon_skin_info
{
	int32_t index = 0;
	std::string name{};
	std::string item_name{};
	std::deque<paint_kit_data_s> paint_kits{};
};

struct skinchanger
{
	skinchanger() = default;

	void on_player_death(sdk::game_event *event);
	void on_post_data_update();
	void on_addon_bits(sdk::cs_player_t *player, int32_t &bits);
	void on_uninject();

	void clear_sequence_table() { original_sequences.clear(); }
	void update_sequence_table(sdk::weapon_t *wpn);
	void update_hud();

	int32_t fix_sequence(sdk::weapon_t *viewmodel, int32_t current_sequence);

	int32_t get_skin_equip_state(int32_t index, int32_t paint_kit);

	bool weapon_has_skin(int32_t index);
	skin_data_s &get_skin_data(int32_t index, int32_t team);
	void set_skin_data(int32_t index, int32_t team, skin_data_s skin);

	void equip(int32_t weapon, int32_t team, int32_t paint_kit, float wear, int32_t seed, int32_t stattrak,
			   bool dyn_stattrak);
	void unequip(int32_t weapon, int32_t team);

	void fill_weapon_list();
	void fill_skin_list(int id, std::string &search);

	void parse_game_items();
	void parse_weapon_names();

	static int32_t get_available_teams(int32_t index)
	{
		switch (index)
		{
		case int32_t(-2):
		case int32_t(-4):
		case int32_t(sdk::item_definition_index::weapon_glock):
		case int32_t(sdk::item_definition_index::weapon_ak47):
		case int32_t(sdk::item_definition_index::weapon_mac10):
		case int32_t(sdk::item_definition_index::weapon_g3sg1):
		case int32_t(sdk::item_definition_index::weapon_tec9):
		case int32_t(sdk::item_definition_index::weapon_galilar):
		case int32_t(sdk::item_definition_index::weapon_sg556):
		case int32_t(sdk::item_definition_index::weapon_sawedoff):
			return 2;
		case int32_t(-3):
		case int32_t(-5):
		case int32_t(sdk::item_definition_index::weapon_aug):
		case int32_t(sdk::item_definition_index::weapon_famas):
		case int32_t(sdk::item_definition_index::weapon_mag7):
		case int32_t(sdk::item_definition_index::weapon_fiveseven):
		case int32_t(sdk::item_definition_index::weapon_usp_silencer):
		case int32_t(sdk::item_definition_index::weapon_hkp2000):
		case int32_t(sdk::item_definition_index::weapon_mp9):
		case int32_t(sdk::item_definition_index::weapon_m4a1_silencer):
		case int32_t(sdk::item_definition_index::weapon_m4a1):
		case int32_t(sdk::item_definition_index::weapon_scar20):
			return 3;
		default:
			return 1;
		}
	}

	void load_skins();
	void save_skins();

	static void update_gui_components();
	static void update_gui_values();

	void remove_glove();

	struct hud_weapons_t
	{
		int32_t *get_weapon_count() { return reinterpret_cast<int32_t *>(uintptr_t(this) + 0x80); }
	};

	static void serialize_skin(nlohmann::json &j, const skin_data_s &skin);

	void handle_player_model();
	void handle_gloves(sdk::player_info &p);
	void handle_skins(sdk::player_info &p);
	void handle_viewmodel();

	const paint_kit_data_s &get_paint_kit_data(int32_t skin_id)
	{
		static const paint_kit_data_s null{};

		for (const auto &wpn : weapon_infos)
			for (const auto &data : wpn.second.paint_kits)
			{
				if (data.paint_kit == skin_id)
					return data;
			}

		return null;
	}
	void apply_skin(sdk::weapon_t *weapon, skin_data_s *skin, int owner_id);
	void apply_glove();

	sdk::weapon_t *create_wearable(int32_t entry, int32_t serial);
	int get_type_by_index(int32_t index)
	{
		// knives
		if (is_knife_by_index(index))
			return 1;
		// gloves
		else if (std::find(glove_def_indices.begin(), glove_def_indices.end(), index) != glove_def_indices.end())
			return 2;
		// player model
		else if (index < -1 && index >= -3)
			return 3;
		// facemask?
		else if (index < -3 && index >= -5)
			return 4;

		// normal skin
		return 0;
	}

	static bool is_knife_by_index(int32_t index)
	{
		switch (index)
		{
		case int32_t(sdk::item_definition_index::weapon_bayonet):
		case int32_t(sdk::item_definition_index::weapon_knife):
		case int32_t(sdk::item_definition_index::weapon_knife_butterfly):
		case int32_t(sdk::item_definition_index::weapon_knife_canis):
		case int32_t(sdk::item_definition_index::weapon_knife_cord):
		case int32_t(sdk::item_definition_index::weapon_knife_css):
		case int32_t(sdk::item_definition_index::weapon_knife_falchion):
		case int32_t(sdk::item_definition_index::weapon_knife_flip):
		case int32_t(sdk::item_definition_index::weapon_knife_ghost):
		case int32_t(sdk::item_definition_index::weapon_knife_gut):
		case int32_t(sdk::item_definition_index::weapon_knife_gypsy_jackknife):
		case int32_t(sdk::item_definition_index::weapon_knife_karambit):
		case int32_t(sdk::item_definition_index::weapon_knife_m9_bayonet):
		case int32_t(sdk::item_definition_index::weapon_knife_outdoor):
		case int32_t(sdk::item_definition_index::weapon_knife_push):
		case int32_t(sdk::item_definition_index::weapon_knife_skeleton):
		case int32_t(sdk::item_definition_index::weapon_knife_stiletto):
		case int32_t(sdk::item_definition_index::weapon_knife_survival_bowie):
		case int32_t(sdk::item_definition_index::weapon_knife_t):
		case int32_t(sdk::item_definition_index::weapon_knife_tactical):
		case int32_t(sdk::item_definition_index::weapon_knife_ursus):
		case int32_t(sdk::item_definition_index::weapon_knife_widowmaker):
			return true;
		}
		return false;
	}

	std::unordered_map<int32_t, skin_data_s> equipped_skins_ct{};
	std::unordered_map<int32_t, skin_data_s> equipped_skins_t{};

	skin_data_s equipped_knife_ct{};
	skin_data_s equipped_knife_t{};

	skin_data_s equipped_glove_ct{};
	skin_data_s equipped_glove_t{};

	skin_data_s equipped_model_ct{};
	skin_data_s equipped_model_t{};

	skin_data_s equipped_mask_ct{};
	skin_data_s equipped_mask_t{};

	sdk::base_handle glove_handle = -1;
	sdk::create_client_class create_fn{};

	skin_data_s current_skin{};

	std::vector<int32_t> glove_def_indices{};
	std::map<int32_t, weapon_skin_info> weapon_infos{};
	std::map<int32_t, std::tuple<std::string, std::string, std::string>> models{};

	int32_t previous_knife_index{}, previous_real_model{};
	std::unordered_map<int32_t, int32_t> original_sequences{};

	bool skins_changed{};
	bool should_update{}, should_update_hud{}, should_clear_glove{};

	std::array<evo::ren::color, 8> rarity_colors = {
		evo::ren::color(190, 195, 205), evo::ren::color(100, 150, 225), evo::ren::color(75, 105, 205),
		evo::ren::color(135, 70, 255),	evo::ren::color(210, 45, 230),	evo::ren::color(235, 75, 75),
		evo::ren::color(200, 170, 5),	evo::ren::color(135, 105, 10),
	};
};

extern std::shared_ptr<skinchanger> skin_changer;
} // namespace hacks

#endif // HACKS_SKINCHANGER_H
