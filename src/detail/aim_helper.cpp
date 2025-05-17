
#include <base/debug_overlay.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <detail/dispatch_queue.h>
#include <detail/shot_tracker.h>
#include <hacks/antiaim.h>
#include <hacks/rage.h>
#include <hacks/tickbase.h>
#include <sdk/client_entity_list.h>
#include <sdk/cs_player.h>
#include <sdk/cs_player_resource.h>
#include <sdk/debug_overlay.h>
#include <sdk/surface_props.h>
#include <sdk/weapon.h>
#include <sdk/weapon_system.h>
#include <unordered_set>
#include <util/poly.h>

using namespace sdk;
using namespace hacks;

namespace detail
{
namespace aim_helper
{
static std::vector<seed> precomputed_seeds = {};

int32_t hitbox_to_hitgroup(cs_player_t::hitbox box)
{
	switch (box)
	{
	case cs_player_t::hitbox::head:
	case cs_player_t::hitbox::neck:
		return hitgroup_head;
	case cs_player_t::hitbox::upper_chest:
	case cs_player_t::hitbox::chest:
	case cs_player_t::hitbox::thorax:
		return hitgroup_chest;
	case cs_player_t::hitbox::body:
	case cs_player_t::hitbox::pelvis:
		return hitgroup_stomach;
	case cs_player_t::hitbox::left_forearm:
	case cs_player_t::hitbox::left_upper_arm:
	case cs_player_t::hitbox::left_hand:
		return hitgroup_leftarm;
	case cs_player_t::hitbox::right_forearm:
	case cs_player_t::hitbox::right_upper_arm:
	case cs_player_t::hitbox::right_hand:
		return hitgroup_rightarm;
	case cs_player_t::hitbox::left_calf:
	case cs_player_t::hitbox::left_foot:
	case cs_player_t::hitbox::left_thigh:
		return hitgroup_leftleg;
	case cs_player_t::hitbox::right_calf:
	case cs_player_t::hitbox::right_foot:
	case cs_player_t::hitbox::right_thigh:
		return hitgroup_rightleg;
	default:
		break;
	}

	return hitgroup_generic;
}

bool is_limbs_hitbox(cs_player_t::hitbox box) { return box >= cs_player_t::hitbox::right_thigh; }

bool is_body_hitbox(cs_player_t::hitbox box)
{
	return box == cs_player_t::hitbox::body || box == cs_player_t::hitbox::pelvis;
}

bool is_multipoint_hitbox(cs_player_t::hitbox box)
{
	if (!is_aim_hitbox_rage(box))
		return false;

	return box != cs_player_t::hitbox::left_thigh && box != cs_player_t::hitbox::right_thigh &&
		   box != cs_player_t::hitbox::left_foot && box != cs_player_t::hitbox::right_foot &&
		   box != cs_player_t::hitbox::body;
}

bool is_aim_hitbox_legit(cs_player_t::hitbox box)
{
	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.legit.hack.hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((hitgroup == hitgroup_leftarm || hitgroup == hitgroup_rightarm) &&
		cfg.legit.hack.hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && cfg.legit.hack.hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.legit.hack.hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return ((hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) &&
			cfg.legit.hack.hitbox.get().test(cfg_t::aim_legs));
}

bool is_aim_hitbox_rage(cs_player_t::hitbox box)
{
	if (GET_CONVAR_INT("mp_damage_headshot_only"))
		return box == cs_player_t::hitbox::head && cfg.rage.hack.hitbox.get().test(cfg_t::aim_head);

	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.rage.hack.hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((box == cs_player_t::hitbox::left_upper_arm || box == cs_player_t::hitbox::right_upper_arm) &&
		cfg.rage.hack.hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && box != cs_player_t::hitbox::chest && box != cs_player_t::hitbox::thorax &&
		cfg.rage.hack.hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.rage.hack.hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return (hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) && box != cs_player_t::hitbox::left_thigh &&
		   box != cs_player_t::hitbox::right_thigh && cfg.rage.hack.hitbox.get().test(cfg_t::aim_legs);
}

bool should_avoid_hitbox(cs_player_t::hitbox box)
{
	if (GET_CONVAR_INT("mp_damage_headshot_only"))
		return false;

	const auto hitgroup = hitbox_to_hitgroup(box);

	if (box == cs_player_t::hitbox::head && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_head))
		return true;

	if ((box == cs_player_t::hitbox::left_upper_arm || box == cs_player_t::hitbox::right_upper_arm) &&
		cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_arms))
		return true;

	if (hitgroup == hitgroup_chest && box != cs_player_t::hitbox::chest &&
		cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_upper_body))
		return true;

	if (hitgroup == hitgroup_stomach && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_lower_body))
		return true;

	return (hitgroup == hitgroup_leftleg || hitgroup == hitgroup_rightleg) && box != cs_player_t::hitbox::left_thigh &&
		   box != cs_player_t::hitbox::right_thigh && cfg.rage.hack.avoid_hitbox.get().test(cfg_t::aim_legs);
}

bool is_shooting(sdk::user_cmd *cmd)
{
	const auto weapon_handle = game->local_player->get_active_weapon();

	if (!weapon_handle)
		return false;

	const auto wpn =
		reinterpret_cast<weapon_t *>(game->client_entity_list->get_client_entity_from_handle(weapon_handle));

	if (!wpn)
		return false;

	const auto is_zeus = wpn->get_item_definition_index() == item_definition_index::weapon_taser;
	const auto is_secondary = (!is_zeus && wpn->get_weapon_type() == weapontype_knife) ||
							  wpn->get_item_definition_index() == item_definition_index::weapon_shield;

	auto shooting = false;

	if (wpn->is_grenade())
	{
		const auto &prev = pred.info[(cmd->command_number - 1) % input_max];
		shooting = prev.sequence == cmd->command_number - 1 && prev.throw_time > 0.f && wpn->get_throw_time() == 0.f;
	}
	else if (is_secondary)
		shooting = (pred.had_attack || pred.had_secondary_attack) && pred.can_shoot;
	else
		shooting = pred.had_attack && pred.can_shoot;

	return shooting;
}

float get_hitchance(cs_player_t *const player)
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto hc = cfg.rage.hack.hitchance_value.get() * .01f;

	if (wpn)
	{
		if (wpn->get_item_definition_index() == item_definition_index::weapon_taser)
			return game->local_player->is_on_ground() ? hc : hc / 2.f;
		if (wpn->get_item_definition_index() == item_definition_index::weapon_ssg08 &&
			!game->local_player->is_on_ground() && wpn->get_inaccuracy() < scout_air_accuracy)
			return 0.f;
	}

	if (hc < .01f)
		return 0.f;

	const auto &entry = GET_PLAYER_ENTRY(player);
	return std::clamp(hc + min(entry.spread_miss, 3) / float(wpn->get_cs_weapon_data()->maxclip1), 0.f, 1.f);
}

