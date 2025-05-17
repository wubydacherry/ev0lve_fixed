
#ifndef SDK_GLOW_MANAGER_H
#define SDK_GLOW_MANAGER_H

#include <sdk/color.h>
#include <sdk/cutlvector.h>
#include <sdk/entity.h>
#include <sdk/global_vars_base.h>
#include <sdk/macros.h>

namespace sdk
{
struct glow_object_definition
{
	int32_t next_free_slot;
	client_entity *ent;
	vec3 glow_color;
	float glow_alpha;

	bool glow_alpha_capped_by_renderer;
	float glow_alpha_fn_of_max_vel;
	float glow_alpha_max;
	float glow_pulse_overdrive;
	bool render_when_occluded;
	bool render_when_unoccluded;
	bool full_bloom_render;
	int32_t full_bloom_stencil_test_value;
	int32_t render_style;
	int32_t split_screen_slot;
};

struct glow_box_definition
{
	vec3 position;
	angle orient;
	vec3 mins, maxs;
	float birth_time, termination_time;
	uint32_t clr;
};

class glow_manager_t
{
public:
	inline void add_glow_box(const vec3 &origin, const angle &orient, const vec3 &mins, const vec3 &maxs,
							 const color &clr, float time = 0.f)
	{
		glow_box_definition def{origin,
								orient,
								mins,
								maxs,
								time == 0.f ? 0.f : game->globals->curtime - .25f,
								time == 0.f ? 2.f * game->globals->curtime : game->globals->curtime + time,
								clr.encoded()};
		glow_box_definitions.add_to_tail(def);
	}

	inline void add_glow(client_entity *const ent, const evo::ren::color &clr)
	{
		for (auto i = 0; i < glow_object_definitions.count(); ++i)
		{
			auto &def = glow_object_definitions[i];

			if (def.ent == ent)
			{
				def.render_when_occluded = true;
				def.render_when_unoccluded = false;
				def.glow_color = {clr.value.r, clr.value.g, clr.value.b};
				def.glow_alpha = {clr.value.a * .8f};
				break;
			}
		}
	}

	inline void clear()
	{
		for (auto n = glow_box_definitions.count() - 1; n >= 0; n--)
			if (glow_box_definitions[n].birth_time == 0.f)
				glow_box_definitions.fast_remove(n);
	}

	cutlvector<glow_object_definition> glow_object_definitions;
	int32_t first_free_slot;
	cutlvector<glow_box_definition> glow_box_definitions;
};
} // namespace sdk

#endif // SDK_GLOW_MANAGER_H
