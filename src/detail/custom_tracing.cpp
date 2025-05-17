
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <detail/custom_tracing.h>
#include <detail/dispatch_queue.h>
#include <sdk/debug_overlay.h>
#include <sdk/surface_props.h>

using namespace sdk;

namespace detail
{
custom_tracing trace{};

custom_tracing::wall_pen custom_tracing::wall_penetration(vec3 src, vec3 end, lag_record *target,
														  bullet_safety scan_secure,
														  std::optional<resolver_direction> override_direction,
														  cs_player_t *const override_player, bool no_opt,
														  cs_weapon_data *override_data, int32_t expected_hitgroup)
{
	const auto player = override_player ? override_player : game->local_player;
	const auto wpn = override_data
						 ? nullptr
						 : reinterpret_cast<weapon_t *>(
							   game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));

	if (!wpn && !(!target && override_data))
		return {};

	wall_pen result{};

	// run bullet simulation.
	if (target)
	{
		trace_friendly_filter filter(player);
		result = fire_bullet(wpn->get_cs_weapon_data(), src, end, &filter, target, scan_secure, override_direction,
							 override_player ? false
											 : wpn->get_item_definition_index() == item_definition_index::weapon_taser,
							 no_opt, expected_hitgroup);
	}
	else
	{
		if (override_data)
		{
			trace_no_player_filter filter{};
			result = fire_bullet(override_data, src, end, &filter, target, scan_secure, override_direction, false,
								 no_opt, expected_hitgroup);
		}
		else
		{
			trace_simple_filter filter(player);
			result = fire_bullet(
				wpn->get_cs_weapon_data(), src, end, &filter, target, scan_secure, override_direction,
				override_player ? false : wpn->get_item_definition_index() == item_definition_index::weapon_taser,
				no_opt, expected_hitgroup);
		}
	}

	// filter low dmg.
	if (result.damage < 1.f)
	{
		result.damage = 0.f;
		result.did_hit = false;
	}

	// report result.
	return result;
}

void custom_tracing::check_wallbang()
{
	wallbang = false;

	if (!cfg.misc.penetration_crosshair.get())
		return;

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn || wpn->is_knife() || wpn->is_grenade() ||
		wpn->get_item_definition_index() == item_definition_index::weapon_taser)
		return;

	const auto wpn_data = wpn->get_cs_weapon_data();
	const auto angles = game->engine_client->get_view_angles();
	auto eye_pos = game->local_player->get_eye_position();

	game_trace tr;
	vec3 direction;

	// get trace end point
	angle_vectors(angles, direction);
	const vec3 end_pos = eye_pos + direction * wpn_data->range;

	ray r;
	trace_friendly_filter filter(game->local_player);

	// init ray
	r.init(eye_pos, end_pos);

	// run trace
	game->engine_trace->trace_ray(r, mask_shot, &filter, &tr);
	auto penetration_count = 1;
	auto damage = static_cast<float>(wpn_data->idamage);
	wallbang =
		handle_bullet_penetration(wpn_data, tr, eye_pos, direction, penetration_count, damage, wpn_data->flpenetration);
}