int32_t get_adjusted_health(cs_player_t *const player)
{
	return max(player->get_player_health() - shot_track.calculate_health_correction(player), 1);
}

float get_mindmg(cs_player_t *const player, const std::optional<cs_player_t::hitbox> hitbox, const bool approach)
{
	const auto &entry = GET_PLAYER_ENTRY(player);
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto info = wpn->get_cs_weapon_data();
	const auto target_min = cfg.rage.hack.min_dmg.get() / static_cast<float>(info->ibullets);

	if (!wpn || wpn->get_item_definition_index() == item_definition_index::weapon_taser ||
		!cfg.rage.hack.dynamic_min_dmg.get())
		return target_min;

	if (wpn->get_item_definition_index() == item_definition_index::weapon_shield)
		return 1.f;

	const auto hitgroup = hitbox_to_hitgroup(hitbox.value_or(cs_player_t::hitbox::pelvis));
	const auto wpn_dmg =
		max(trace.scale_damage(player, static_cast<float>(info->idamage), info->flarmorratio, hitgroup) - 1.f, 0);

	if (target_min > wpn_dmg)
		return wpn_dmg;

	if (!approach)
		return target_min;

	const auto target_time = std::clamp(entry.aimbot.target_time * 2.f, 0.f, .25f);
	return wpn_dmg - 4.f * target_time * (wpn_dmg - target_min);
}

std::vector<cs_player_t *> get_closest_targets()
{
	constexpr auto close_fov = 20.f;

	auto current_fov = FLT_MAX;
	std::vector<cs_player_t *> targets;

	if (game->local_player && game->local_player->is_alive())
		game->client_entity_list->for_each_player(
			[&targets, &current_fov](cs_player_t *const player)
			{
				if (!player->is_valid(true) || !player->is_enemy() || game->local_player == player)
					return;

				if (player->is_dormant() &&
					GET_PLAYER_ENTRY(player).visuals.last_update <
						TIME_TO_TICKS(game->engine_client->get_last_timestamp()) - TIME_TO_TICKS(5.f))
					return;

				const auto fov = get_fov_simple(game->engine_client->get_view_angles(),
												game->local_player->get_eye_position(), player->get_eye_position());

				if (current_fov > fov || fov < close_fov)
				{
					if (current_fov >= close_fov && fov < close_fov || fov >= close_fov)
						targets.clear();
					targets.push_back(player);
					current_fov = fov;
				}
			});

	return targets;
}

bool in_range(vec3 start, vec3 end)
{
	constexpr auto ray_extension = 12.f;

	ray r{};
	trace_no_player_filter filter{};

	const auto length = (end - start).length();
	const auto direction = (end - start).normalize();

	auto distance = 0.f, thickness = 0.f;
	auto penetrations = 5;

	while (distance <= length)
	{
		if (penetrations < 0)
			return false;

		distance += ray_extension;
		r.init(start + direction * distance, end);
		game_trace tr{};
		game->engine_trace->trace_ray(r, mask_shot_hull, &filter, &tr);

		if (tr.fraction == 1.f)
			return true;

		penetrations--;
		thickness += ray_extension;
		distance += (end - r.start).length() * tr.fraction + ray_extension;

		while (distance <= length)
		{
			auto check = start + direction * distance;
			if (!!(game->engine_trace->get_point_contents_world_only(check, mask_shot_hull) & mask_shot_hull))
				thickness += ray_extension;
			else
			{
				thickness = 0.f;
				break;
			}

			if (thickness >= 90.f)
				return false;

			distance += ray_extension;
		}
	}

	return true;
}

std::optional<sdk::vec3> get_hitbox_position(cs_player_t *const player, const cs_player_t::hitbox id,
											 const mat3x4 *const bones)
{
	const auto studio_model = player->get_studio_hdr()->hdr;

	if (studio_model)
	{
		const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(id), player->get_hitbox_set());

		if (hitbox)
		{
			auto tmp = angle_matrix(hitbox->rotation);
			tmp = concat_transforms(bones[hitbox->bone], tmp);
			vec3 min{}, max{};
			vector_transform(hitbox->bbmin, tmp, min);
			vector_transform(hitbox->bbmax, tmp, max);
			return (min + max) * .5f;
		}
	}

	return std::nullopt;
}

std::vector<rage_target> perform_hitscan(lag_record *record, const custom_tracing::bullet_safety minimal)
{
	std::vector<rage_target> targets;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn || !record->player->get_studio_hdr())
		return targets;

	const auto studio_model = record->player->get_studio_hdr()->hdr;

	std::array<mat3x4 *, 3> mats = {record->res_mat[record->res_direction], nullptr, nullptr};
	if (record->res_direction != resolver_max && record->res_direction != resolver_min)
	{
		mats[1] = record->res_mat[resolver_max];
		mats[2] = record->res_mat[resolver_min];
	}
	else
	{
		if (record->res_direction != resolver_max)
			mats[1] = record->res_mat[resolver_max];
		else
			mats[1] = record->res_mat[resolver_min];
	}
	const auto pinter = util::player_intersection(studio_model, record->player->get_hitbox_set(), mats);

	std::vector<dispatch_queue::fn> calls;
	calls.reserve(static_cast<size_t>(cs_player_t::hitbox::max));
	std::array<std::deque<rage_target>, static_cast<size_t>(cs_player_t::hitbox::max)> reductions;
	const auto start = game->local_player->get_eye_position();

	// determine multipoint adjustment.
	const auto spread_rad = wpn->get_spread() + get_lowest_inaccuray();
	const auto corner_rad = DEG2RAD(90.f - RAD2DEG(spread_rad));
	const auto dist = (record->origin - game->local_player->get_origin()).length();
	const auto spread_radius = (dist / sinf(corner_rad)) * sinf(spread_rad);

	// build run tasks.
	for (const auto &hitbox : cs_player_t::hitboxes)
	{
		if (!is_aim_hitbox_rage(hitbox))
			continue;

		calls.push_back(
			[&record, &start, &reductions, &hitbox, &minimal, &studio_model, &spread_radius, &pinter]()
			{
				auto &reduction = reductions[static_cast<size_t>(hitbox)];
				calculate_multipoint(reduction, record, hitbox, pinter, nullptr, spread_radius, minimal);

				for (auto target = reduction.begin(); target != reduction.end(); target = std::next(target))
				{
					target->pen = trace.wall_penetration(start, target->position, record, target->safety, std::nullopt,
														 nullptr, false, nullptr, target->hitgroup);
					if (target->pen.did_hit)
						target->position = target->pen.end;
				}
			});
	}

	// perform hitscan.
	dispatch.evaluate(calls);

	// perform reduction.
	for (const auto &reduction : reductions)
		std::copy(reduction.begin(), reduction.end(), std::back_inserter(targets));

	// report result.
	return targets;
}

