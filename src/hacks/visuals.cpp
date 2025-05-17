
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/event_log.h>
#include <detail/aim_helper.h>
#include <detail/custom_tracing.h>
#include <detail/events.h>
#include <detail/player_list.h>
#include <gui/gui.h>
#include <hacks/antiaim.h>
#include <hacks/esp.h>
#include <hacks/misc.h>
#include <hacks/skinchanger.h>
#include <hacks/visuals.h>
#include <sdk/client_entity_list.h>
#include <sdk/client_state.h>
#include <sdk/debug_overlay.h>
#include <sdk/game_event_manager.h>
#include <sdk/glow_manager.h>
#include <sdk/math.h>
#include <sdk/misc.h>
#include <sdk/render_view.h>
#include <sdk/surface.h>

using namespace detail;
using namespace detail::aim_helper;
using namespace sdk;
using namespace hacks::misc;

namespace hacks
{
void visuals::on_player_hurt(game_event *event)
{
	static const auto sound = XOR_STR_STORE("buttons\\arena_switch_press_02.wav");

	// no event info
	if (!event)
		return;

	// get involved players
	const auto target = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(
		game->engine_client->get_player_for_user_id(event->get_int(XOR_STR("userid")))));
	const auto attacker = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(
		game->engine_client->get_player_for_user_id(event->get_int(XOR_STR("attacker")))));

	// invalid or not our shot
	if (!attacker)
		return;
	if (!target)
		return;
	if (attacker->index() != game->engine_client->get_local_player())
		return;
	if (!target->is_enemy())
		return;
	hitmarker_color =
		(event->get_int(XOR_STR("health")) <= 0 && cfg.local_visuals.hitmarker_style->test(cfg_t::hitmarker_animated))
			? color::attention()
			: color::white();
	hitmarker_offset = 0.f;
	hitmarker_alpha = 255.f;

	if (cfg.local_visuals.hitsound.get())
	{
		XOR_STR_STACK(snd, sound);
		game->surface->play_sound(snd);
	}
}

void visuals::draw(const gui_legacy::draw_adapter &draw)
{
	output_eventlog(draw);
	hitmarker(draw);
	indicators(draw);
	scope(draw);
	pen_crosshair(draw);
}

void visuals::on_frame_render_start(const bool pre) const
{
	if (pre)
	{
		const auto player = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));
		if (!player || !hacks::skin_changer)
			return;
#if 1
		const auto wpn = reinterpret_cast<weapon_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
		const auto view_model = reinterpret_cast<weapon_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_view_model_0()));
		if (view_model && wpn)
		{
			game->mdl_cache->begin_lock();
			const auto weapon_entity = reinterpret_cast<weapon_t *>(
				game->client_entity_list->get_client_entity_from_handle(view_model->get_viewmodel_weapon()));
			if (weapon_entity && weapon_entity->is_knife())
			{
				const auto view_model_index = game->model_info_client->get_model_index(
					std::get<1>(hacks::skin_changer->models[size_t(weapon_entity->get_item_definition_index())])
						.c_str());
				view_model->set_model_index(view_model_index);
			}
			game->mdl_cache->end_lock();
		}
#endif
		return;
	}

	if (game->local_player && !game->input->camera_in_third_person)
		game->local_player->setup_bones(nullptr, max_bones, bone_used_by_anything, game->globals->curtime);

	if (cfg.removals.no_flash.get() && game->local_player)
		game->local_player->get_flash_max_alpha() = 0.f;

	if (cfg.removals.no_smoke.get())
		*reinterpret_cast<int32_t *>(game->client.at(globals::smoke_count)) = 0;

	if (cfg.local_visuals.thirdperson_dead.get())
	{
		const auto lp = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));
		if (lp && !lp->is_alive() && lp->get_observer_mode() > 2)
			lp->get_observer_mode() = 5;
	}
}

