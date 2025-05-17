
#include <base/debug_overlay.h>
#include <detail/aim_helper.h>
#include <hacks/antiaim.h>
#include <hacks/grenade_prediction.h>
#include <hacks/misc.h>
#include <sdk/debug_overlay.h>
#include <sdk/glow_manager.h>
#include <sdk/math.h>

using namespace detail;
using namespace sdk;

namespace hacks
{
std::shared_ptr<grenade_prediction> grenade_predict = std::make_shared<grenade_prediction>();

void grenade_prediction::draw()
{
	constexpr auto radius = 52;

	if (!game->local_player || !game->local_player->is_alive())
		return;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!cfg.world_esp.visualize_nade_path.get() || !wpn)
		return;

	simulate(wpn);

	if (grenade_path.size() <= 1)
		return;

	const auto clr = cfg.world_esp.nade_path_color.get();
	auto prev = grenade_path.front();

	for (auto &cur : grenade_path)
	{
		angle trajectory;
		vec3 delta_pos(prev - cur);
		vector_angles(delta_pos, trajectory);

		game->glow_manager->add_glow_box(cur, trajectory, {0.f, -.2f, -.2f}, {delta_pos.length(), .2f, .2f}, clr);
		prev = cur;
	}

	if (landed)
	{
		auto last_seg = land_position;

		// helper lambda to check if we are within the segement bounds.
		const auto in_segment = [](int32_t i, float start, float end)
		{
			return (i > start && i < end)					   // default case.
				   || (end < start && (i > start || i < end)); // looping around?
		};

		// timer to position our segements dynamically...
		const auto segment = static_cast<float>(radius) / 4.f;
		const auto timer = fmodf(game->globals->curtime, 1.f);

		// start and end positions of our two segments...
		const auto segment_1_start = fmodf(1.f * segment + timer * radius, radius);
		const auto segment_1_end = fmodf(2.f * segment + timer * radius, radius);
		const auto segment_2_start = fmodf(3.f * segment + timer * radius, radius);
		const auto segment_2_end = fmodf(4.f * segment + timer * radius, radius);

		for (auto i = 0; i < radius + 1; i++)
		{
			if (!in_segment(i, segment_1_start, segment_1_end) && !in_segment(i, segment_2_start, segment_2_end))
				continue;

			const auto x = land_position.x + radius * cosf(i * 2.f * sdk::pi / radius);
			const auto y = land_position.y - radius * sinf(i * 2.f * sdk::pi / radius);
			const auto seg = vec3{x, y, land_position.z};
			vec3 delta_pos(last_seg - seg);

			// only draw joint segments...
			if (delta_pos.length2d() < .25f * radius)
			{
				const auto deg = RAD2DEG(atan2f(delta_pos.y, delta_pos.x));
				angle trajectory{0.f, deg - floorf(deg / 360.0f + .5f) * 360.0f, 0.f};
				game->glow_manager->add_glow_box(seg, trajectory, {0.f, -.2f, -.2f}, {delta_pos.length(), .2f, .2f},
												 color(255, 0, 0));
			}

			last_seg = seg;
		}
	}
	else
		game->glow_manager->add_glow_box(prev, angle(), {-2.f, -2.f, -2.f}, {2.f, 2.f, 2.f}, color(255, 0, 0));
}

void grenade_prediction::adjust_throw_velocity(angle &ang)
{
	if (!cfg.misc.nade_assistant.get())
		return;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	auto throw_velocity = fmin(fmax(wpn->get_cs_weapon_data()->flthrowvelocity * .9f, 15.f), 750.f);
	throw_velocity *= (.7f * wpn->get_throw_strength() + .3f);
	vec3 aim_throw = ang, forward;
	if (aim_throw.x > 90.f)
		aim_throw.x -= 360.f;
	else if (aim_throw.x < -90.f)
		aim_throw.x += 360.f;
	aim_throw.x -= (90.f - fabsf(aim_throw.x)) * 10.f / 90.f;
	angle_vectors(aim_throw, forward);
	auto vec_throw_target = forward * throw_velocity;
	auto vec_throw = vec_throw_target + game->local_player->get_velocity() * 1.4f;
	angle ang_throw_target, ang_throw;
	vector_angles(vec_throw_target, ang_throw_target);
	vector_angles(vec_throw, ang_throw);
	ang += vec3(ang_throw_target.x - ang_throw.x, ang_throw_target.y - ang_throw.y, 0.f);
	normalize(ang);
}