void calculate_multipoint(std::deque<rage_target> &targets, lag_record *record, const cs_player_t::hitbox box,
						  const struct util::player_intersection &pinter, cs_player_t *override_player,
						  const float adjustment, custom_tracing::bullet_safety minimal)
{
	if (!record->player->get_studio_hdr())
		return;

	const auto studio_model = record->player->get_studio_hdr()->hdr;
	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(box), record->player->get_hitbox_set());
	if (!hitbox)
		return;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn)
		return;

	const auto info = wpn->get_cs_weapon_data();

	vec3 min{}, max{};
	vector_transform(hitbox->bbmin, record->mat[hitbox->bone], min);
	vector_transform(hitbox->bbmax, record->mat[hitbox->bone], max);
	auto center = (min + max) * .5f;

	const auto is_zeus = wpn->get_item_definition_index() == item_definition_index::weapon_taser;
	const auto is_knife = wpn->is_knife();
	const auto is_fake = record->player->is_fake_player();
	const auto hitgroup = hitbox->group;

	if (is_fake)
		minimal = custom_tracing::bullet_safety::none;

	auto rs = .975f;
	const auto eye = override_player ? override_player->get_eye_position() : game->local_player->get_eye_position();
	const auto ang = calc_angle(eye, center);
	vec3 forward;
	angle_vectors(ang, forward);

	if (!override_player)
	{
		if (!is_knife)
		{
			const auto dist_scaled_damage =
				info->idamage * powf(info->flrangemodifier, (center - eye).length() * .002f);
			if (trace.scale_damage(record->player, dist_scaled_damage, info->flarmorratio,
								   is_zeus ? hitgroup_generic : hitgroup) +
					2 <
				min(get_mindmg(record->player, box), get_adjusted_health(record->player)))
				return;
		}

		// scale it down when we are missing due to spread.
		auto &entry = GET_PLAYER_ENTRY(record->player);
		if (entry.spread_miss > 2)
			rs *= 1.f - (entry.spread_miss - 2) * .2f;

		// scale it down further when it's a limb.
		if (is_limbs_hitbox(box))
			rs *= .75f;
	}

	if (hitbox->radius == -1.f || is_knife)
	{
		auto point = center;
		if (wpn->is_knife())
		{
			const auto radius = hitbox->radius == -1.f ? 3.f : hitbox->radius;
			point += forward * radius * -rs;
		}
		else if (minimal != custom_tracing::bullet_safety::none)
			return;
		targets.emplace_back(point, record, false, center, box, hitgroup, 0.f);
		return;
	}

	rs *= .5f + .5f * cfg.rage.hack.multipoint.get() * .01f;
	if (!game->cl_lagcompensation || !game->cl_predict)
		rs *= .8f;

	rs -= adjustment * .1f;

	const auto safe_legs = record->velocity.length2d() < 1.1f || !(record->flags & cs_player_t::on_ground);

	thread_local static util::circular_buffer<vec3> projected_points[3][resolver_direction_max];
	thread_local static util::circular_buffer<vec3> mapped_points[3][resolver_direction_max];

	constexpr auto middle = .5f;
	constexpr auto dt = .125f;
	constexpr auto one_third = middle - dt;
	constexpr auto two_thirds = middle + dt;

	const auto n = forward;
	const auto u = forward.cross(vec3(0.f, 0.f, 1.f));
	const auto v = u.cross(n);
	for (auto i = 0; i < 3; i++)
	{
		const auto previous_matrix_scan = i == 1;
		const auto previous_record_scan = i == 2;

		if (i >= 1 && override_player)
			break;

		if (previous_matrix_scan && !record->has_previous_matrix)
			continue;

		if (previous_record_scan && !record->previous)
			break;

		const auto radius = hitbox->radius * .925f;

		for (auto j = 0; j < resolver_direction_max; j++)
		{
			if ((is_fake || override_player) && j != resolver_networked)
				continue;

			if (!(j == record->res_direction && !previous_record_scan ||
				  j == resolver_networked && !previous_record_scan && !previous_matrix_scan || j == resolver_center ||
				  j == resolver_min || j == resolver_max || j == resolver_min_min || j == resolver_max_max ||
				  j == resolver_min_extra && !previous_record_scan || j == resolver_max_extra && !previous_record_scan))
				continue;

			const auto &bone_transform = previous_record_scan
											 ? record->previous->res_mat[j][hitbox->bone]
											 : (previous_matrix_scan ? record->previous_res_mat[j][hitbox->bone]
																	 : record->res_mat[j][hitbox->bone]);

			vector_transform(hitbox->bbmin, bone_transform, min);
			vector_transform(hitbox->bbmax, bone_transform, max);

			auto &proj = projected_points[i][j];
			auto &map = mapped_points[i][j];

			proj.clear();
			proj.reserve(24);
			map.resize(24);

			const auto add_circle_points = [&proj, &min, &max](const vec3 &p)
			{
				*proj.push_front() = min + p;
				*proj.push_front() = max + p;
			};

			const auto right = u * radius;
			const auto top = vec3(0.f, 0.f, 1.f) * radius;
			const auto left = n.cross(vec3(0.f, 0.f, -1.f)) * radius;
			const auto bot = top * -1.f;

			add_circle_points(right);
			add_circle_points(top);
			add_circle_points(left);
			add_circle_points(bot);

			add_circle_points(interpolate(right, top, one_third).normalize() * radius);
			add_circle_points(interpolate(right, top, two_thirds).normalize() * radius);
			add_circle_points(interpolate(right, bot, one_third).normalize() * radius);
			add_circle_points(interpolate(right, bot, two_thirds).normalize() * radius);

			add_circle_points(interpolate(left, top, one_third).normalize() * radius);
			add_circle_points(interpolate(left, top, two_thirds).normalize() * radius);
			add_circle_points(interpolate(left, bot, one_third).normalize() * radius);
			add_circle_points(interpolate(left, bot, two_thirds).normalize() * radius);

			for (auto &p : proj)
				p -= n * (p - center).dot(n);

			const auto &p0 = projected_points[0][resolver_networked][0];
			for (auto k = 0; k < proj.size(); k++)
			{
				const auto &p = proj[k];
				const auto p_p0 = p - p0;
				map[k] = vec3(p_p0.dot(u), p_p0.dot(v), 0.f);
			}

			if (!override_player)
				util::monotone_chain(map);
		}
	}

	const auto point_to_world = [&](const vec3 &p)
	{
		const auto &p0 = projected_points[0][resolver_networked][0];
		const auto d = vec3(1.f, p.x, p.y);
		return vec3(vec3(p0.x, u.x, v.x).dot(d), vec3(p0.y, u.y, v.y).dot(d), vec3(p0.z, u.z, v.z).dot(d));
	};

	if (override_player)
	{
		auto &intersect = mapped_points[0][record->res_direction];

		constexpr auto epsilon = .01f;
		vec3 leftm, rightm, topm, botm;

		leftm = rightm = topm = botm = intersect.front();
		for (auto &p : intersect)
		{
			if (p.x < leftm.x - epsilon)
				leftm = p;
			else if (p.x > rightm.x + epsilon)
				rightm = p;

			if (p.y > topm.y + epsilon)
				topm = p;
			else if (p.y < botm.y - epsilon)
				botm = p;
		}

		leftm = point_to_world(leftm);
		rightm = point_to_world(rightm);

		targets.emplace_back(leftm, record, false, center, box, hitgroup, 0.f, (center - leftm).length_sqr(), 0.f,
							 custom_tracing::bullet_safety::none);
		targets.emplace_back(rightm, record, false, center, box, hitgroup, 0.f, (center - rightm).length_sqr(), 0.f,
							 custom_tracing::bullet_safety::none);

		if (box == cs_player_t::hitbox::head)
		{
			topm = point_to_world(topm);
			targets.emplace_back(topm, record, false, center, box, hitgroup, 0.f, (center - topm).length_sqr(), 0.f,
								 custom_tracing::bullet_safety::none);
		}

		return;
	}

	const auto check_hit = [&](const vec3 &pos, const bool scan_secure)
	{
		ray r{};
		r.init(eye, pos);

		const auto res = pinter.trace_hitgroup(r, scan_secure);
		return pinter.rank(res) == pinter.rank(hitbox->group);
	};

	const auto extra_head_calc =
		[&](vec3 &top, vec3 &bottom, vec3 &left, vec3 &right, const custom_tracing::bullet_safety safety)
	{
		if (box != cs_player_t::hitbox::head)
			return top;

		const auto move_point = [&](vec3 &p, const vec3 &to)
		{
			vec3 point{};
			for (auto i = 0; i <= 8; i++)
			{
				point = interpolate(p, to, i / 8.f);

				if (check_hit(point, safety > custom_tracing::bullet_safety::none))
				{
					p = point;
					break;
				}
			}
		};

		move_point(bottom, top);
		move_point(top, bottom);
		move_point(left, right);
		move_point(right, left);

		return (top + bottom + left + right) * .25f;
	};

	const auto point_to_center = [&](vec3 &p, const vec3 &center, const custom_tracing::bullet_safety safety)
	{
		const auto mp = p;

		auto i = 1u;
		for (; i <= 5; i++)
		{
			const auto point = interpolate(center, mp, i / 5.f);

			if (!check_hit(point, safety > custom_tracing::bullet_safety::none))
				break;

			p = point;
		}

		return i > 1u;
	};

	const auto extract_points_from_intersection =
		[&](util::convex_polygon &intersect, custom_tracing::bullet_safety safety)
	{
		constexpr auto epsilon = .01f;
		vec3 leftm, rightm, topm, botm;

		if (intersect.size() < 3)
			return false;

		leftm = rightm = topm = botm = intersect.front();
		for (auto &p : intersect)
		{
			if (p.x < leftm.x - epsilon)
				leftm = p;
			else if (p.x > rightm.x + epsilon)
				rightm = p;

			if (p.y > topm.y + epsilon)
				topm = p;
			else if (p.y < botm.y - epsilon)
				botm = p;
		}

		center = (leftm + rightm + topm + botm) * .25f;

		const auto target_rs = box == cs_player_t::hitbox::head ? max(.9f, rs) : rs;

		std::array<vec3, 3> intersection{point_to_world(interpolate(center, leftm, target_rs)),
										 point_to_world(interpolate(center, rightm, target_rs)),
										 box == cs_player_t::hitbox::head
											 ? point_to_world(interpolate(center, topm, target_rs))
											 : point_to_world(center)};

		auto bottom = point_to_world(interpolate(center, botm, target_rs));
		const auto new_center = extra_head_calc(intersection[2], bottom, intersection[0], intersection[1], safety);

		if (!check_hit(new_center, safety > custom_tracing::bullet_safety::none))
			return true;

		auto safety_size = safety != custom_tracing::bullet_safety::none ? util::area(intersect) : 0.f;
		if (box == cs_player_t::hitbox::head)
			safety_size *= .1f;
		else if (box == cs_player_t::hitbox::upper_chest)
			safety_size *= .2f;
		else if (box == cs_player_t::hitbox::left_calf || box == cs_player_t::hitbox::right_calf)
			safety_size *= .01f;

		if (rs < .2f || box == cs_player_t::hitbox::head)
			targets.emplace_back(new_center, record, false, new_center, box, hitgroup, 0.f, 0.f, safety_size,
								 is_fake ? custom_tracing::bullet_safety::full : safety);

		if (rs >= .2f)
			for (auto &p : intersection)
			{
				if (p != new_center)
				{
					if (box == cs_player_t::hitbox::head)
						p = interpolate(new_center, p, rs);

					if (!check_hit(new_center, safety > custom_tracing::bullet_safety::none) &&
						!point_to_center(p, new_center, safety))
						continue;
				}

				targets.emplace_back(p, record, false, new_center, box, hitgroup, 0.f, (new_center - p).length_sqr(),
									 safety_size, is_fake ? custom_tracing::bullet_safety::full : safety);
			}

		return true;
	};

	if (minimal == custom_tracing::bullet_safety::none)
	{
		auto intersect = mapped_points[0][record->res_direction];

		if (record->has_previous_matrix)
			intersect = util::get_intersection_poly(intersect, mapped_points[1][record->res_direction]);

		if (!extract_points_from_intersection(intersect, custom_tracing::bullet_safety::none))
			return;
	}

	if (is_fake || (is_limbs_hitbox(box) && !safe_legs))
		return;

	rs *= .8f;
	auto intersect = util::get_intersection_poly(mapped_points[0][resolver_min], mapped_points[0][resolver_max]);
	intersect = util::get_intersection_poly(intersect, mapped_points[0][resolver_center]);

	if (record->previous)
	{
		intersect = util::get_intersection_poly(intersect, mapped_points[2][resolver_min]);
		intersect = util::get_intersection_poly(intersect, mapped_points[2][resolver_max]);
		intersect = util::get_intersection_poly(intersect, mapped_points[2][resolver_center]);
	}

	if (record->has_previous_matrix)
	{
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_min]);
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_max]);
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_center]);
	}

	if (minimal != custom_tracing::bullet_safety::full &&
		!extract_points_from_intersection(intersect, custom_tracing::bullet_safety::no_roll))
		return;

	intersect = util::get_intersection_poly(intersect, mapped_points[0][resolver_min_min]);
	intersect = util::get_intersection_poly(intersect, mapped_points[0][resolver_max_max]);
	intersect = util::get_intersection_poly(intersect, mapped_points[0][resolver_min_extra]);
	intersect = util::get_intersection_poly(intersect, mapped_points[0][resolver_max_extra]);

	if (record->previous)
	{
		intersect = util::get_intersection_poly(intersect, mapped_points[2][resolver_min_min]);
		intersect = util::get_intersection_poly(intersect, mapped_points[2][resolver_max_max]);
	}

	if (record->has_previous_matrix)
	{
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_min_min]);
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_max_max]);
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_min_extra]);
		intersect = util::get_intersection_poly(intersect, mapped_points[1][resolver_max_extra]);
	}

	extract_points_from_intersection(intersect, custom_tracing::bullet_safety::full);
}