custom_tracing::wall_pen custom_tracing::fire_bullet(cs_weapon_data *const data, vec3 src, vec3 pos,
													 trace_filter *const filter, lag_record *target,
													 bullet_safety scan_secure,
													 std::optional<resolver_direction> override_direction, bool is_zeus,
													 bool no_opt, int32_t expected_hitgroup)
{
	vec3 angles;
	vector_angles(pos - src, angles);

	if (!angles.is_valid())
		return {};

	vec3 direction;
	angle_vectors(angles, direction);

	if (!direction.is_valid())
		return {};

	direction.normalize();

	const auto start = src;
	const auto max_length = data->range;
	auto penetrate_count = 4;
	auto length = 0.f, damage = static_cast<float>(data->idamage);
	game_trace enter_trace{}, final_trace{};
	wall_pen pen{};
	pen.direction = direction;
	pen.safety = scan_secure;

	if (target)
	{
		if (target->player->is_fake_player())
			scan_secure = bullet_safety::none;

		mat3x4 *mat_[max_bones];

		const auto test = [&](const mat3x4 *mat)
		{
			ray r{};
			r.init(src, src + direction * (max_length + ray_extension));
			game_trace tr{};
			tr.startpos = r.start + r.start_offset;
			tr.endpos = tr.startpos + r.delta;
			for (auto i = 0; i < max_bones; i++)
				mat_[i] = const_cast<mat3x4 *>(&mat[i]);
			const auto ret = trace_to_studio_csgo_hitgroups_priority(target->player, mask_shot_player, &target->origin,
																	 &tr, &r, mat_);
			final_trace = tr;

			if (expected_hitgroup != -1 && tr.hitgroup != expected_hitgroup)
				return false;

			return ret;
		};

		pen.did_hit = test(override_direction.has_value() ? target->res_mat[override_direction.value()]
														  : target->res_mat[target->res_direction]);

		if (!pen.did_hit)
			return std::move(pen);
	}

	pen.did_hit = false;
	pen.damage = 0.f;

	while (damage > 0.f)
	{
		const auto length_remaining = max_length - length;
		auto end = src + direction * length_remaining;

		if (enter_trace.entity)
			reinterpret_cast<trace_simple_filter *>(filter)->set_skip_entity(enter_trace.entity);

		ray r{};
		r.init(src, end);
		game->engine_trace->trace_ray(r, mask_shot, filter, &enter_trace);

		if (target && target->do_not_set)
			continue;

		if (!target && enter_trace.startsolid)
		{
			damage = 0;
			break;
		}

		if (target && !pen.did_hit)
		{
			const auto dist = (start - target->origin).length();
			const auto behind = (start - enter_trace.endpos).length() > dist;

			if (behind || enter_trace.fraction == 1.f)
			{
				ray r2{};
				r2.init(src, src + direction * (max_length + ray_extension));
				final_trace.fraction = (final_trace.endpos - r2.start).length() / r2.delta.length();
				enter_trace = final_trace;

				auto smallest_fraction = (src - enter_trace.endpos).length() / length_remaining;
				if (const auto range = distance_to_ray(
						target->origin + vec3(0.f, 0.f, .5f * target->player->get_collideable()->obb_maxs().z),
						r2.start, r2.start + r2.delta);
					range >= 0.0f && range <= 60.f && enter_trace.fraction < smallest_fraction)
					smallest_fraction = enter_trace.fraction;
				const auto final_length = length + smallest_fraction * (length_remaining + ray_extension);

				if (final_length >= max_length)
					break;

				const auto final_damage = damage * std::powf(data->flrangemodifier, final_length * .002f);
				const auto new_damage = scale_damage(target->player, final_damage, data->flarmorratio,
													 is_zeus ? hitgroup_generic : enter_trace.hitgroup);

				pen.did_hit = true;
				pen.end = enter_trace.endpos;
				pen.impacts[pen.impact_count++] = enter_trace.endpos;
				pen.hitbox = cs_player_t::hitbox(enter_trace.hitbox);
				pen.hitgroup = enter_trace.hitgroup;
				pen.damage = new_damage;
				pen.potential_damage =
					scale_damage(target->player,
								 data->idamage * std::powf(data->flrangemodifier, (start - pen.end).length() * .002f),
								 data->flarmorratio, is_zeus ? hitgroup_generic : enter_trace.hitgroup);

				if (!no_opt)
					break;
			}
		}
		else if (!target)
		{
			const auto dist = (start - pos).length();
			const auto behind = (start - enter_trace.endpos).length() > dist;

			if (behind || enter_trace.fraction == 1.f)
			{
				length += (pos - enter_trace.startpos).length();
				damage *= std::powf(data->flrangemodifier, length * .002f);
				break;
			}
		}

		length += enter_trace.fraction * length_remaining;
		damage *= std::powf(data->flrangemodifier, length * .002f);

		if (!no_opt && length > (start - pos).length())
			break;

		if (enter_trace.fraction == 1.f)
		{
			if (!target)
				pen.did_hit = true;
			break;
		}

		pen.impacts[pen.impact_count++] = enter_trace.endpos;

		const auto enter_surface = game->surface_props->get_surface_data(enter_trace.surface.surface_props);
		if ((length > 3000.f && data->flpenetration > 0.f) ||
			(!enter_surface || enter_surface->game.penetration_mod < .1f))
			penetrate_count = 0;

		if (!handle_bullet_penetration(data, enter_trace, src, direction, penetrate_count, damage, data->flpenetration))
		{
			damage = 0.f;
			break;
		}
	}

	if (!pen.did_hit || !target)
		pen.damage = damage;

	return std::move(pen);
}

