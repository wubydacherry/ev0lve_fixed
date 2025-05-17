
#ifndef SDK_WEAPON_H
#define SDK_WEAPON_H

#include <sdk/cutlvector.h>
#include <sdk/econ_item.h>
#include <sdk/entity.h>
#include <sdk/localize.h>
#include <sdk/math.h>
#include <sdk/weapon_system.h>

namespace sdk
{
class cs_player_t;

struct ref_counted
{
	uint32_t vtable;
	volatile long count;

	VIRTUAL(0, destruct(), int32_t(__thiscall *)(void *, bool))(true);
	VIRTUAL(1, release_internal(), int32_t(__thiscall *)(void *))();

	void release()
	{
		if (InterlockedDecrement(&count) == 0 && release_internal())
			destruct();
	}
};

inline void clear_ref_counted(cutlvector<ref_counted *> &vec)
{
	for (auto &el : vec)
		if (el)
		{
			el->release();
			el = nullptr;
		}

	vec.remove_all();
}

enum weapon_type
{
	weapontype_knife = 0,
	weapontype_pistol,
	weapontype_submachinegun,
	weapontype_rifle,
	weapontype_shotgun,
	weapontype_sniper_rifle,
	weapontype_machinegun,
	weapontype_c4,
	weapontype_grenade,
	weapontype_equipment,
	weapontype_stackableitem,
	weapontype_unknown
};

class econ_item_view
{
public:
	cutlvector<ref_counted *> &get_custom_materials()
	{
		return *reinterpret_cast<cutlvector<ref_counted *> *>(uint32_t(this) + 0x14); // correct
	}

	cutlvector<ref_counted *> &get_visual_data_processors()
	{
		return *reinterpret_cast<cutlvector<ref_counted *> *>(uint32_t(this) + 0x230); // correct. proof: "81 C7 30 02 00 00" "client.dll"
	}

	FUNC(game->client.at(offsets::weapon_csbase::fn_get_econ_static_data), get_static_data(),
		 econ_item_definition *(__thiscall *)(void *))
	();
};

class weapon_t : public entity_t
{
public:
	FUNC(game->client.at(offsets::weapon_csbase::fn_get_econ_item_view), get_econ_item_view(),
		 econ_item_view *(__thiscall *)(void *))
	();
	FUNC(game->client.at(offsets::weapon_csbase::fn_equip_glove), equip_glove(cs_player_t *player),
		 void(__thiscall *)(void *, cs_player_t *))
	(player);

	cutlvector<ref_counted*>& get_custom_materials()
	{
		const auto addr = server_offset( 0x9e8 ) - 0xC;
		return *reinterpret_cast< cutlvector<ref_counted*>* >( reinterpret_cast< uintptr_t >( this ) + addr );
	}

	bool& get_custom_materials_initialized()
	{
		return *reinterpret_cast< bool* >( reinterpret_cast< uint32_t >( this ) + server_offset( 0x3370 ) );
	}

	VAR(offsets::base_combat_weapon, clip1, int32_t);
	VAR(offsets::base_combat_weapon, next_primary_attack, float);
	VAR(offsets::base_combat_weapon, next_secondary_attack, float);
	VAR(offsets::base_combat_weapon, weapon_world_model, base_handle);
	VAR(offsets::base_combat_weapon, world_model_index, int32_t);
	VAR(offsets::base_combat_weapon, world_dropped_model_index, int32_t);
	VAR(offsets::base_combat_weapon, view_model_index, int32_t);

	VAR(offsets::weapon_csbase, postpone_fire_ready_time, float);
	VAR(offsets::weapon_csbase, weapon_mode, int32_t);
	VAR(offsets::weapon_csbase, accuracy_penalty, float);
	VAR(offsets::weapon_csbase, recoil_index, float);
	VAR(offsets::weapon_csbase, last_shot_time, float);
	VAR(offsets::weapon_csbase, iron_sight_controller, uintptr_t);
	VAR(offsets::weapon_csbase, last_client_fire_bullet_time, float);
	//VAR(offsets::weapon_csbase, custom_materials, cutlvector<ref_counted *>);
	//VAR(offsets::weapon_csbase, custom_materials_initialized, bool);