void optimize_cornerpoint(rage_target *target)
{
	if (target->dist < 1.f)
		return;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	const auto info = wpn->get_cs_weapon_data();
	const auto eye = game->local_player->get_eye_position();

	auto new_point = *target;
	const auto shots_to_kill = target->get_shots_to_kill();

	for (auto i = 0; i < 6; i++)
	{
		auto next_point = *target;
		next_point.position = interpolate(target->position, target->center, i / 6.f);
		next_point.pen = trace.wall_penetration(eye, next_point.position, target->record, target->safety, std::nullopt,
												nullptr, false, nullptr, target->hitgroup);
		if (!next_point.pen.did_hit || next_point.pen.damage < min(get_mindmg(target->record->player, target->hitbox),
																   get_adjusted_health(target->record->player)))
			continue;
		if (new_point.get_shots_to_kill() <= shots_to_kill)
			new_point = next_point;
	}

	if (new_point.position == target->position)
		return;

	new_point.position = interpolate(new_point.position, target->position, .5f);
	new_point.pen = trace.wall_penetration(eye, new_point.position, target->record, target->safety, std::nullopt,
										   nullptr, false, nullptr, target->hitgroup);
	if (!new_point.pen.did_hit || new_point.pen.damage < min(get_mindmg(target->record->player, target->hitbox),
															 get_adjusted_health(target->record->player)))
		return;
	if (new_point.get_shots_to_kill() <= shots_to_kill)
		*target = new_point;
}