std::optional<vec3> grenade_prediction::to_end(weapon_t *wpn, cs_player_t *thrower, bool molotov, float *progress)
{
	reset();
	weapon_id = molotov ? item_definition_index::weapon_molotov : item_definition_index::weapon_hegrenade;

	auto &time = game->nade_throw_times[thrower->index()];
	if (thrower->is_dormant() &&
		fabsf(wpn->get_spawn_time() - time) < max(1.5f, GET_CONVAR_FLOAT("molotov_throw_detonate_time")))
		wpn->get_spawn_time() = time;
	time = FLT_MAX;

	if (thrower->is_dormant() && !wpn->get_bounces())
	{
		const auto grav_dt = wpn->get_v_initial_velocity().z - wpn->get_grenade_velocity().z;
		const auto scaled_gravity = GET_CONVAR_FLOAT("sv_gravity") * .4f * game->globals->interval_per_tick;
		const auto time_passed = TICKS_TO_TIME(int32_t(grav_dt / scaled_gravity));
		wpn->get_spawn_time() = wpn->get_simulation_time() - time_passed;
	}

	// if the nade throw happened in the past, assume the player was fully behind too!
	if (wpn->get_simulation_time() < wpn->get_spawn_time())
		wpn->get_spawn_time() = wpn->get_simulation_time() - TICKS_TO_TIME(TIME_TO_TICKS(.03f));

	// this is probably some weapon_t constant by which the throw is delayed that i can't find...
	const auto weapon_dt = molotov ? .0546875 : .1484375f;
	const auto dt_tick = TIME_TO_TICKS(wpn->get_simulation_time() - wpn->get_spawn_time() - weapon_dt);
	const auto steps = TIME_TO_TICKS(will_detonate()) - dt_tick;

	trace_simple_filter filter(thrower);
	auto start_pos = wpn->get_origin(), throw_pos = wpn->get_grenade_velocity();

	auto i = steps;
	for (; i >= 0; i--)
		if (step(start_pos, throw_pos, filter) == state_detonate)
			break;

	if (progress)
		*progress =
			std::clamp(1.f - TICKS_TO_TIME(steps - i) / TICKS_TO_TIME(TIME_TO_TICKS(will_detonate()) - i), 0.f, 1.f);

	game_trace trace;
	hull_trace(filter, start_pos, start_pos - vec3(0.f, 0.f, 12.5f), trace,
			   mask_solid | contents_current_90 | contents_slime | contents_water);
	return trace.endpos;
}

void grenade_prediction::setup(trace_simple_filter &filter, weapon_t *wpn, vec3 &vec_src, vec3 &vec_throw)
{
	weapon_id = wpn->get_item_definition_index();
	angle ang_throw = game->engine_client->get_view_angles();
	adjust_throw_velocity(ang_throw);
	vec3 forward;
	if (ang_throw.x > 90.f)
		ang_throw.x -= 360.f;
	else if (ang_throw.x < -90.f)
		ang_throw.x += 360.f;
	ang_throw.x -= (90.f - fabsf(ang_throw.x)) * 10.f / 90.f;
	angle_vectors(ang_throw, forward);

	const auto throw_strength = wpn->get_throw_strength();
	const auto throw_height = throw_strength * 12.f - 12.f;
	auto throw_velocity = fmin(fmax(wpn->get_cs_weapon_data()->flthrowvelocity * .9f, 15.f), 750.f);
	throw_velocity *= (.7f * throw_strength + .3f);

	const auto start_pos =
		aa.get_visual_eye_position() + vec3(0.f, 0.f, throw_height) + game->local_player->get_velocity() * .1f;
	const auto end_pos = start_pos + forward * 22.f;

	game_trace trace;
	hull_trace(filter, start_pos, end_pos, trace, mask_solid | contents_current_90 | contents_slime | contents_water);

	vec_src = trace.endpos - forward * 6.f;
	vec_throw = forward * throw_velocity + game->local_player->get_velocity() * 1.25f;
}