void visuals::on_post_data_update() const
{
	/*auto death_notice = find_hud_element(XOR_STR("CCSGO_HudDeathNotice"));

	if (death_notice && cfg.misc.preserve_killfeed.get())
	{
		const auto local_death_notice = reinterpret_cast<float *>(death_notice + 0x50);
		if (local_death_notice)
			*local_death_notice = game->local_player && game->local_player->is_alive() ? FLT_MAX : 1.5f;
	}*/

	// block fast path for entities that are owned by players and ragdolls.
#if 1
	game->client_entity_list->for_each(
		[](client_entity *const ent)
		{
			const auto e = reinterpret_cast<weapon_t *>(ent);
			if (!e->get_model())
				return;
			const auto owner = reinterpret_cast<entity_t *>(
				game->client_entity_list->get_client_entity_from_handle(e->get_owner_entity()));

			if ((e->is_weapon_world_model() && (owner && owner->is_player())) ||
				e->get_class_id() == class_id::csragdoll)
				e->get_can_use_fast_path() = false;

			if (e->is_weapon_world_model() && !owner && !e->get_custom_materials().count())
				e->get_can_use_fast_path() = true;
		});
#endif
}

void visuals::draw_glow()
{
	game->client_entity_list->for_each_player(
		[](cs_player_t *const player) -> void
		{
			if (cfg.local_visuals.glow_local.get() && player == game->local_player)
				return game->glow_manager->add_glow(player, cfg.local_visuals.local_color_glow.get());

			if (cfg.player_esp.glow_enemy.get() && player->is_enemy())
				return game->glow_manager->add_glow(player, cfg.player_esp.enemy_color_glow.get());

			if (cfg.player_esp.glow_team.get() && (!player->is_enemy() && player != game->local_player))
				return game->glow_manager->add_glow(player, cfg.player_esp.team_color_glow.get());
		});
}

void visuals::on_override_view(view_setup *const setup, bool pre_original)
{
	if (pre_original)
		third_person(setup);
}

void visuals::reset()
{
	hitmarker_color = color::white();
	hitmarker_alpha = 0.f;
	eventlog.reset();
}

void visuals::third_person(view_setup *const setup)
{
	const bool in_third_person = game->input->camera_in_third_person;

	linear_fade(third_person_dist, .35f, 1.f, 6.f, (cfg.local_visuals.thirdperson.get() || in_third_person));

	if (game->local_player)
	{
		const auto wep = (sdk::weapon_t *)game->client_entity_list->get_client_entity_from_handle(
			game->local_player->get_active_weapon());
		if (cfg.local_visuals.thirdperson.get() && !(cfg.local_visuals.thirdperson_nade && wep && wep->is_grenade()))
		{
			if (!in_third_person)
			{
				game->input->camera_in_third_person = true;
				DISPATCH_COMMAND("cl_updatevisibility");
			}

			auto player = game->local_player;

			if (!player->is_alive())
				player = reinterpret_cast<cs_player_t *>(
					game->client_entity_list->get_client_entity_from_handle(player->get_observer_target()));

			if (!player)
				return;

			auto adjusted_eye = aa.get_visual_eye_position();
			game->input->camera_offset = vec3(setup->angles.x, setup->angles.y,
											  cfg.local_visuals.thirdperson_distance.get() * third_person_dist);

			vec3 offset = game->input->camera_offset;
			vec3 forward;
			angle_vectors(vec3(offset.x, offset.y, 0.f), forward);

			const vec3 mins(-cam_hull_offset, -cam_hull_offset, -cam_hull_offset);
			const vec3 maxs(cam_hull_offset, cam_hull_offset, cam_hull_offset);

			game_trace trace{};
			trace_no_player_filter filter{};
			ray r;
			r.init(adjusted_eye, adjusted_eye - (forward * offset.z), mins, maxs);

			game->engine_trace->trace_ray(r, mask_solid, &filter, &trace);

			if (trace.fraction < 1.f)
				offset.z *= trace.fraction;

			game->input->camera_offset = offset;
			if (aa.is_visually_fakeducking())
				setup->origin.z = aa.get_visual_eye_position().z;
		}
		else if (in_third_person)
		{
			game->input->camera_in_third_person = false;
			third_person_dist = .35f;
		}
	}
}