float get_lowest_inaccuray()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto info = wpn->get_cs_weapon_data();

	if (game->local_player->get_flags() & cs_player_t::ducking)
		return (wpn->is_scoped_weapon() && (cfg.rage.hack.auto_scope.get() || wpn->get_zoom_level() != 0))
				   ? info->flinaccuracycrouchalt
				   : info->flinaccuracycrouch;

	return (wpn->is_scoped_weapon() && (cfg.rage.hack.auto_scope.get() || wpn->get_zoom_level() != 0))
			   ? info->flinaccuracystandalt
			   : info->flinaccuracystand;
}

bool has_full_accuracy()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	return wpn->get_inaccuracy() == 0.f || fabsf(wpn->get_inaccuracy() - get_lowest_inaccuray()) < .001f;
}

float calculate_hitchance(rage_target *target, bool full_accuracy)
{
	if (precomputed_seeds.empty())
		return 0.f;

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return 0.f;

	if (!target->record->player->get_studio_hdr())
		return 0.f;

	constexpr auto hg_equal_or_better = [](int32_t hitgroup, int32_t second)
	{
		if (hitgroup != second)
		{
			if (second == hitgroup_gear || second == hitgroup_generic)
				return false;
		}

		switch (hitgroup)
		{
		case hitgroup_head:
			if (second != hitgroup_head)
				return false;
			break;
		case hitgroup_neck:
		case hitgroup_chest:
			if (second > hitgroup_stomach && second != hitgroup_neck)
				return false;
			break;
		case hitgroup_stomach:
			if (second == hitgroup_chest || second > hitgroup_stomach)
				return false;
			break;
		case hitgroup_leftarm:
		case hitgroup_rightarm:
			if (second > hitgroup_rightarm && second != hitgroup_neck)
				return false;
			break;
		default:
			break;
		}

		return true;
	};

	const auto studio_model = target->record->player->get_studio_hdr()->hdr;

	const auto start = game->local_player->get_eye_position();
	const auto info = wpn->get_cs_weapon_data();
	const auto has_best_speed =
		game->local_player->get_velocity().length() <=
		(wpn->get_item_definition_index() == item_definition_index::weapon_revolver ? 180.f : wpn->get_max_speed()) /
				3.f -
			1.f;
	const auto true_accuracy = full_accuracy ? true : has_full_accuracy();
	const auto min_damage = get_mindmg(target->record->player, target->hitbox);
	const auto health = get_adjusted_health(target->record->player);

	// calculate inaccuracy and spread.
	const auto weapon_inaccuracy = full_accuracy ? get_lowest_inaccuray() : wpn->get_inaccuracy();
	const auto weapon_spread = info->flspread;

	// calculate angle.
	const auto aim_angle = calc_angle(start, target->position);
	vec3 forward, right, up;
	angle_vectors(aim_angle, forward, right, up);

	// setup calculation parameters.
	vec3 total_spread, spread_angle, end;
	float inaccuracy, spread, total_x, total_y;
	seed *s;

	// are we playing on a no-spread server?
	const auto is_spread = weapon_inaccuracy > 0.f;
	const auto total = is_spread ? total_seeds : 1;

	// setup all traces.
	std::vector<dispatch_queue::fn> calls;
	calls.reserve(total);
	std::vector<custom_tracing::wall_pen> traces(total);

	std::array<mat3x4 *, 3> mats = {target->record->res_mat[target->record->res_direction], nullptr, nullptr};
	if (target->record->res_direction != resolver_max && target->record->res_direction != resolver_min)
	{
		mats[1] = target->record->res_mat[resolver_max];
		mats[2] = target->record->res_mat[resolver_min];
	}
	else
	{
		if (target->record->res_direction != resolver_max)
			mats[1] = target->record->res_mat[resolver_max];
		else
			mats[1] = target->record->res_mat[resolver_min];
	}
	const auto pinter = util::player_intersection(studio_model, target->record->player->get_hitbox_set(), mats);
	const auto peek_exploit_ended = (tickbase.fast_fire || tickbase.hide_shot) &&
									game->local_player->get_tick_base() >= tickbase.lag.first - 2 &&
									!aa.is_lag_hittable();
	const auto is_zeus = wpn->get_item_definition_index() == item_definition_index::weapon_taser;
	const auto ignore_safety = has_best_speed || true_accuracy || is_zeus || peek_exploit_ended;

	if (is_spread)
		for (auto i = 0; i < total; i++)
		{
			// get seed.
			s = &precomputed_seeds[i];

			// calculate spread.
			inaccuracy = s->inaccuracy;
			spread = s->spread;

			// correct spread value for different weapons.
			if (wpn->get_item_definition_index() == item_definition_index::weapon_negev &&
				wpn->get_recoil_index() < 3.f)
				for (auto j = 3; j > static_cast<int32_t>(wpn->get_recoil_index()); j--)
					inaccuracy *= inaccuracy;

			inaccuracy *= weapon_inaccuracy;
			spread *= weapon_spread;
			total_x = (s->cos_inaccuracy * inaccuracy) + (s->cos_spread * spread);
			total_y = (s->sin_inaccuracy * inaccuracy) + (s->sin_spread * spread);
			total_spread = (forward + right * total_x + up * total_y).normalize();

			// calculate angle with spread applied.
			vector_angles(total_spread, spread_angle);

			// calculate end point of trace.
			angle_vectors(spread_angle, end);
			end = start + end.normalize() * info->range;

			// insert call instruction.
			calls.push_back(
				[&traces, &start, end, &target, i, is_zeus, &pinter, ignore_safety, &hg_equal_or_better]()
				{
					if (is_zeus)
					{
						ray r;
						r.init(start, end);

						if (pinter.trace_hitgroup(r, target->safety > custom_tracing::bullet_safety::none) ==
							hitgroup_gear)
							return;
					}

					if (!ignore_safety)
					{
						ray r;
						r.init(start, end);

						if (!hg_equal_or_better(
								target->hitgroup,
								pinter.trace_hitgroup(r, target->safety > custom_tracing::bullet_safety::none)))
							return;
					}

					traces[i] = trace.wall_penetration(start, end, target->record, custom_tracing::bullet_safety::none,
													   std::nullopt, nullptr, false, nullptr);
				});
		}
	else
		traces[0] = trace.wall_penetration(start, target->position, target->record, custom_tracing::bullet_safety::none,
										   std::nullopt, nullptr, false, nullptr);

	// dispatch traces to tracing pool.
	dispatch.evaluate(calls);

	// calculate statistics.
	auto total_hits = 0;

	for (const auto &tr : traces)
	{
		if (!tr.did_hit)
			continue;

		if (is_zeus)
		{
			if (tr.damage >= 100.f)
				total_hits++;

			continue;
		}

		if (!true_accuracy && !hg_equal_or_better(target->hitgroup, tr.hitgroup))
			continue;

		if (true_accuracy && tr.impact_count == 1 || tr.damage >= min_damage || tr.damage >= health)
			total_hits++;
	}

	// return result.
	return min(static_cast<float>(total_hits * info->ibullets) / static_cast<float>(total), 1.f);
}