void grenade_prediction::simulate(weapon_t *wpn)
{
	reset();

	if (!wpn->is_grenade() || !wpn->get_pin_pulled())
		return;

	vec3 start_pos, throw_pos;
	trace_simple_filter filter(game->local_player);
	setup(filter, wpn, start_pos, throw_pos);

	const auto steps = TIME_TO_TICKS(will_detonate()) + 1;
	grenade_path.reserve(steps);

	for (auto i = steps; i >= 0; i--)
	{
		grenade_path.emplace_back(start_pos);

		const auto step_value = step(start_pos, throw_pos, filter);

		if (step_value == state_detonate)
			break;

		if (step_value == state_colide)
			grenade_collision.emplace_back(start_pos);
	}

	game_trace trace;
	hull_trace(filter, start_pos, start_pos - vec3(0.f, 0.f, 12.5f), trace,
			   mask_solid | contents_current_90 | contents_slime | contents_water);
	landed = trace.entity || trace.fraction != 1.f;
	land_position = trace.endpos;
}

states grenade_prediction::step(sdk::vec3 &vec_src, sdk::vec3 &vec_throw, sdk::trace_simple_filter &filter)
{
	uint32_t mask = mask_solid | contents_current_90 | contents_slime | contents_water;

	if (did_enter_water)
		mask = mask_solid | contents_current_90;

	states return_value = state_none;

	// apply gravity
	vec3 move;
	apply_gravity(move, vec_throw);

	// move entity
	game_trace tr;
	move_entity(filter, vec_src, move, tr, mask);

	// we hit something
	if (tr.entity)
	{
		if (tr.entity->is_player())
		{
			filter.set_skip_entity(tr.entity);
			move_entity(filter, vec_src, move, tr, mask);
			vec_throw *= .2f;
		}
		else if (tr.entity->is_breakable())
		{
			filter.set_skip_entity(tr.entity);
			move_entity(filter, vec_src, move, tr, mask);
			vec_throw *= .4f;
		}
	}

	if (tr.contents & contents_water && !did_enter_water)
	{
		did_enter_water = true;
		move_entity(filter, tr.endpos, move * .5f, tr);
		vec_throw *= .5f;
	}

	if (tr.fraction != 1.f)
		return_value = calculate_path(filter, tr, vec_throw);

	vec_src = tr.endpos;

	return return_value;
}

void grenade_prediction::move_entity(sdk::trace_simple_filter &filter, sdk::vec3 &vec_src, const sdk::vec3 &move,
									 sdk::game_trace &tr, uint32_t mask)
{
	const vec3 vec_end = vec_src + move;
	hull_trace(filter, vec_src, vec_end, tr, mask);
}

void grenade_prediction::hull_trace(sdk::trace_simple_filter &filter, const sdk::vec3 &vec_src,
									const sdk::vec3 &vec_dest, sdk::game_trace &tr, uint32_t mask)
{
	ray r;
	r.init(vec_src, vec_dest, vec3(-2.f, -2.f, -2.f), vec3(2.f, 2.f, 2.f));
	game->engine_trace->trace_ray(r, mask, &filter, &tr);
}

