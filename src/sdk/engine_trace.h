
#ifndef SDK_ENGINE_TRACE_H
#define SDK_ENGINE_TRACE_H

#include <sdk/client_entity_list.h>
#include <sdk/cs_player.h>

#define contents_empty 0
#define contents_solid 0x1
#define contents_window 0x2
#define contents_aux 0x4
#define contents_grate 0x8
#define contents_slime 0x10
#define contents_water 0x20
#define contents_blocklos 0x40
#define contents_opaque 0x80
#define last_visible_contents contents_opaque
#define all_visible_contents (last_visible_contents | (last_visible_contents - 1))
#define contents_testfogvolume 0x100
#define contents_unused 0x200
#define contents_blocklight 0x400
#define contents_team1 0x800
#define contents_team2 0x1000
#define contents_ignore_nodraw_opaque 0x2000
#define contents_moveable 0x4000
#define contents_areaportal 0x8000
#define contents_playerclip 0x10000
#define contents_monsterclip 0x20000
#define contents_current_0 0x40000
#define contents_current_90 0x80000
#define contents_current_180 0x100000
#define contents_current_270 0x200000
#define contents_current_up 0x400000
#define contents_current_down 0x800000
#define contents_origin 0x1000000
#define contents_monster 0x2000000
#define contents_debris 0x4000000
#define contents_detail 0x8000000
#define contents_translucent 0x10000000
#define contents_ladder 0x20000000
#define contents_hitbox 0x40000000

#define mask_all 0xffffffff
#define mask_solid (contents_solid | contents_moveable | contents_window | contents_monster | contents_grate)
#define mask_playersolid                                                                                               \
	(contents_solid | contents_moveable | contents_playerclip | contents_window | contents_monster | contents_grate)
#define mask_npcsolid                                                                                                  \
	(contents_solid | contents_moveable | contents_monsterclip | contents_window | contents_monster | contents_grate)
#define mask_npcfluid (contents_solid | contents_moveable | contents_monsterclip | contents_window | contents_monster)
#define mask_water (contents_water | contents_moveable | contents_slime)
#define mask_opaque (contents_solid | contents_moveable | contents_opaque)
#define mask_opaque_and_npcs (mask_opaque | contents_monster)
#define mask_blocklos (contents_solid | contents_moveable | contents_blocklos)
#define mask_blocklos_and_npcs (mask_blocklos | contents_monster)
#define mask_visible (mask_opaque | contents_ignore_nodraw_opaque)
#define mask_visible_and_npcs (mask_opaque_and_npcs | contents_ignore_nodraw_opaque)
#define mask_shot                                                                                                      \
	(contents_solid | contents_moveable | contents_monster | contents_window | contents_debris | contents_hitbox)
#define mask_shot_brushonly (contents_solid | contents_moveable | contents_window | contents_debris)
#define mask_shot_hull                                                                                                 \
	(contents_solid | contents_moveable | contents_monster | contents_window | contents_debris | contents_grate)
#define mask_shot_player (mask_shot_hull | contents_hitbox)
#define mask_shot_portal (contents_solid | contents_moveable | contents_window | contents_monster)
#define mask_solid_brushonly (contents_solid | contents_moveable | contents_window | contents_grate)
#define mask_playersolid_brushonly                                                                                     \
	(contents_solid | contents_moveable | contents_window | contents_playerclip | contents_grate)
#define mask_npcsolid_brushonly                                                                                        \
	(contents_solid | contents_moveable | contents_window | contents_monsterclip | contents_grate)
#define mask_npcworldstatic (contents_solid | contents_window | contents_monsterclip | contents_grate)
#define mask_npcworldstatic_fluid (contents_solid | contents_window | contents_monsterclip)
#define mask_splitareaportal (contents_water | contents_slime)
#define mask_current                                                                                                   \
	(contents_current_0 | contents_current_90 | contents_current_180 | contents_current_270 | contents_current_up |    \
	 contents_current_down)
#define mask_deadsolid (contents_solid | contents_playerclip | contents_window | contents_grate)

#define hitgroup_generic 0
#define hitgroup_head 1
#define hitgroup_chest 2
#define hitgroup_stomach 3
#define hitgroup_leftarm 4
#define hitgroup_rightarm 5
#define hitgroup_leftleg 6
#define hitgroup_rightleg 7
#define hitgroup_neck 8
#define hitgroup_gear 10