bool should_prefer_record(std::optional<rage_target> &target, std::optional<rage_target> &alternative, bool compare_hc)
{
	static constexpr auto max_body_volume = 1600.f;

	if (alternative.has_value() && alternative->pen.did_hit && (!target.has_value() || !target->pen.did_hit))
	{
		alternative->best_to_kill = alternative->get_shots_to_kill();
		return true;
	}

	if (!alternative.has_value() || !alternative->pen.did_hit || !target.has_value() || !target->pen.did_hit)
		return false;

	const auto to_kill = alternative->get_shots_to_kill();
	const auto old_to_kill = target->get_shots_to_kill();

	if (alternative->pen.damage + health_buffer < target->pen.damage && old_to_kill > 2)
		return false;

	if (to_kill >= 2 * target->best_to_kill)
		return false;

	if (alternative->safety_size < target->safety_size && old_to_kill <= to_kill &&
		alternative->pen.safety == target->pen.safety)
		return false;

	if (old_to_kill <= to_kill && alternative->pen.safety < target->pen.safety)
		return false;

	const auto target_hdr = target->record->player->get_studio_hdr()->hdr;
	const auto alternative_hdr = alternative->record->player->get_studio_hdr()->hdr;
	const auto alternative_hitbox = alternative_hdr->get_hitbox(static_cast<uint32_t>(alternative->pen.hitbox), 0);
	const auto target_hitbox = target_hdr->get_hitbox(static_cast<uint32_t>(target->pen.hitbox), 0);

	if (to_kill == old_to_kill && target_hitbox->radius == -1.f && alternative_hitbox->radius != -1.f)
	{
		alternative->best_to_kill = to_kill;
		return true;
	}

	if (compare_hc && alternative->hc > target->hc)
	{
		alternative->best_to_kill = to_kill;
		return true;
	}

	if (fabsf(alternative->safety_size - target->safety_size) <= 5.f && old_to_kill <= to_kill &&
		alternative_hitbox->radius != -1.f && target_hitbox->radius != -1.f)
	{
		const auto alternative_volume =
			min(max_body_volume, static_cast<float>(sdk::pi) * alternative_hitbox->radius * alternative_hitbox->radius *
									 (4.f / 3.f * alternative_hitbox->radius +
									  (alternative_hitbox->bbmax - alternative_hitbox->bbmin).length()));
		const auto target_volume =
			min(max_body_volume,
				static_cast<float>(sdk::pi) * target_hitbox->radius * target_hitbox->radius *
					(4.f / 3.f * target_hitbox->radius + (target_hitbox->bbmax - target_hitbox->bbmin).length()));

		if (alternative_volume < target_volume - 2.f)
			return false;

		if (fabsf(alternative_volume - target_volume) < 2.f)
		{
			if (alternative->hitbox > target->hitbox)
				return false;
			if (alternative->dist > target->dist + .3f)
				return false;
		}
	}

	if (to_kill == old_to_kill && alternative->pen.safety == target->pen.safety &&
		(!compare_hc || alternative->hc == target->hc) && alternative->record->recv_time < target->record->recv_time)
		return false;

	alternative->best_to_kill = min(to_kill, target->best_to_kill);
	return true;
}