	VAR(offsets::weapon_csbase_gun, zoom_level, int32_t);

	VAR(offsets::script_created_item, item_definition_index, item_definition_index);
	VAR(offsets::script_created_item, entity_quality, int32_t);
	VAR(offsets::script_created_item, account_id, int32_t);
	VAR(offsets::script_created_item, item_idhigh, int32_t);
	VAR(offsets::script_created_item, initialized, bool);

	VAR(offsets::econ_entity, fallback_seed, int32_t);
	VAR(offsets::econ_entity, fallback_stat_trak, int32_t);
	VAR(offsets::econ_entity, fallback_paint_kit, int32_t);
	VAR(offsets::econ_entity, fallback_wear, float);
	VAR(offsets::econ_entity, original_owner_xuid_high, int32_t);
	VAR(offsets::econ_entity, original_owner_xuid_low, int32_t);

	VAR(offsets::base_grenade, thrower, base_handle);
	VAR(offsets::base_grenade, dmg_radius, float);

	VAR(offsets::base_csgrenade, pin_pulled, bool);
	VAR(offsets::base_csgrenade, throw_time, float);
	VAR(offsets::base_csgrenade, throw_strength, float);

	VAR(offsets::base_csgrenade_projectile, v_initial_velocity, vec3);
	VAR(offsets::base_csgrenade_projectile, bounces, int32_t);
	VAR(offsets::base_csgrenade_projectile, spawn_time, float);
	VAR(offsets::base_csgrenade_projectile, next_trail_line, float);
	VAR(offsets::base_csgrenade_projectile, last_trail_pos, vec3);

	VAR(offsets::molotov_projectile, is_inc_grenade, bool);
	VAR(offsets::smoke_grenade_projectile, smoke_effect_tick_begin, int32_t);

	VAR(offsets::base_weapon_world_model, combat_weapon_parent, base_handle);
	VAR(offsets::base_weapon_world_model, model_index, int32_t);

	MEMBER(offsets::base_combat_weapon::next_primary_attack + 0x6d, in_reload, bool);
	MEMBER(offsets::base_grenade::velocity, grenade_velocity, vec3);

	inline bool is_grenade()
	{
		return get_item_definition_index() >= item_definition_index::weapon_flashbang &&
			   get_item_definition_index() <= item_definition_index::weapon_incgrenade;
	}

	VIRTUAL(277, update_body_groups(void *owner, int state), bool(__thiscall *)(void *, void *, int))(owner, state);
	VIRTUAL(282, equip_wearable(void *player), void(__thiscall *)(void *, void *))(player);
	VIRTUAL(286, unequip_wearable(void *player), void(__thiscall *)(void *, void *))(player);
	VIRTUAL(442, get_max_speed(), float(__thiscall *)(void *))();
	VIRTUAL(445, get_zoom_in_sound(), const char *(__thiscall *)(void *))();
	VIRTUAL(446, get_zoom_out_sound(), const char *(__thiscall *)(void *))();
	VIRTUAL(453, get_spread(), float(__thiscall *)(void *))();
	VIRTUAL(455, get_weapon_type(), int32_t(__thiscall *)(void *))();
	VIRTUAL(461, get_cs_weapon_data(), cs_weapon_data *(__thiscall *)(void *))();
	VIRTUAL(483, get_inaccuracy(), float(__thiscall *)(void *))();
	VIRTUAL(484, update_accuracy(), void(__thiscall *)(void *))();

	inline float get_detonation_time()
	{
		switch (get_class_id())
		{
		case class_id::smoke_grenade_projectile:
			return TICKS_TO_TIME(get_smoke_effect_tick_begin());
		case class_id::inferno:
			return *reinterpret_cast<float *>(uint32_t(this) + 0x20);
		default:
			return 0.f;
		}
	}

	inline float get_expiry_time()
	{
		switch (get_class_id())
		{
		case class_id::smoke_grenade_projectile:
			return 17.5f;
		case class_id::inferno:
			return 7.f;
		default:
			return 0.f;
		}
	}

	inline std::wstring get_localized_name() { return game->localize->find_safe(get_cs_weapon_data()->szhudname); }