void visuals::set_alpha_for_third_person(const model_render_info_t &info) const
{
	if (!cfg.local_visuals.alpha_blend.get())
		return;

	auto current = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(info.entity_index));

	// try to swap for owner.
	if (info.renderable)
	{
		const auto parent = info.renderable->get_shadow_parent();
		const auto unknown = parent != nullptr ? parent->get_client_unknown() : nullptr;
		const auto owner =
			unknown != nullptr ? reinterpret_cast<cs_player_t *const>(unknown->get_client_entity()) : nullptr;

		if (owner && owner->is_player())
			current = owner;
	}

	const auto view_setup = game->view_render->get_view_setup();
	if (!view_setup || !game->local_player || !game->local_player->is_alive() || !current || !current->is_player() ||
		!game->input->camera_in_third_person)
		return;

	const auto local_weapon = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (current == game->local_player &&
		(!local_weapon || !local_weapon->is_scoped_weapon() || !local_weapon->get_zoom_level()))
		return;

	// if we just rendered the glow, the override might still be active.
	game->model_render->forced_material_override(nullptr);

	// determine alpha scale.
	auto &cf = current == game->local_player
				   ? cfg.local_visuals.local_chams
				   : (current->is_enemy() ? cfg.player_esp.enemy_chams : cfg.player_esp.team_chams);
	auto alpha_scale =
		cfg.local_visuals.local_chams.mode.get().test(cfg_t::chams_disabled) ? 1.f : cf.primary.get().value.a;

	if (cfg.local_visuals.glow_local.get())
		alpha_scale *= 1.f - cfg.local_visuals.local_color_glow.get().value.a * .6f;

	if (current == game->local_player)
		game->render_view->set_blend(.25f * alpha_scale);
	else
	{
		constexpr auto cutoff_dist = 64.f;
		constexpr auto epsilon = .0001f;

		vec3 forward;
		angle_vectors(view_setup->angles, forward);
		const auto end = view_setup->origin + forward * game->input->camera_offset.z;
		const auto eye = current->get_eye_position();

		if ((eye - end).dot(forward) < 0.f)
		{
			const auto dist = distance_point_to_line(eye, view_setup->origin, forward);

			if (dist > epsilon && dist <= cutoff_dist)
				game->render_view->set_blend(dist / cutoff_dist * alpha_scale);
		}
	}
}

void visuals::indicators(const gui_legacy::draw_adapter &draw)
{
	if (!game->local_player || !game->local_player->is_alive() || !game->local_player->get_anim_state())
		return;

	// draw antiaim arrow
	if (!cfg.local_visuals.thirdperson.get())
	{
		const std::vector<vertex_t> verts{vec2(0, 0), vec2(-5, 15), vec2(5, 15)};

		const vec2 screen_size = vec2(evo::ren::draw.display.x, evo::ren::draw.display.y),
				   screen_center = screen_size / 2.f;
		const auto offset_x = 50.f;
		const auto offset_y = (screen_size.y / screen_size.x) * offset_x;

		if (cfg.local_visuals.aa_indicator.get().test(cfg_t::aa_real))
		{
			const auto rotation_real = std::remainderf(
				game->engine_client->get_view_angles().y - game->local_player->get_anim_state()->foot_yaw, yaw_bounds);
			const float rotation_rad_real = DEG2RAD(rotation_real);
			const auto pos_real = vec2(screen_center.x + offset_x * sin(rotation_rad_real),
									   screen_center.y - offset_y * cos(rotation_rad_real));
			draw.polygon(pos_real, verts, rotation_real, cfg.local_visuals.clr_aa_real.get());
		}

		if (cfg.local_visuals.aa_indicator.get().test(cfg_t::aa_desync))
		{
			const auto rotation_desync = std::remainderf(game->engine_client->get_view_angles().y -
															 local_fake_record.state[resolver_networked].foot_yaw,
														 yaw_bounds);
			const float rotation_rad_desync = DEG2RAD(rotation_desync);
			const auto pos_desync = vec2(screen_center.x + offset_x * sin(rotation_rad_desync),
										 screen_center.y - offset_y * cos(rotation_rad_desync));
			draw.polygon(pos_desync, verts, rotation_desync, cfg.local_visuals.clr_aa_desync.get());
		}
	}
}