bool is_attackable()
{
	if (rag.has_priority_targets())
		return true;

	auto found = false;

	game->client_entity_list->for_each_player(
		[&](cs_player_t *const player)
		{
			if (!player->is_valid() || !player->is_alive() || !player->is_enemy())
				return;

			const auto &entry = GET_PLAYER_ENTRY(player);

			if (entry.hittable || entry.danger)
				found = true;
		});

	return found;
}

bool holds_tick_base_weapon()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return false;

	return wpn->get_item_definition_index() != item_definition_index::weapon_taser &&
		   wpn->get_item_definition_index() != item_definition_index::weapon_fists &&
		   wpn->get_item_definition_index() != item_definition_index::weapon_c4 && !wpn->is_grenade() &&
		   wpn->get_class_id() != class_id::snowball && wpn->get_weapon_type() != weapontype_unknown;
}

void fix_movement(user_cmd *const cmd, angle &wishangle)
{
	vec3 view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	angle_vectors(wishangle, view_fwd, view_right, view_up);
	angle_vectors(cmd->viewangles, cmd_fwd, cmd_right, cmd_up);

	view_fwd.z = view_right.z = cmd_fwd.z = cmd_right.z = 0.f;
	view_fwd.normalize();
	view_right.normalize();
	cmd_fwd.normalize();
	cmd_right.normalize();

	const auto dir = view_fwd * cmd->forwardmove + view_right * cmd->sidemove;
	const auto denom = cmd_right.y * cmd_fwd.x - cmd_right.x * cmd_fwd.y;

	cmd->sidemove = (cmd_fwd.x * dir.y - cmd_fwd.y * dir.x) / denom;
	cmd->forwardmove = (cmd_right.y * dir.x - cmd_right.x * dir.y) / denom;

	normalize_move(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	wishangle = cmd->viewangles;
}

void slow_movement(user_cmd *const cmd, float target_speed)
{
	const auto factor = game->local_player->get_duck_amount() * .34f + 1.f - game->local_player->get_duck_amount();

	if ((target_speed == 0.f || (pred.original_cmd.forwardmove == 0.f && pred.original_cmd.sidemove == 0.f)) &&
		pred.info[(cmd->command_number - 1) % input_max].velocity.length() < 1.1f)
	{
		if (aa.is_active() && !cfg.antiaim.foot_yaw.get().test(cfg_t::foot_yaw_none) &&
			game->local_player->is_on_ground())
		{
			cmd->forwardmove = (cmd->command_number % 2 ? -1.01f : 1.01f) / factor;
			cmd->sidemove = 0;
		}
		return;
	}

	pred.repredict(&game->input->commands[(cmd->command_number - 1) % input_max]);
	auto data = game->cs_game_movement->setup_move(game->local_player, cmd);
	slow_movement(&data, target_speed);
	std::tie(cmd->forwardmove, cmd->sidemove) = std::tie(data.forward_move, data.side_move);
	cs_game_movement_t::walk_move(&data, game->local_player);

	if (aa.is_active() && !cfg.antiaim.foot_yaw.get().test(cfg_t::foot_yaw_none) &&
		game->local_player->is_on_ground() && data.velocity.length() < 1.1f &&
		game->local_player->get_velocity().length() < 1.1f)
	{
		cmd->forwardmove = (cmd->command_number % 2 ? -1.01f : 1.01f);
		cmd->sidemove = 0;
	}

	cmd->forwardmove /= factor;
	cmd->sidemove /= factor;
	normalize_move(cmd->forwardmove, cmd->sidemove, cmd->upmove);
	pred.repredict(cmd);
}

void slow_movement(move_data *const data, float target_speed)
{
	const auto player =
		reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity_from_handle(data->player_handle));

	const auto on_ground = player->get_ground_entity() != -1;

	cs_game_movement_t::check_parameters(data, player);
	if (on_ground)
		cs_game_movement_t::friction(data, player);

	auto data2 = *data;
	if (on_ground)
		cs_game_movement_t::walk_move(&data2, player);
	else
		cs_game_movement_t::air_move(player, &data2);

	if (data2.velocity.length2d() <= target_speed)
		return;

	if (data->velocity.length2d() > target_speed)
	{
		vec3 ang;
		vector_angles(data->velocity * -1.f, ang);
		ang.y = std::remainderf(data->view_angles.y - ang.y, yaw_bounds);

		vec3 forward;
		angle_vectors(ang, forward);
		forward.z = 0.f;
		forward.normalize();

		auto target_accel = data->velocity.length2d() - target_speed;

		if (!on_ground)
		{
			const auto accel_speed = GET_CONVAR_FLOAT("sv_airaccelerate") * game->globals->interval_per_tick *
									 player->get_surface_friction();
			if (accel_speed < 1.f)
				target_accel /= accel_speed;
		}

		const auto len = min(target_accel, data->max_speed);
		data->forward_move = forward.x * len;
		data->side_move = forward.y * len;
		data->up_move = 0.f;
	}
	else
	{
		vec3 forward, right, up;
		angle_vectors(data->view_angles, forward, right, up);
		forward.z = right.z = 0.f;
		forward.normalize();
		right.normalize();

		vec3 wishdir(forward.x * data->forward_move + right.x * data->side_move,
					 forward.y * data->forward_move + right.y * data->side_move, 0.f);
		wishdir.normalize();

		const auto current_speed = data->velocity.dot(wishdir);
		const auto target_accel = target_speed - data->velocity.length2d();
		const auto target_move = min(current_speed + target_accel, data->max_speed);

		const auto squared = (data->forward_move * data->forward_move) + (data->side_move * data->side_move) +
							 (data->up_move * data->up_move);
		const auto ratio = target_move / sqrtf(squared);

		if (ratio < 1.f)
		{
			data->forward_move *= ratio;
			data->side_move *= ratio;
			data->up_move *= ratio;
		}
	}
}