void grenade_prediction::apply_gravity(sdk::vec3 &move, sdk::vec3 &vec_velocity)
{
	const auto gravity = GET_CONVAR_FLOAT("sv_gravity") * .4f;
	move = vec_velocity * game->globals->interval_per_tick;
	const auto new_velocity_z = vec_velocity.z - gravity * game->globals->interval_per_tick;
	move.z = (vec_velocity.z + new_velocity_z) * .5f * game->globals->interval_per_tick;
	vec_velocity.z = new_velocity_z;
}

states grenade_prediction::calculate_path(sdk::trace_simple_filter &filter, sdk::game_trace &tr,
										  sdk::vec3 &vec_velocity)
{
	const auto physics_clip_velocity = [](const vec3 &in, const vec3 &normal, vec3 &out, float overbounce)
	{
		int blocked = 0;
		const float angle = normal.y;

		if (angle > 0)
			blocked |= 1; // floor

		if (!angle)
			blocked |= 2; // step

		const float backoff = in.dot(normal) * overbounce;

		for (int i = 0; i < 3; i++)
		{
			out[i] = in[i] - (normal[i] * backoff);

			if (out[i] > -.1f && out[i] < .1f)
				out[i] = 0;
		}

		return blocked;
	};

	if (weapon_id == item_definition_index::weapon_tagrenade)
		return state_detonate;

	auto surface_elasticity = 1.f;

	// players block nades
	if (tr.entity && tr.entity->is_player())
		surface_elasticity = .3f;

	const auto grenade_elasticity = .45f;
	const auto total_elasticity = std::clamp(grenade_elasticity * surface_elasticity, 0.f, .9f);

	if (tr.allsolid)
		vec_velocity = vec3(0.f, 0.f, 0.f);

	vec3 vec_new_velocity;

	physics_clip_velocity(vec_velocity, tr.plane.normal, vec_new_velocity, 2.f);
	vec_new_velocity *= total_elasticity;

	vec_velocity = vec_new_velocity;

	if (vec_velocity.length2d() <= 20.f &&
		(weapon_id == item_definition_index::weapon_decoy || weapon_id == item_definition_index::weapon_smokegrenade))
		return state_detonate;

	const auto plane_z = tr.plane.normal.z;
	if (plane_z > .7f)
	{
		const auto sqr = vec_new_velocity.dot(vec_new_velocity);
		if (sqr > 96000.f)
			if (const auto l = vec3(vec_new_velocity).normalize().dot(tr.plane.normal); l > .5f)
				vec_new_velocity *= 1.f - l + .5f;

		if (sqr < 400.f)
			vec_velocity = vec3();
		else
		{
			vec_velocity = vec_new_velocity;
			vec_new_velocity *= (1.f - tr.fraction) * game->globals->interval_per_tick;
			move_entity(filter, tr.endpos, vec_new_velocity, tr);
		}
	}
	else
	{
		vec_velocity = vec_new_velocity;
		vec_new_velocity *= (1.f - tr.fraction) * game->globals->interval_per_tick;
		move_entity(filter, tr.endpos, vec_new_velocity, tr);
	}

	if ((weapon_id == item_definition_index::weapon_molotov || weapon_id == item_definition_index::weapon_incgrenade) &&
		plane_z > cos(DEG2RAD(GET_CONVAR_FLOAT("weapon_molotov_maxdetonateslope"))))
		return state_detonate;

	return state_colide;
}

float grenade_prediction::will_detonate() const
{
	switch (weapon_id)
	{
	case item_definition_index::weapon_flashbang:
	case item_definition_index::weapon_hegrenade:
		return 1.5f;
	case item_definition_index::weapon_incgrenade:
	case item_definition_index::weapon_molotov:
		return GET_CONVAR_FLOAT("molotov_throw_detonate_time");
	case item_definition_index::weapon_decoy:
	case item_definition_index::weapon_smokegrenade:
		return 10.f;
	case item_definition_index::weapon_tagrenade:
		return 5.f;
	default:
		break;
	}

	return 3.f;
}
} // namespace hacks