bool custom_tracing::handle_bullet_penetration(const cs_weapon_data *const weapon_data, game_trace &enter_trace,
											   vec3 &eye_position, vec3 direction, int &penetrate_count,
											   float &current_damage, float penetration_power) const
{
	game_trace exit_trace{};
	auto enemy = reinterpret_cast<cs_player_t *>(enter_trace.entity);
	const auto enter_surface_data = game->surface_props->get_surface_data(enter_trace.surface.surface_props);
	const int enter_material = enter_surface_data->game.material;
	const bool is_grate = enter_trace.contents & contents_grate;
	const bool is_no_draw = !!(enter_trace.surface.flags & surf_nodraw);

	if (weapon_data->flpenetration <= 0.f || penetrate_count <= 0 ||
		(!trace_to_exit(enter_trace, exit_trace, enter_trace.endpos, direction) &&
		 !(game->engine_trace->get_point_contents_world_only(enter_trace.endpos, mask_shot_hull) & mask_shot_hull)))
		return false;

	const auto exit_surface_data = game->surface_props->get_surface_data(exit_trace.surface.surface_props);
	const auto exit_material = exit_surface_data->game.material;

	auto damage_lost = .16f;
	auto penetration_modifier = enter_surface_data->game.penetration_mod;
	if (is_grate || is_no_draw || enter_material == char_tex_grate || enter_material == char_tex_glass)
	{
		if (enter_material == char_tex_grate || enter_material == char_tex_glass)
		{
			penetration_modifier = 3.f;
			damage_lost = .05f;
		}
		else
			penetration_modifier = 1.f;
	}
	else if (enter_material == char_tex_flesh && enemy && !enemy->is_enemy() &&
			 GET_CONVAR_FLOAT("ff_damage_reduction_bullets") == 0.f)
	{
		const auto damage_bullet_penetration = GET_CONVAR_FLOAT("ff_damage_bullet_penetration");
		if (damage_bullet_penetration == 0.f)
			return false;

		penetration_modifier = damage_bullet_penetration;
	}
	else
		penetration_modifier = (penetration_modifier + exit_surface_data->game.penetration_mod) / 2.f;

	if (enter_material == exit_material)
	{
		if (exit_material == char_tex_cardboard || exit_material == char_tex_wood)
			penetration_modifier = 3.f;
		else if (exit_material == char_tex_plastic)
			penetration_modifier = 2.f;
	}

	const auto dist = (exit_trace.endpos - enter_trace.endpos).length();
	const auto pen_mod = max(0.f, (1.f / penetration_modifier));
	const auto wpn_mod = current_damage * damage_lost + max(0.f, (3.f / penetration_power) * 1.25f) * (pen_mod * 3.f);
	const auto lost_damage = wpn_mod + (pen_mod * dist * dist) / 24.f;
	current_damage -= max(0.f, lost_damage);

	if (current_damage < 1.f)
		return false;

	eye_position = exit_trace.endpos;
	--penetrate_count;
	return true;
}