void build_seed_table()
{
	if (precomputed_seeds.empty())
		for (auto i = 0; i < total_seeds; i++)
		{
			random_seed(i + 1);

			seed a;
			a.inaccuracy = random_float(0.f, 1.f);
			auto p = random_float(0.f, 2.f * sdk::pi);
			a.sin_inaccuracy = sin(p);
			a.cos_inaccuracy = cos(p);
			a.spread = random_float(0.f, 1.f);
			p = random_float(0.f, 2.f * sdk::pi);
			a.sin_spread = sin(p);
			a.cos_spread = cos(p);

			precomputed_seeds.push_back(a);
		}
}

inline int32_t rage_target::get_shots_to_kill()
{
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	const auto health = get_adjusted_health(record->player);
	auto shots = static_cast<int32_t>(ceilf(health / fminf(pen.damage, health)));

	if (tickbase.fast_fire && holds_tick_base_weapon() && cfg.rage.hack.secure_fast_fire.get() &&
		tickbase.compute_current_limit() > 0)
		shots -= static_cast<int32_t>(
			floorf(TICKS_TO_TIME(tickbase.compute_current_limit()) / (.75f * wpn->get_cs_weapon_data()->cycle_time)));

	return max(1, shots);
}

bool is_visible(const vec3 &pos)
{
	if (!game->local_player || game->local_player->get_flash_bang_time() > game->globals->curtime)
		return false;

	const auto eye = game->local_player->get_eye_position();
	// TODO:
	// static const auto line_goes_through_smoke = PATTERN(uint32_t, "client.dll", "55 8B EC 83 EC 08 8B 15 ? ? ? ?
	// 0F"); if (cfg.legit.hack.smokecheck.get() && reinterpret_cast<bool(__cdecl*)(vec3,
	// vec3)>(line_goes_through_smoke())(eye, pos)) 	return false;

	trace_no_player_filter filter{};
	game_trace t;
	ray r;
	r.init(eye, pos);
	game->engine_trace->trace_ray(r, mask_visible | contents_blocklos, &filter, &t);
	return t.fraction == 1.f;
}

std::pair<float, float> perform_freestanding(const vec3 &from, std::vector<sdk::vec3> tos, bool *previous)
{
	if (tos.empty())
		INVALID_ARGUMENT("Target vector was empty");

	// setup data for tracing.
	const auto info = *game->weapon_system->get_weapon_data(item_definition_index::weapon_awp);
	float real{}, fake{}, current_back{};
	auto is_forced = false;

	for (auto &to : tos)
	{
		// calculate distance and angle to target.
		const auto at_target = calc_angle(from, to);
		const auto dist = (to - from).length();

		// directions of local player.
		const auto direction_left = std::remainderf(at_target.y - 90.f, yaw_bounds);
		const auto direction_right = std::remainderf(at_target.y + 90.f, yaw_bounds);
		const auto direction_back = std::remainderf(at_target.y + 180.f, yaw_bounds);

		// calculate scan positions.
		const auto local_left = rotate_2d(from, direction_left, freestand_width);
		const auto local_right = rotate_2d(from, direction_right, freestand_width);
		const auto local_half_left = rotate_2d(from, direction_left, .5f * freestand_width);
		const auto local_half_right = rotate_2d(from, direction_right, .5f * freestand_width);
		const auto enemy_left = rotate_2d(to, direction_left, freestand_width);
		const auto enemy_right = rotate_2d(to, direction_right, freestand_width);

		// comperator function.
		const auto compare = [&](const vec3 &fleft, const vec3 &fright, const vec3 &l, const vec3 &r,
								 const bool check = false, const bool check2 = false) -> float
		{
			auto cinfo = info;
			if (check)
				cinfo.idamage = 200;

			const auto res_left = trace.wall_penetration(fleft, l, nullptr, custom_tracing::bullet_safety::none,
														 std::nullopt, nullptr, false, &cinfo);
			if (check && res_left.damage > 0 && res_left.impact_count > 0 &&
				(res_left.impacts.front() - fleft).length() < (res_left.impacts.front() - l).length())
				return FLT_MAX;

			if (check2)
				return res_left.damage > 0 ? FLT_MAX : direction_back;

			const auto res_right = trace.wall_penetration(fright, r, nullptr, custom_tracing::bullet_safety::none,
														  std::nullopt, nullptr, false, &cinfo);
			if (check)
			{
				if (res_right.damage > 0 && res_right.impact_count > 0 &&
					(res_right.impacts.front() - fright).length() < (res_right.impacts.front() - r).length())
					return FLT_MAX;
			}
			else
			{
				if (res_right.damage && !res_left.damage)
					return direction_left;
				if (!res_right.damage && res_left.damage)
					return direction_right;
				if (res_right.damage && res_left.damage)
					return FLT_MAX;
			}

			return direction_back;
		};

		if (!is_forced)
		{
			if (const auto r1 = compare(to, to, local_left, local_right); r1 != direction_back)
			{
				if (r1 != FLT_MAX)
				{
					real = fake = r1;
					current_back = direction_back;
				}
				else
					real = fake = current_back = direction_back;

				is_forced = true;
			}
			else if (const auto r2 = compare(enemy_left, enemy_right, local_left, local_right); r2 != direction_back)
			{
				if (r2 != FLT_MAX)
				{
					real = fake = r2;
					current_back = direction_back;
				}
				else
					real = fake = current_back = direction_back;

				is_forced = true;
			}
		}

		if (real != direction_back && compare(to, {}, real == direction_left ? local_half_left : local_half_right, {},
											  false, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}
		if (real != direction_back &&
			compare(real == direction_left ? enemy_right : enemy_left, {},
					real == direction_left ? local_half_left : local_half_right, {}, false, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}
		if (real != direction_back &&
			compare(enemy_left, enemy_right, local_half_right, local_half_left, true) != direction_back)
		{
			real = direction_back;
			is_forced = true;
		}

		if (previous)
		{
			if (real == direction_back)
				real = *previous ? direction_left : direction_right;
			*previous = real == direction_left;
			break;
		}
	}

	const auto force_back = real != fake;
	if (!previous && (force_back || !is_forced))
	{
		vec3 sum;
		for (auto &to : tos)
			sum += to;
		sum /= tos.size();
		real = calc_angle(sum, from).y;
	}

	if (!force_back && cfg.antiaim.yaw.get().test(cfg_t::yaw_freestanding))
		fake = current_back;

	return std::make_pair(real, fake);
}
} // namespace aim_helper
} // namespace detail