void visuals::hitmarker(const gui_legacy::draw_adapter &draw)
{
	linear_fade(hitmarker_alpha, 0.f, 255.f,
				255.f / (cfg.local_visuals.hitmarker_style->test(cfg_t::hitmarker_animated) ? .4f : .8f), false);

	if (hitmarker_alpha <= FLT_EPSILON || !cfg.local_visuals.hitmarker.get())
		return;

	linear_fade(hitmarker_offset, 0.f, 1.f, 1.f / .4f, true);

	const auto center = vec2(evo::ren::draw.display.x, evo::ren::draw.display.y) / 2.f;
	const auto alpha = .9f * hitmarker_alpha / 255.f;
	const auto offset = 9.f * hitmarker_offset;

	if (cfg.local_visuals.hitmarker_style->test(cfg_t::hitmarker_animated))
	{
		draw.line(vec2(center.x - 9 - offset, center.y - 9 - offset),
				  vec2(center.x - 3 - offset, center.y - 3 - offset), hitmarker_color.alpha(alpha), 2.f);
		draw.line(vec2(center.x - 9 - offset, center.y + 9 + offset),
				  vec2(center.x - 3 - offset, center.y + 3 + offset), hitmarker_color.alpha(alpha), 2.f);
		draw.line(vec2(center.x + 9 + offset, center.y + 9 + offset),
				  vec2(center.x + 3 + offset, center.y + 3 + offset), hitmarker_color.alpha(alpha), 2.f);
		draw.line(vec2(center.x + 9 + offset, center.y - 9 - offset),
				  vec2(center.x + 3 + offset, center.y - 3 - offset), hitmarker_color.alpha(alpha), 2.f);
	}
	else
	{
		draw.line(vec2(center.x - 10, center.y - 10), vec2(center.x - 4, center.y - 4), hitmarker_color.alpha(alpha));
		draw.line(vec2(center.x - 10, center.y + 10), vec2(center.x - 4, center.y + 4), hitmarker_color.alpha(alpha));
		draw.line(vec2(center.x + 10, center.y + 10), vec2(center.x + 4, center.y + 4), hitmarker_color.alpha(alpha));
		draw.line(vec2(center.x + 10, center.y - 10), vec2(center.x + 4, center.y - 4), hitmarker_color.alpha(alpha));
	}
}

void visuals::scope(const gui_legacy::draw_adapter& draw) const
{
	if (cfg.removals.scope->test(cfg_t::scope_default) || !game->local_player || !game->local_player->get_is_scoped())
		return;

	const auto wpn = reinterpret_cast<weapon_t*>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	if (!wpn->is_scoped_weapon())
		return;
	//bool ez = wpn->is_scoped_weapon();
	if (wpn->get_zoom_level() == 0)
		return;

	if ((wpn->get_item_definition_index() == item_definition_index::weapon_sg556 ||
		 wpn->get_item_definition_index() == item_definition_index::weapon_aug) &&
		!game->input->camera_in_third_person)
		return;

	const auto spread =
		std::clamp((wpn->get_inaccuracy() - wpn->get_cs_weapon_data()->flinaccuracystandalt) * 5.f, 0.f, 1.f);
	const auto w = evo::ren::draw.display.x;
	const auto h = evo::ren::draw.display.y;
	auto width = float(GET_CONVAR_INT("cl_crosshair_sniper_width"));
	auto alpha = .8f;

	if (cfg.removals.scope->test(cfg_t::scope_dynamic))
	{
		width += 12.f * spread;
		alpha *= .2f + .8f * (1.f - spread);
		const auto clr = color::black().alpha(alpha);
		draw.rect_filled_fade(vec2(w / 2 - width / 2, 0), vec2(w / 2, h), clr.alpha(0.f), clr);
		draw.rect_filled_fade(vec2(w / 2, 0), vec2(width / 2 + w / 2, h), clr, clr.alpha(0.f));
		draw.rect_filled_fade(vec2(0, h / 2), vec2(w, width / 2 + h / 2), clr, clr.alpha(0.f), true);
		draw.rect_filled_fade(vec2(0, h / 2 - width / 2), vec2(w, h / 2), clr.alpha(0.f), clr, true);
	}
	else if (cfg.removals.scope->test(cfg_t::scope_small) || cfg.removals.scope->test(cfg_t::scope_small_dynamic))
	{
		const auto cfg_clr = cfg.removals.scope_color.get();

		const auto clr = color(cfg_clr).alpha(alpha);
		const auto offset = cfg.removals.scope->test(cfg_t::scope_small_dynamic) ? spread * 40 : 0;
		draw.rect_filled_fade(vec2(w / 2 - width / 2, h / 2 - 100 - offset),
							  vec2(w / 2 + width / 2, h / 2 - 10 - offset), clr.alpha(0.f), clr, true);
		draw.rect_filled_fade(vec2(w / 2 - width / 2, h / 2 + 10 + offset),
							  vec2(w / 2 + width / 2, h / 2 + 100 + offset), clr, clr.alpha(0.f), true);
		draw.rect_filled_fade(vec2(w / 2 - 100 - offset, h / 2 - width / 2),
							  vec2(w / 2 - 10 - offset, h / 2 + width / 2), clr.alpha(0.f), clr);
		draw.rect_filled_fade(vec2(w / 2 + 10 + offset, h / 2 - width / 2),
							  vec2(w / 2 + 100 + offset, h / 2 + width / 2), clr, clr.alpha(0.f));
	}
	else
	{
		const auto clr = color::black().alpha(alpha);
		draw.rect_filled(vec2(w / 2 - width / 2, 0), vec2(w / 2, h), clr);
		draw.rect_filled(vec2(w / 2, 0), vec2(width / 2 + w / 2, h), clr);
		draw.rect_filled(vec2(0, h / 2), vec2(w, width / 2 + h / 2), clr);
		draw.rect_filled(vec2(0, h / 2 - width / 2), vec2(w, h / 2), clr);
	}
}