bool custom_tracing::trace_to_exit(game_trace &enter_trace, game_trace &exit_trace, vec3 start_position,
								   vec3 direction) const
{
	static constexpr auto max_distance = 90.f;
	static constexpr auto ray_extension = 4.f;

	float current_distance = 0;
	auto first_contents = 0;

	while (current_distance <= max_distance)
	{
		current_distance += ray_extension;

		auto start = start_position + direction * current_distance;

		const auto point_contents = game->engine_trace->get_point_contents_world_only(start, mask_shot_player);

		if (!first_contents)
			first_contents = point_contents;

		if (!(point_contents & mask_shot_hull) ||
			((point_contents & contents_hitbox) && point_contents != first_contents))
		{
			const auto end = start - direction * ray_extension;

			ray r{};
			r.init(start, end);
			trace_simple_filter filter(game->local_player);
			game->engine_trace->trace_ray(r, mask_shot_player, &filter, &exit_trace);

			if (exit_trace.startsolid && exit_trace.surface.flags & surf_hitbox)
			{
				r.init(start, start_position);
				filter.ent = exit_trace.entity;
				game->engine_trace->trace_ray(r, mask_shot_player, &filter, &exit_trace);

				if (exit_trace.did_hit() && !exit_trace.startsolid)
					return true;
			}
			else if (exit_trace.did_hit() && !exit_trace.startsolid)
			{
				if (enter_trace.surface.flags & surf_nodraw && enter_trace.entity->is_breakable() &&
					exit_trace.entity->is_breakable())
					return true;
				else if ((!(exit_trace.surface.flags & surf_nodraw) || enter_trace.surface.flags & surf_nodraw) &&
						 exit_trace.plane.normal.dot(direction) <= 1.f)
					return true;
			}
			else if (enter_trace.did_hit_non_world_entity() && enter_trace.entity->is_breakable())
			{
				exit_trace = enter_trace;
				exit_trace.endpos = start + direction;
				return true;
			}
		}
	}

	return false;
}

float custom_tracing::scale_damage(cs_player_t *const target, float damage, float weapon_armor_ratio,
								   int hitgroup) const
{
	const auto is_armored = [&]() -> bool
	{
		if (target->get_armor() > 0.f)
		{
			switch (hitgroup)
			{
			case hitgroup_generic:
			case hitgroup_chest:
			case hitgroup_stomach:
			case hitgroup_leftarm:
			case hitgroup_rightarm:
				return true;
			case hitgroup_head:
				return target->has_helmet();
			default:
				break;
			}
		}

		return false;
	};

	switch (hitgroup)
	{
	case hitgroup_head:
		if (target->get_has_heavy_armor())
			damage = (damage * 4.f) * .5f;
		else
			damage *= 4.f;
		break;
	case hitgroup_stomach:
		damage *= 1.25f;
		break;
	case hitgroup_leftleg:
	case hitgroup_rightleg:
		damage *= .75f;
		break;
	default:
		break;
	}

	if (is_armored())
	{
		auto modifier = 1.f, armor_bonus_ratio = .5f, armor_ratio = weapon_armor_ratio * .5f;

		if (target->get_has_heavy_armor())
		{
			armor_bonus_ratio = .33f;
			armor_ratio = (weapon_armor_ratio * .5f) * .5f;
			modifier = .33f;
		}

		auto new_damage = damage * armor_ratio;

		if (target->get_has_heavy_armor())
			new_damage *= .85f;

		if ((damage - damage * armor_ratio) * (modifier * armor_bonus_ratio) > target->get_armor())
			new_damage = damage - target->get_armor() / armor_bonus_ratio;

		damage = new_damage;
	}

	return damage;
}

bool __declspec(noinline) custom_tracing::trace_to_studio_csgo_hitgroups_priority(cs_player_t *player,
																				  uint32_t contents_mask, vec3 *origin,
																				  game_trace *tr, ray *r,
																				  mat3x4 **mat) const
{
	// if the arguments are pointer, the linker crashes for some reason...
	const auto studio_model = player->get_studio_hdr()->hdr;
	const auto r_ = uintptr_t(r);
	const auto tr_ = uintptr_t(tr);
	const auto scale_ = player->get_model_scale();
	const auto origin_ = uintptr_t(origin);
	const auto mat_ = uintptr_t(mat);
	const auto set_ = uintptr_t(studio_model->get_hitbox_set(player->get_hitbox_set()));
	const auto fn_ = game->client.at(functions::trace_to_studio_csgo_hitgroups_priority);
	const auto chdr_ = uintptr_t(player->get_studio_hdr());
	auto rval = false;
	_asm
	{
			mov edx, r_
			push tr_
			push scale_
			push origin_
			push contents_mask
			push mat_
			push set_
			push chdr_
			mov eax, [fn_]
			call eax
			add esp, 0x1C
			mov rval, al
	}
	return rval;
}
} // namespace detail