namespace sdk
{
enum trace_type
{
	trace_everything = 0,
	trace_world_only,
	trace_entities_only,
	trace_everything_filter_props,
};

class trace_filter
{
public:
	virtual bool should_hit_entity(entity_t *entity, int contents_mask) = 0;
	virtual trace_type get_trace_type() const = 0;
};

class trace_simple_filter : public trace_filter
{
public:
	trace_simple_filter(entity_t *ent)
	{
		this->ent = ent;
		*reinterpret_cast<uintptr_t *>(this) = game->client.at(globals::trace_filter_simple_vtable);
	}

	// those won't get called.
	bool should_hit_entity(entity_t *ent, int) override { return false; }

	trace_type get_trace_type() const override { return trace_everything; }

	entity_t *get_skip_entity() { return ent; }

	void set_skip_entity(entity_t *const other) { ent = other; }

	entity_t *ent;

private:
	uint32_t pad[2]{};
};

class trace_world_filter : public trace_filter
{
public:
	bool should_hit_entity(entity_t *ent, int) override { return false; }

	trace_type get_trace_type() const override { return trace_everything; }
};

class trace_no_player_filter : public trace_filter
{
public:
	bool should_hit_entity(entity_t *ent, int) override { return !(ent && ent->is_player()); }

	trace_type get_trace_type() const override { return trace_everything; }
};

class trace_player_filter : public trace_filter
{
public:
	trace_player_filter(int32_t player) : player(player) {}

	bool should_hit_entity(entity_t *ent, int) override { return ent && ent->index() == player; }

	trace_type get_trace_type() const override { return trace_entities_only; }

private:
	int32_t player;
};

class trace_friendly_filter : public trace_filter
{
public:
	trace_friendly_filter(cs_player_t *player) : player(player) {}

	bool should_hit_entity(entity_t *ent, int) override
	{
		return !ent || !ent->is_player() ||
			   (ent->index() != player->index() && !reinterpret_cast<cs_player_t *>(ent)->is_enemy(player));
	}

	trace_type get_trace_type() const override { return trace_everything; }

private:
	cs_player_t *player;
};

class trace_no_grenades_filter : public trace_filter
{
public:
	trace_no_grenades_filter(entity_t *ent) { this->ent = ent; }

	bool should_hit_entity(entity_t *ent, int contents_mask) override
	{
		if (ent == this->ent)
			return false;

		if (ent->get_class_id() == class_id::base_csgrenade_projectile)
			return false;

		return true;
	}

	trace_type get_trace_type() const override { return trace_everything; }

	entity_t *ent;
};

struct plane
{
	vec3 normal{};
	float dist{};
	uint8_t type{};
	uint8_t signbits{};
	uint8_t pad[2]{};
};

class base_trace
{
public:
	vec3 startpos{};
	vec3 endpos{};
	plane plane{};
	float fraction = 1.f;
	int contents{};
	unsigned short disp_flags{};
	bool allsolid{};
	bool startsolid{};
};

struct surf
{
	const char *name{};
	short surface_props{};
	unsigned short flags{};
};

class game_trace : public base_trace
{
public:
	bool did_hit() const { return fraction < 1 || allsolid || startsolid; }

	bool did_hit_world() const { return entity == game->client_entity_list->get_client_entity(0); }

	bool did_hit_non_world_entity() const { return entity != nullptr && !did_hit_world(); }

	float fractionleftsolid{};
	surf surface{"**empty**", 0, 0};
	int hitgroup{};
	short physicsbone{};
	unsigned short world_surface_index{};
	entity_t *entity{};
	int hitbox{};
};

class engine_trace_t
{
public:
	VIRTUAL(1, get_point_contents_world_only(vec3 &abs_position, int mask), int(__thiscall *)(void *, vec3 &, int))
	(abs_position, mask);
	VIRTUAL(3, clip_ray_to_entity(ray &r, const uint32_t mask, entity_t *ent, game_trace *trace),
			void(__thiscall *)(void *, ray &, uint32_t, entity_t *, game_trace *))
	(r, mask, ent, trace);
	VIRTUAL(5, trace_ray(ray &r, const uint32_t mask, trace_filter *filter, game_trace *trace),
			void(__thiscall *)(void *, ray &, uint32_t, trace_filter *, game_trace *))
	(r, mask, filter, trace);
};
} // namespace sdk

#endif // SDK_ENGINE_TRACE_H
