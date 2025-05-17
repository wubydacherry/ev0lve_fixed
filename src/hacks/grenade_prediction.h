
#ifndef HACKS_GRENADE_PREDICTION_H
#define HACKS_GRENADE_PREDICTION_H

#include <base/draw_manager.h>
#include <optional>
#include <sdk/engine_trace.h>
#include <sdk/input.h>

enum states
{
	state_none = 0,
	state_detonate,
	state_colide
};

namespace hacks
{
class grenade_prediction
{
public:
	void draw();
	void adjust_throw_velocity(sdk::angle &ang);
	std::optional<sdk::vec3> to_end(sdk::weapon_t *wpn, sdk::cs_player_t *thrower, bool molotov = false,
									float *progress = nullptr);

private:
	void setup(sdk::trace_simple_filter &filter, sdk::weapon_t *wpn, sdk::vec3 &vec_src, sdk::vec3 &vec_throw);
	void simulate(sdk::weapon_t *wpn);
	states step(sdk::vec3 &vec_src, sdk::vec3 &vec_throw, sdk::trace_simple_filter &filter);
	void move_entity(sdk::trace_simple_filter &filter, sdk::vec3 &vec_src, const sdk::vec3 &move, sdk::game_trace &tr,
					 uint32_t mask = mask_solid | contents_current_90);
	void hull_trace(sdk::trace_simple_filter &filter, const sdk::vec3 &vec_src, const sdk::vec3 &vec_dest,
					sdk::game_trace &tr, uint32_t mask = mask_solid | contents_current_90);
	void apply_gravity(sdk::vec3 &move, sdk::vec3 &vec_velocity);
	states calculate_path(sdk::trace_simple_filter &filter, sdk::game_trace &tr, sdk::vec3 &vec_velocity);

	[[nodiscard]] float will_detonate() const;

	sdk::item_definition_index weapon_id = sdk::item_definition_index::weapon_hegrenade;
	std::vector<sdk::vec3> grenade_path;
	std::vector<sdk::vec3> grenade_collision;
	bool did_enter_water = false;
	bool landed = false;
	sdk::vec3 land_position{};

	void reset()
	{
		grenade_path.clear();
		grenade_collision.clear();
		did_enter_water = landed = false;
		land_position = {};
		weapon_id = sdk::item_definition_index::weapon_hegrenade;
	}
};

extern std::shared_ptr<grenade_prediction> grenade_predict;
} // namespace hacks

#endif // HACKS_GRENADE_PREDICTION_H