void visuals::pen_crosshair(const gui_legacy::draw_adapter &draw) const
{
	if (!game->local_player || !game->local_player->is_alive() || !cfg.misc.penetration_crosshair.get())
		return;

	auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn || wpn->is_knife() || wpn->is_grenade() ||
		wpn->get_item_definition_index() == item_definition_index::weapon_taser)
		return;

	const auto center = vec2(evo::ren::draw.display.x, evo::ren::draw.display.y) / 2.f;
	draw.rect_filled(center - vec2(2, 2), center + vec2(2, 2),
					 trace.can_wallbang() ? color(0, 255, 0, 255).alpha(.7f) : color(255, 0, 0, 255).alpha(.7f));
	draw.rect(center - vec2(2, 2), center + vec2(2, 2), color(0, 0, 0, 255).alpha(.7f));
}

void visuals::output_eventlog(const gui_legacy::draw_adapter &draw) const
{
	if (!cfg.misc.eventlog.get().test(cfg_t::eventlog_top_left))
		return;

	auto &text_log = eventlog.get_log_text_only();

	if (text_log.empty())
		return;

	for (size_t i = 0; i < text_log.size(); ++i)
	{
		auto &log_line = text_log.at(i);

		const auto line_size = draw.get_text_size(std::get<0>(log_line).c_str());
		const auto fadeout_time = std::get<1>(log_line) + 3.f;
		const auto expire_time = fadeout_time + 2.f;

		if (fadeout_time <= game->globals->realtime && i == 0)
			linear_fade(std::get<2>(log_line), 0.f, 1.f, 1.f / .3f, false);

		if (expire_time <= game->globals->realtime || std::get<2>(log_line) <= .1f)
			text_log.erase(text_log.begin() + i);
		else
			draw.text(vec2(5, 5 + i * (line_size.y + 2)),
					  color::white().alpha(std::get<2>(log_line) * draw.get_alpha_modifier()),
					  std::get<0>(log_line).c_str());
	}
}

void visuals::on_bomb_beginplant(game_event *event)
{
	if (!cfg.indicators.indicators->test(cfg_t::ui_indicators_bomb) || !game->local_player ||
		!game->local_player->is_alive())
		return;

	bomb_info.plant_end = game->globals->curtime + sdk::plant_time;
	bomb_info.planter = (sdk::cs_player_t *)game->client_entity_list->get_client_entity(
		game->engine_client->get_player_for_user_id(event->get_int(XOR_STR("userid"))));

	const auto site = game->client_entity_list->get_client_entity(event->get_int(XOR_STR("site")));
	if (site && game->cs_player_resource->data && bomb_info.planter)
	{
		const auto origin =
			(bomb_info.planter == game->local_player || !bomb_info.planter->is_enemy(game->local_player))
				? game->local_player->get_abs_origin()
				: GET_PLAYER_ENTRY(bomb_info.planter).visuals.pos.target;
		const auto pos_a = game->cs_player_resource->data->get_bombsite_center_a();
		const auto pos_b = game->cs_player_resource->data->get_bombsite_center_b();

		bomb_info.plant_site = (origin - pos_a).length() < (origin - pos_b).length() ? site_a : site_b;
	}
}

void visuals::abort_bomb()
{
	bomb = nullptr;
	bomb_info.plant_site = site_none;
	bomb_info.planter = nullptr;
	bomb_info.plant_end = 0.f;
}

void visuals::on_bomb_planted()
{
	if (!cfg.indicators.indicators->test(cfg_t::ui_indicators_bomb))
		return;

	game->client_entity_list->for_each(
		[&](sdk::entity_t *ent)
		{
			if (ent->get_class_id() != class_id::planted_c4)
				return;

			bomb = (sdk::planted_c4_t *)ent;
		});
}
} // namespace hacks