	inline const char *get_icon_name()
	{
		const auto item = get_econ_item_view();

		if (!item)
			RUNTIME_ERROR("Failure to get econ item view.");

		const auto data = item->get_static_data();

		if (!data)
			RUNTIME_ERROR("Failure to get econ item definition.");

		auto name = data->definition_name;
		const auto underscore = strchr(name, '_');
		if (underscore)
			name = underscore + 1;

		return name;
	}

	inline const char *get_nade_icon_name()
	{
		static const char *names[] = {"hegrenade", "molotov", "incgrenade", "smokegrenade", "flashbang", "tagrenade"};

		switch (get_class_id())
		{
		case class_id::molotov_projectile:
			return get_is_inc_grenade() ? names[2] : names[1];
		case class_id::smoke_grenade_projectile:
			return names[3];
		case class_id::sensor_grenade_projectile:
			return names[5];
		default:
			break;
		}

		const auto model = get_studio_hdr();

		if (model && model->hdr)
			return *reinterpret_cast<uint32_t *>(model->hdr->name + 13) == 0x67617266 ? names[0] : names[4];

		return names[0];
	}

	inline bool is_scoped_weapon()
	{
	//	printf("%i\n", get_item_definition_index());
		return get_item_definition_index() == item_definition_index::weapon_g3sg1 ||
			   get_item_definition_index() == item_definition_index::weapon_scar20 ||
			   get_item_definition_index() == item_definition_index::weapon_ssg08 ||
			   get_item_definition_index() == item_definition_index::weapon_awp ||
			   get_item_definition_index() == item_definition_index::weapon_sg556 ||
			   get_item_definition_index() == item_definition_index::weapon_aug;
	}

	inline bool is_pistol() { return get_weapon_type() == weapontype_pistol; }

	inline bool is_heavy_pistol()
	{
		return get_item_definition_index() == item_definition_index::weapon_deagle ||
			   get_item_definition_index() == item_definition_index::weapon_revolver;
	}

	inline bool is_shotgun() { return get_weapon_type() == weapontype_shotgun; }

	inline bool is_sniper()
	{
		return get_item_definition_index() == item_definition_index::weapon_g3sg1 ||
			   get_item_definition_index() == item_definition_index::weapon_scar20 ||
			   get_item_definition_index() == item_definition_index::weapon_ssg08 ||
			   get_item_definition_index() == item_definition_index::weapon_awp;
	}

	inline bool is_knife()
	{
		return get_item_definition_index() != item_definition_index::weapon_taser &&
			   get_weapon_type() == weapontype_knife;
	}

	inline bool is_automatic()
	{
		const bool is_smg = get_item_definition_index() == item_definition_index::weapon_bizon ||
							get_item_definition_index() == item_definition_index::weapon_mac10 ||
							get_item_definition_index() == item_definition_index::weapon_p90 ||
							get_item_definition_index() == item_definition_index::weapon_ump45 ||
							get_item_definition_index() == item_definition_index::weapon_mp7 ||
							get_item_definition_index() == item_definition_index::weapon_mp9 ||
							get_item_definition_index() == item_definition_index::weapon_mp5sd;

		const bool is_rifle = get_item_definition_index() == item_definition_index::weapon_ak47 ||
							  get_item_definition_index() == item_definition_index::weapon_aug ||
							  get_item_definition_index() == item_definition_index::weapon_m4a1 ||
							  get_item_definition_index() == item_definition_index::weapon_m4a1_silencer ||
							  get_item_definition_index() == item_definition_index::weapon_famas ||
							  get_item_definition_index() == item_definition_index::weapon_galilar ||
							  get_item_definition_index() == item_definition_index::weapon_sg556;

		const bool is_mg = get_item_definition_index() == item_definition_index::weapon_m249 ||
						   get_item_definition_index() == item_definition_index::weapon_negev;

		return is_smg || is_rifle || is_mg;
	}

	inline bool is_shootable_weapon()
	{
		const auto def_index = get_item_definition_index();
		return !is_grenade() && !is_knife() && def_index != item_definition_index::weapon_c4 &&
			   def_index != item_definition_index::weapon_healthshot;
	}
};
} // namespace sdk

#endif // SDK_WEAPON_H
