
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <detail/player_list.h>
#include <gui/gui.h>
#include <hacks/esp.h>
#include <hacks/grenade_prediction.h>
#include <hacks/rage.h>
#include <sdk/client_entity_list.h>
#include <sdk/cs_player_resource.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_client.h>
#include <sdk/engine_trace.h>
#include <sdk/math.h>
#include <sdk/misc.h>
#include <sdk/model_render.h>
#include <sdk/weapon_system.h>
#include <util/misc.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#include <lua/helpers.h>
#endif

using namespace sdk;
using namespace detail;
using namespace detail::aim_helper;

namespace hacks
{
void esp::draw(const gui_legacy::draw_adapter &draw)
{
	closest_entity.fill({});

	game->client_entity_list->for_each(
		[&](entity_t *ent)
		{
			if (!ent->is_entity())
				return;

			if (ent->is_base_combat_weapon() && game->client_entity_list->get_client_entity_from_handle(
													reinterpret_cast<weapon_t *>(ent)->get_owner_entity()))
				return;

			auto &entry = closest_entity[size_t(ent->get_class_id())];
			const auto dist = (game->view_render && game->view_render->get_view_setup())
								  ? (game->view_render->get_view_setup()->origin - ent->get_abs_origin()).length()
								  : 0.f; // how can this possibly fail?

			if (!entry.first || dist <= entry.second)
				entry = {ent, dist};
		});

	game->client_entity_list->for_each_z(
		[&](entity_t *const entity)
		{
			draw_entity(draw, entity);
			draw.set_alpha_modifier(1.f);
		});
}

void esp::draw_player(const gui_legacy::draw_adapter &draw, cs_player_t *const player) const
{
	auto observer_target = reinterpret_cast<cs_player_t *const>(
		game->client_entity_list->get_client_entity(game->engine_client->get_local_player()));

	if (observer_target && (player->get_observer_mode() == 4 || player->get_observer_mode() == 5))
		observer_target = reinterpret_cast<cs_player_t *const>(
			game->client_entity_list->get_client_entity_from_handle(observer_target->get_observer_target()));

	if (player == observer_target || player->index() == game->engine_client->get_local_player())
		return;

	if (!player->is_alive() && !player->is_dormant() &&
		(player->get_observer_mode() == 4 || player->get_observer_mode() == 5))
	{
		observer_target = reinterpret_cast<cs_player_t *const>(
			game->client_entity_list->get_client_entity_from_handle(player->get_observer_target()));
		if (observer_target && observer_target->is_player() && observer_target->is_dormant())
		{
			const auto origin = player->get_origin();
			auto &vis = GET_PLAYER_ENTRY(observer_target).visuals;
			if ((origin - vis.pos.current).length() <= 512.f)
				vis.pos.update(origin, .05f);
			else
				vis.pos.current = vis.pos.target = origin;
			vis.last_update = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
			vis.alpha = .3f;
		}
	}

	if (player->is_dormant() && cfg.player_esp.legit_esp.get())
		return;

	auto &player_entry = GET_PLAYER_ENTRY(player);
	auto &alpha = player_entry.visuals.alpha;
	if (!player->is_alive() && !player->is_dormant())
	{
		alpha = player_entry.visuals.dormant_alpha = 0.f;
		return;
	}

	if (!player->is_dormant())
	{
		player_entry.visuals.pos.current = player_entry.visuals.pos.target = player->get_abs_origin();
		player_entry.visuals.last_update = TIME_TO_TICKS(game->engine_client->get_last_timestamp());
	}

	if (!cfg.player_esp.disable_dormancy_interp)
		player_entry.visuals.pos.advance(.5f * game->globals->frametime);
	else
		player_entry.visuals.pos.current = player_entry.visuals.pos.target;

	if (player->is_dormant() && alpha == 0.f)
	{
		player_entry.visuals.dormant_alpha = 0.f;
		return;
	}

	linear_fade(player_entry.visuals.dormant_alpha, 0.f, 1.f, 2.f, !player->is_dormant());

	linear_fade(alpha,
				player_entry.visuals.last_update <
						TIME_TO_TICKS(game->engine_client->get_last_timestamp()) - TIME_TO_TICKS(5.f)
					? 0.f
					: .3f,
				.7f, 2.f, !player->is_dormant());

	auto &cf = player->is_enemy() ? cfg.player_esp.enemy : cfg.player_esp.team;

	if (!cf.get().first_set_bit() || cf->selected_bits() == 1 && *cf->first_set_bit() == cfg_t::esp_software)
		return;

	const auto visible = is_visible(player->get_eye_position());
	const auto &clr =
		(cfg.player_esp.vis_check.get() && visible)
			? (player->is_enemy() ? cfg.player_esp.enemy_color_visible.get() : cfg.player_esp.team_color_visible.get())
			: (player->is_enemy() ? cfg.player_esp.enemy_color.get() : cfg.player_esp.team_color.get());
	const auto clr_a = clr.value.a;
	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));
	const auto box = get_player_bounds(player);

	// do this prior to offscreen arrows as they utilize some of this data.
	const auto health_bar =
		std::make_pair(vec2(box->first.x - 6, box->first.y - 1), vec2(box->first.x - 2, box->second.y + 1));
	const auto current_health = std::clamp(player->get_health(), 0, 100) / 100.f;
	linear_fade(player_entry.visuals.previous_health, current_health, 1.f, 1.5f, false);

	const auto armor_bar =
		std::make_pair(vec2(box->second.x + 2, box->first.y - 1), vec2(box->second.x + 6, box->second.y + 1));
	const auto current_armor = std::clamp(player->get_armor(), 0, 100) / 100.f;
	linear_fade(player_entry.visuals.previous_armor, current_armor, 1.f, 1.5f, false);

	if (!box.has_value())
		return draw_player_offscreen(draw, player);

	if (cfg.player_esp.legit_esp.get())
	{
		bool should_draw = false;

		if (visible && cfg.player_esp.legit_triggers.get().test(cfg_t::legit_visible))
			should_draw = true;
		else if (player->get_spotted() && cfg.player_esp.legit_triggers.get().test(cfg_t::legit_spotted))
			should_draw = true;

		if (!should_draw)
			return;
	}

	draw.set_alpha_modifier(alpha);
	const auto fractional = (alpha - .3f) * 2.5f;

	if (player->is_enemy() ? cfg.player_esp.enemy_skeleton.get() : cfg.player_esp.team_skeleton.get())
		draw_bones(draw, player,
				   saturate(player->is_enemy() ? cfg.player_esp.enemy_skeleton_color.get()
											   : cfg.player_esp.team_skeleton_color.get(),
							fractional));

	const auto ammo_bar =
		std::make_pair(vec2(box->first.x - 1, box->second.y + 2), vec2(box->second.x + 1, box->second.y + 6));

	esp_builder bld;

	if (cf->test(cfg_t::esp_box))
		draw_box(draw, box.value(), saturate(clr, fractional).alpha(clr_a));

	if (cf->test(cfg_t::esp_text))
		bld.add(esp_side_top, esp_item::text(get_player_name(player), color::white()));

	if (cf->test(cfg_t::esp_health_bar))
	{
		bld.add(esp_side_left,
				esp_item::bar(current_health, player_entry.visuals.previous_health,
							  (cf->test(cfg_t::esp_health_text) && current_health <= .99f)
								  ? std::to_string(player->get_health())
								  : XOR_STR(""),
							  interpolate(color::health_end(), color::health_start(), current_health, fractional)));
	}

	if (cf->test(cfg_t::esp_distance))
	{
		const auto dist = calculate_distance_to_object(player);
		if (dist > 0)
			bld.add(esp_side_left, esp_item::text(tfm::format(XOR_STR("%d YD"), dist), color::white()));
	}

	if (cf->test(cfg_t::esp_warn_fastfire) && (player_entry.is_charged() || player_entry.is_exploiting()))
		bld.add(esp_side_left, esp_item::icon(XOR_STR("icons/ui/warning"),
											  player_entry.is_exploiting() ? color::attention() : color::white()));

	if (cf->test(cfg_t::esp_software))
	{
		if (player_entry.hack == hack_kind_ev0lve)
			bld.add(esp_side_left, esp_item::text(XOR_STR("E"), color(0, 172, 245)));
		if (player_entry.hack == hack_kind_fatality)
			bld.add(esp_side_left, esp_item::text(XOR_STR("F"), color(235, 5, 90)));
	}

	if (cf->test(cfg_t::esp_armor) && player->get_armor() > 0)
		bld.add(esp_side_right, esp_item::bar(current_armor, player_entry.visuals.previous_armor, color::armor()));

	if (cf->test(cfg_t::esp_flags) && player_entry.is_visually_fakeducking())
		bld.add(esp_side_right, esp_item::text(XOR_STR("DUCK"), color::attention()));

	if (cf->test(cfg_t::esp_warn_zeus) && player->get_ammo_count(FNV1A("AMMO_TYPE_TASERCHARGE")))
		bld.add(esp_side_right, esp_item::text(XOR_STR("ZEUS"), color::warning()));

	if (cf->test(cfg_t::esp_flags))
	{
		if (player->holds_bomb())
			bld.add(esp_side_right, esp_item::text(XOR_STR("C4"), color::warning()));

		if (game->client_entity_list->get_client_entity_from_handle(player->get_carried_hostage()))
			bld.add(esp_side_right, esp_item::text(XOR_STR("HOST"), color::warning()));

		if (player->has_defuser())
			bld.add(esp_side_right, esp_item::text(XOR_STR("D"), color::armor()));

		if (player->get_armor() > 0)
			bld.add(esp_side_right,
					esp_item::text(player->has_helmet() ? XOR_STR("KH") : XOR_STR("K"), color::white()));

		if (player_entry.hittable && rag.is_active())
			bld.add(esp_side_right, esp_item::text(XOR_STR("HIT"), color::info()));
	}

	if (wpn && wpn->get_clip1() > 0)
	{
		const auto wpn_info = wpn->get_cs_weapon_data();

		if (wpn_info->maxclip1 > 1)
		{
			const auto current_ammo =
				std::clamp(wpn->get_clip1(), 0, wpn_info->maxclip1) / static_cast<float>(wpn_info->maxclip1);
			linear_fade(player_entry.visuals.previous_ammo, current_ammo, 1.f, 1.f, false);

			if (cf->test(cfg_t::esp_ammo))
				bld.add(esp_side_bottom,
						esp_item::bar(current_ammo, player_entry.visuals.previous_ammo, color::ammo()));
		}
	}

	if (wpn && cf->test(cfg_t::esp_weapon))
	{
		if (cfg.player_esp.prefer_icons.get())
			bld.add(esp_side_bottom,
					esp_item::icon(tfm::format(XOR_STR("icons/equipment/%s"), wpn->get_icon_name()), color::white()));
		else
		{
			auto weapon_name_raw = wpn->get_localized_name();
			std::transform(weapon_name_raw.begin(), weapon_name_raw.end(), weapon_name_raw.begin(), ::towupper);
			auto weapon_name = std::string(weapon_name_raw.begin(), weapon_name_raw.end());
			shorten_str(weapon_name);

			bld.add(esp_side_bottom, esp_item::text(weapon_name, color::white()));
		}
	}

#ifdef CSGO_LUA
	lua::api.callback(FNV1A("on_paint_esp"),
					  [&](lua::state &s)
					  {
						  s.create_user_object_ptr(XOR_STR("csgo.entity"), player);
						  s.create_user_object_ptr(XOR_STR("render.esp_builder"), &bld);
						  return 2;
					  });
#endif

	bld.draw(draw, *box, fractional, clr_a);
}

void esp::draw_player_offscreen(const gui_legacy::draw_adapter &draw, cs_player_t *const player) const
{
	if (!cfg.player_esp.offscreen_esp.get())
		return;

	if (!game->local_player || !player)
		return;

	if (player->is_dormant() && cfg.player_esp.legit_esp.get())
		return;

	auto &player_entry = GET_PLAYER_ENTRY(player);
	float rotation;
	const auto vertex_pos =
		rotate_offscreen(player_entry.visuals.pos.current, cfg.player_esp.offscreen_radius.get(), rotation);

	std::vector<vertex_t> verts{vec2{}, vec2{-10, 20}, vec2{10, 20}};

	if (fabsf(player_entry.visuals.alpha) <= FLT_EPSILON)
		return;

	const auto clr = cfg.player_esp.offscreen_color.get();
	draw.polygon(vertex_pos, verts, -rotation, clr.mod_a(player_entry.visuals.alpha));

	vec2 top_left{}, bot_right{};

	for (auto &v : verts)
	{
		v.pos += vertex_pos;
		v = rotate_vertex(vertex_pos, v, -rotation);

		if (top_left.x == 0.f || top_left.x > v.pos.x)
			top_left.x = v.pos.x;

		if (top_left.y == 0.f || top_left.y > v.pos.y)
			top_left.y = v.pos.y;

		if (bot_right.x == 0.f || bot_right.x < v.pos.x)
			bot_right.x = v.pos.x;

		if (bot_right.y == 0.f || bot_right.y < v.pos.y)
			bot_right.y = v.pos.y;
	}

	auto &cf = player->is_enemy() ? cfg.player_esp.enemy : cfg.player_esp.team;

	if (!cf->first_set_bit())
		return;

	if (cf->test(cfg_t::esp_health_bar))
	{
		const auto health_bar = std::make_pair(vec2(top_left.x - 6, top_left.y), vec2(top_left.x - 2, bot_right.y));
		const auto current_health = std::clamp(player->get_health(), 0, 100) / 100.f;

		draw_bar(draw, health_bar,
				 interpolate(color::health_end(), color::health_start(), current_health,
							 (player_entry.visuals.alpha - .3f) * 2.5f)
					 .alpha(player_entry.visuals.alpha),
				 current_health, player_entry.visuals.previous_health,
				 (cf->test(cfg_t::esp_health_text) && current_health <= .99f) ? std::to_string(player->get_health())
																			  : XOR_STR(""));
	}

	const auto wpn = reinterpret_cast<weapon_t *>(
		game->client_entity_list->get_client_entity_from_handle(player->get_active_weapon()));

	if (wpn && cf->test(cfg_t::esp_weapon))
	{
		if (cfg.player_esp.prefer_icons.get())
			draw.weapon({top_left.x + (bot_right.x - top_left.x) / 2.f, bot_right.y + 6.f}, 12.f,
						color::white().alpha(player_entry.visuals.alpha), wpn->get_icon_name());
		else
		{
			auto weapon_name = wpn->get_localized_name();
			std::transform(weapon_name.begin(), weapon_name.end(), weapon_name.begin(), ::towupper);
			shorten_str(weapon_name);

			draw.text({top_left.x + (bot_right.x - top_left.x) / 2.f, bot_right.y},
					  color::white().alpha(player_entry.visuals.alpha), weapon_name.c_str(), FNV1A("small8"),
					  gui_legacy::horizontal_alignment::center);
		}
	}
}

void esp::draw_entity(const gui_legacy::draw_adapter &draw, entity_t *const ent) const
{
	static auto white = setting<evo::ren::color>(evo::ren::color::white());

	if (ent->get_entity_type() == entity_type::none)
		return;

	if (ent->is_player())
		return draw_player(draw, reinterpret_cast<cs_player_t *>(ent));

	vec3 screen_origin;
	const auto on_screen = world_to_screen(ent->get_abs_origin(), screen_origin);
	const auto type = ent->get_entity_type();
	if (!on_screen && type != entity_type::projectile)
		return;

	wchar_t out[256] = {0};
	auto cf = &cfg.world_esp.weapon;
	auto clr = &white;
	auto pad = 0;

	draw.set_alpha_modifier(.7f);

	switch (type)
	{
	case entity_type::c4:
		if (game->client_entity_list->get_client_entity_from_handle(
				reinterpret_cast<entity_t *>(ent)->get_owner_entity()))
			return;
	case entity_type::plantedc4:
		cf = &cfg.world_esp.objective;
		clr = &cfg.world_esp.objective_color;
		swprintf_s(out, 256, XOR_STR_UTF8("C4"));
		break;
	case entity_type::hostage:
		if (game->client_entity_list->get_client_entity_from_handle(
				reinterpret_cast<entity_t *>(ent)->get_owner_entity()))
			return;
		cf = &cfg.world_esp.objective;
		clr = &cfg.world_esp.objective_color;
		swprintf_s(out, 256, XOR_STR_UTF8("Hostage"));
		break;
	case entity_type::projectile:
	{
		cf = &cfg.world_esp.nades;

		const auto wpn = reinterpret_cast<weapon_t *>(ent);
		auto owner = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity_from_handle(wpn->get_owner_entity()));
		player_info player_inf;

		if (wpn->get_origin().is_zero() || !owner ||
			!game->engine_client->get_player_info(owner->index(), &player_inf) ||
			(!owner->is_enemy() && owner != game->local_player))
			return;

		const auto model = wpn->get_studio_hdr();

		if (wpn->get_class_id() == class_id::molotov_projectile &&
			(draw_nade_impact(draw, wpn, true) - vec2(screen_origin.x, screen_origin.y)).length() < 26.f)
			return;

		if (wpn->get_class_id() == class_id::base_csgrenade_projectile && model &&
			*reinterpret_cast<uint32_t *>(model->hdr->name + 13) == 0x67617266 &&
			(draw_nade_impact(draw, wpn, false) - vec2(screen_origin.x, screen_origin.y)).length() < 26.f)
			return;

		if (!on_screen)
			return;

		if (model)
		{
			if (cfg.player_esp.prefer_icons.get() && wpn->get_class_id() != class_id::inferno)
			{
				if (wpn->get_class_id() != class_id::base_csgrenade_projectile ||
					*reinterpret_cast<uint32_t *>(model->hdr->name + 13) != 0x67617266 ||
					wpn->get_grenade_velocity().length() > .1f)
					draw.weapon(vec2(screen_origin.x, screen_origin.y), 10.f, color::white(),
								wpn->get_nade_icon_name());
			}
			else
				switch (wpn->get_class_id())
				{
				case class_id::molotov_projectile:
					swprintf_s(out, 256, XOR_STR_UTF8("Fire"));
					break;
				case class_id::smoke_grenade_projectile:
					swprintf_s(out, 256, XOR_STR_UTF8("Smoke"));
					break;
				case class_id::base_csgrenade_projectile:
					if (*reinterpret_cast<uint32_t *>(model->hdr->name + 13) == 0x67617266)
					{
						if (wpn->get_grenade_velocity().length() > .1f)
							swprintf_s(out, 256, XOR_STR_UTF8("Nade"));
					}
					else
						swprintf_s(out, 256, XOR_STR_UTF8("Flash"));
					break;
				default:
					break;
				}
		}

		switch (wpn->get_class_id())
		{
		case class_id::molotov_projectile:
		case class_id::incendiary_grenade:
		case class_id::molotov_grenade:
		case class_id::inferno:
			clr = &cfg.world_esp.fire_color;
			break;
		case class_id::smoke_grenade_projectile:
			clr = &cfg.world_esp.smoke_color;
			break;
		default:
			break;
		}

		auto time_left = 0.f;

		if (cf->get().test(cfg_t::esp_expiry))
		{
			auto start_time = wpn->get_detonation_time();
			auto stop_time = wpn->get_expiry_time();

			wchar_t timer[256]{};

			if (start_time > 0.f && stop_time > 0.f)
			{
				time_left = stop_time - (game->globals->curtime - start_time);

				if (time_left <= 0.f)
					return;

				memset(out, 0, 256);

				const auto timer_bar = std::make_pair(vec2(screen_origin.x - 12, screen_origin.y - 2),
													  vec2(screen_origin.x + 12, screen_origin.y + 2));
				draw_bar(draw, timer_bar, clr->get(), 1.f - (game->globals->curtime - start_time) / stop_time,
						 1.f - (game->globals->curtime - start_time) / stop_time);

				swprintf_s(timer, 256, XOR_STR_UTF8("%.2f"), time_left);
				draw.text(vec2(screen_origin.x, screen_origin.y + 3.f), clr->get(), timer, FNV1A("small8"),
						  gui_legacy::horizontal_alignment::center);
			}
		}

		if (time_left == 0.f && cf->get().test(cfg_t::esp_text))
			draw.text(vec2(screen_origin.x, screen_origin.y), clr->get(), out, FNV1A("small8"),
					  gui_legacy::horizontal_alignment::center);

		break;
	}
	case entity_type::weapon_entity:
	{
		cf = &cfg.world_esp.weapon;
		clr = &cfg.world_esp.weapon_color;

		if (ent != closest_entity[size_t(ent->get_class_id())].first)
			return;

		const auto wpn = reinterpret_cast<weapon_t *>(ent);
		const auto wpn_info = wpn->get_cs_weapon_data();
		if (game->client_entity_list->get_client_entity_from_handle(wpn->get_owner_entity()))
			return;

		const auto abs_dist = (game->view_render->get_view_setup()->origin - wpn->get_abs_origin()).length();
		const auto dist = std::clamp(abs_dist - 1800.f, 0.f, 510.f);
		draw.set_alpha_modifier((255.f - dist / 2.f) / 255.f * draw.get_alpha_modifier());

		if (cf->get().test(cfg_t::esp_text))
		{
			if (cfg.player_esp.prefer_icons.get())
			{
				draw.weapon(vec2(screen_origin.x, screen_origin.y), 10.f, clr->get(), wpn->get_icon_name());
				pad += 3;
			}
			else
			{
				auto weapon_name = wpn->get_localized_name();
				std::transform(weapon_name.begin(), weapon_name.end(), weapon_name.begin(), ::towupper);
				shorten_str(weapon_name);
				swprintf_s(out, 256, weapon_name.c_str());
			}
		}

		if (cf->get().test(cfg_t::esp_ammo) && wpn->get_clip1() > 0 && wpn_info && wpn_info->maxclip1 > 1)
		{
			const auto percent =
				std::clamp(wpn->get_clip1(), 0, wpn_info->maxclip1) / static_cast<float>(wpn_info->maxclip1);
			const auto armor_bar = std::make_pair(vec2(screen_origin.x - 12, screen_origin.y - 7),
												  vec2(screen_origin.x + 12, screen_origin.y - 3));
			draw_bar(draw, armor_bar, cfg.world_esp.weapon_ammo_color.get(), percent, percent);
		}
		break;
	}
	default:
		return;
	}

	if (cf->get().test(cfg_t::esp_text))
	{
		pad += 9;
		draw.text(vec2(screen_origin.x, screen_origin.y), clr->get(), out, FNV1A("small8"),
				  gui_legacy::horizontal_alignment::center);
	}

	if (cf->get().test(cfg_t::esp_distance))
	{
		const auto dist = calculate_distance_to_object(ent);
		if (dist > 0)
			draw.text(vec2(screen_origin.x, screen_origin.y + pad), clr->get(), XOR_FORMAT_UTF8("%d YD", dist),
					  FNV1A("small8"), gui_legacy::horizontal_alignment::center);
	}
}

vec2 esp::draw_nade_impact(const gui_legacy::draw_adapter &draw, weapon_t *const wpn, bool molotov) const
{
	const auto thrower =
		reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity_from_handle(wpn->get_thrower()));
	const auto progress = wpn->get_next_trail_line();
	if (!game->local_player || !thrower || !thrower->is_player() || !cfg.world_esp.grenade_warning.get() ||
		!wpn->get_studio_hdr() || wpn->get_last_trail_pos().z == FLT_MAX || progress > 1.f || progress < 0.f ||
		wpn->get_simulation_time() < game->engine_client->get_last_timestamp() - .35f)
		return vec2(FLT_MAX, FLT_MAX);

	if (cfg.world_esp.grenade_warning_options->test(cfg_t::grenade_warning_only_hostile) &&
		(GET_CONVAR_INT("mp_friendlyfire") ||
		 (!game->local_player->is_enemy(thrower) && game->local_player != thrower)))
		return vec2(FLT_MAX, FLT_MAX);

	vec3 screen_end;
	vec2 screen;
	if (!world_to_screen(wpn->get_last_trail_pos(), screen_end))
	{
		float rotation;
		screen = rotate_offscreen(wpn->get_last_trail_pos(), .5f * evo::ren::draw.display.x - 48.f, rotation);
	}
	else
		screen = vec2(screen_end.x, screen_end.y);

	auto alpha = 1.f;
	if (cfg.world_esp.grenade_warning_options->test(cfg_t::grenade_warning_only_near))
		alpha *= 1.f - max((wpn->get_last_trail_pos() - game->local_player->get_origin()).length() - 300.f, 0) * .025f;

	if (progress < .1f)
		alpha *= progress * 10.f;
	else if (progress > .9f)
		alpha *= 1.f - (progress - .9f) * 10.f;

	if (alpha <= 0.f)
		return vec2(FLT_MAX, FLT_MAX);

	const auto fraction =
		estimate_nade_damage_fraction(thrower, game->local_player, wpn->get_last_trail_pos(), molotov);
	draw.circle(screen, 24.f, color::black().alpha(alpha));
	if (fraction > 0.f)
	{
		auto clr_dmg = *cfg.world_esp.grenade_warning_damage;
		clr_dmg.a(clr_dmg.value.a * .8f * alpha * fraction);
		draw.circle_filled_fade(screen, 24.f, clr_dmg, fraction);
	}

	auto clr_progress = *cfg.world_esp.grenade_warning_progress;
	clr_progress.a(clr_progress.value.a * alpha);
	draw.circle(screen, 24.f, clr_progress, progress, false);

	if (cfg.world_esp.grenade_warning_options->test(cfg_t::grenade_warning_show_type))
		draw.weapon(screen + vec2(1.f, -14.f), 24.f, color::white().alpha(alpha), wpn->get_nade_icon_name());
	else
		draw.text(screen, color::white().alpha(alpha), L"!", FNV1A("segoeuib28"),
				  gui_legacy::horizontal_alignment::center, gui_legacy::vertical_alignment::center);

	return screen;
}

void esp::draw_box(const gui_legacy::draw_adapter &draw, const std::pair<gui_legacy::vec2, gui_legacy::vec2> &box,
				   const sdk::color &clr, const bool drop_shadow) const
{
	if (drop_shadow)
	{
		draw.rect(box.first - vec2(1, 1), box.second + vec2(1, 1), color(0, 0, 0).alpha(.5f * clr.alpha() / 255.f));
		draw.rect(box.first + vec2(1, 1), box.second - vec2(1, 1), color(0, 0, 0).alpha(.5f * clr.alpha() / 255.f));
	}

	draw.rect(box.first, box.second, clr);
}

void esp::draw_bar(const gui_legacy::draw_adapter &draw, const std::pair<gui_legacy::vec2, gui_legacy::vec2> &box,
				   const sdk::color &clr, const float percent, const float old_percent,
				   std::optional<std::string> text) const
{
	const auto is_horizontal = (box.second.x - box.first.x) > (box.second.y - box.first.y);
	const auto length = (is_horizontal ? (box.second.x - box.first.x) : (box.second.y - box.first.y)) - 1;
	const auto old_end = old_percent * length;
	const auto end = percent * length;
	const auto alpha = clr.alpha() / 255.f;

	draw.rect_filled(box.first, box.second, color(0, 0, 0).alpha(.5f * alpha));

	if (is_horizontal)
	{
		draw.rect_filled(box.first + vec2(1, 1), box.first + vec2(end, box.second.y - box.first.y - 1), clr);
		draw.rect_filled(box.first + vec2(end, 1), box.first + vec2(old_end, box.second.y - box.first.y - 1),
						 clr.alpha(.45f));
	}
	else
	{
		draw.rect_filled(box.first + vec2(1, 1 + length - end), box.second - vec2(1, 1), clr);
		draw.rect_filled(box.first + vec2(1, 1 + length - old_end),
						 box.first + vec2(box.second.x - box.first.x - 1, length - end), clr.alpha(.45f));
	}

	if (text.has_value())
	{
		const auto cl = color::white().alpha(alpha);
		const auto txt = std::wstring(text->begin(), text->end());

		if (is_horizontal)
			draw.text(box.first + vec2(end, box.second.y - box.first.y - 1), cl, txt.c_str(), FNV1A("small8"),
					  gui_legacy::horizontal_alignment::center, gui_legacy::vertical_alignment::center);
		else
			draw.text(box.first + vec2(2, length - end + 1), cl, txt.c_str(), FNV1A("small8"),
					  gui_legacy::horizontal_alignment::center, gui_legacy::vertical_alignment::center);
	}
}

void esp::draw_bones(const gui_legacy::draw_adapter &draw, cs_player_t *player, const color &clr) const
{
	auto hdr = player->get_studio_hdr();
	if (!hdr || !hdr->hdr || !player->get_bone_accessor().out)
		return;
	const auto studio_model = hdr->hdr;

	const auto &entry = GET_PLAYER_ENTRY(player);
	mat3x4 bones[max_bones];
	memcpy(bones, entry.visuals.matrix, sizeof(mat3x4) * max_bones);
	auto start = entry.visuals.abs_org;
	move_matrix(bones, start, entry.visuals.pos.current);

	const auto has_hitbox_attached = [&](int32_t i)
	{
		for (auto &hitbox : cs_player_t::hitboxes)
		{
			const auto box = studio_model->get_hitbox(static_cast<uint32_t>(hitbox), player->get_hitbox_set());
			if (box && box->bone == i)
				return true;
		}

		return false;
	};

	for (auto i = 0; i < hdr->numbones(); i++)
	{
		auto bone = hdr->bone(i);
		if (!bone || !(bone->flags & bone_used_by_hitbox) || !has_hitbox_attached(i) || bone->parent < 0)
			continue;

		auto to = bone->parent;
		for (auto to_bone = hdr->bone(to);
			 (!(bone->flags & bone_used_by_hitbox) || !has_hitbox_attached(to)) && to_bone; to = to_bone->parent)
			;

		const auto from_bone = vec3(bones[i][0][3], bones[i][1][3], bones[i][2][3]);
		const auto to_bone = vec3(bones[to][0][3], bones[to][1][3], bones[to][2][3]);

		vec3 screen_from, screen_to;
		if (!world_to_screen(from_bone, screen_from) || !world_to_screen(to_bone, screen_to))
			continue;

		draw.line(vec2(screen_from.x, screen_from.y), vec2(screen_to.x, screen_to.y),
				  clr.alpha(entry.visuals.dormant_alpha));
	}
}

std::string esp::get_player_name(cs_player_t *const player) const
{
	const auto info = game->engine_client->get_player_info(player->index());
	std::wstring name{util::utf8_decode(info.name)};
	shorten_str(name);
	return util::utf8_encode(name);
}

void esp::shorten_str(std::string &str) const
{
	auto s = util::utf8_decode(str);
	if (s.length() > 14)
	{
		s.resize(11);

		for (auto i = 0; i < 3; i++)
			s.push_back('.');
	}
	str = util::utf8_encode(s);
}

void esp::shorten_str(std::wstring &str) const
{
	auto s = str;
	if (s.length() > 14)
	{
		s.resize(11);

		for (auto i = 0; i < 3; i++)
			s.push_back('.');
	}
	str = s;
}

uint32_t esp::calculate_distance_to_object(entity_t *const entity) const
{
	static constexpr auto hammer_to_yards = .037037037f;

	const auto view_setup = game->view_render->get_view_setup();

	if (!view_setup)
		return 0;

	auto origin = entity->get_abs_origin();
	if (entity->is_player())
		origin = GET_PLAYER_ENTRY(reinterpret_cast<cs_player_t *>(entity)).visuals.pos.current;

	const auto length_remaining = (origin - view_setup->origin).length() * hammer_to_yards;
	if (length_remaining < 0)
		return 0;

	return static_cast<uint32_t>(std::floor(length_remaining / 5.f) * 5.f);
}

float esp::estimate_nade_damage_fraction(cs_player_t *const thrower, cs_player_t *const player, const vec3 &impact,
										 bool molotov) const
{
	if (!player->is_enemy(thrower) && thrower != player && !GET_CONVAR_INT("mp_friendlyfire"))
		return 0.f;

	auto world_space_center = player->get_abs_origin();
	world_space_center.z += .5f * player->get_collideable()->obb_maxs().z;

	if (molotov)
	{
		const auto dist = (impact - world_space_center).length();
		return std::clamp(1.f - max(dist - 200.f, 0) * .025f, 0.f, 1.f);
	}

	if ((world_space_center - impact).length() < 350.f * GET_CONVAR_FLOAT("sv_hegrenade_radius_multiplier"))
	{
		trace_world_filter filter{};
		ray r{};
		r.init(impact, world_space_center);
		game_trace tr{};
		game->engine_trace->trace_ray(r, mask_shot, &filter, &tr);

		if (tr.fraction >= .99f)
		{
			const auto scaled_length = (((world_space_center - impact).length() - 25.f) / 140.f);
			const auto unscaled_damage = 105.f * exp(-scaled_length * scaled_length);
			const auto scaled_damage =
				max(int32_t(ceilf(trace.scale_damage(player, unscaled_damage, .5f, hitgroup_generic))), 0);

			if (scaled_damage == 0)
				return 0.f;

			if (!player->is_enemy(thrower))
			{
				if (thrower == player)
					return (scaled_damage * GET_CONVAR_FLOAT("ff_damage_reduction_grenade_self")) /
						   float(player->get_player_health());

				return (scaled_damage * GET_CONVAR_FLOAT("ff_damage_reduction_grenade")) /
					   float(player->get_player_health());
			}

			return (scaled_damage * GET_CONVAR_FLOAT("sv_hegrenade_damage_multiplier")) /
				   float(player->get_player_health());
		}
	}

	return 0.f;
}

std::optional<std::pair<gui_legacy::vec2, gui_legacy::vec2>> esp::get_player_bounds(cs_player_t *const player) const
{
	const auto view_setup = game->view_render->get_view_setup();

	if (!game->rules->data || !view_setup)
		return std::nullopt;

	const auto &origin = GET_PLAYER_ENTRY(player).visuals.pos.current;
	const auto shoot =
		player->is_dormant()
			? origin.z + game->rules->data->get_view_vectors()->duck_view.z +
				  (game->rules->data->get_view_vectors()->view - game->rules->data->get_view_vectors()->duck_view).z *
					  (1.f - player->get_duck_amount())
			: player->get_eye_position().z;

	const auto bottom = origin - vec3(0, 0, 6);
	const auto top = origin + vec3(0, 0, shoot - origin.z + 8);

	vec3 screen_bottom, screen_top;
	if (!world_to_screen(bottom, screen_bottom) || !world_to_screen(top, screen_top))
		return std::nullopt;

	if (!is_point_in_viewport(screen_bottom) || !is_point_in_viewport(screen_top))
		return std::nullopt;

	const auto height = max(screen_bottom.y - screen_top.y, 20.f);
	const auto half_height = height * .5f;
	const auto half_width = height * .275f;
	const auto median_x = (screen_bottom.x + screen_top.x) / 2.f;
	const auto median_y = (screen_bottom.y + screen_top.y) / 2.f;

	auto box = std::make_pair(vec2(median_x - half_width, median_y - half_height),
							  vec2(median_x + half_width, median_y + half_height));
	normalize_box(box);
	return box;
}

bool esp::is_point_in_viewport(vec3 &vec) const
{
	const auto half_x = .5f * evo::ren::draw.display.x;
	const auto half_y = .5f * evo::ren::draw.display.y;
	const auto length = sqrt_ps(half_x * half_x + half_y * half_y);
	const auto dir_x = vec.x - half_x;
	const auto dir_y = vec.y - half_y;
	const auto dir_length = sqrt_ps(dir_x * dir_x + dir_y * dir_y);
	return dir_length <= length;
}

void esp::normalize_box(std::pair<gui_legacy::vec2, gui_legacy::vec2> &box) const
{
	if (box.first.x > box.second.x)
		std::swap(box.first.x, box.second.x);

	if (box.first.y > box.second.y)
		std::swap(box.first.y, box.second.y);
}

vec2 esp::rotate_offscreen(vec3 pos, float offset, float &rotation) const
{
	const auto view_setup = game->view_render->get_view_setup();

	if (!view_setup)
		return vec2();

	auto delta = pos - view_setup->origin;
	delta.normalize();

	const auto screen_size = evo::ren::draw.display, screen_center = screen_size / 2.f;
	const auto view = game->engine_client->get_view_angles();
	vec3 forward, up = vec3(0.f, 0.f, 1.f);
	angle_vectors(view, forward);
	forward.z = 0.f;
	forward.normalize();

	const auto right = up.cross(forward);
	const auto front = delta.dot(forward);
	const auto side = delta.dot(right);

	auto vertex_pos = vec2(-side, -front) * 360.f;
	rotation = RAD2DEG(atan2(vertex_pos.x, vertex_pos.y) + sdk::pi);
	const auto rotation_rad = DEG2RAD(-rotation);
	const auto offset_y = (screen_size.y / screen_size.x) * offset;

	return vec2(screen_center.x + offset * sin(rotation_rad), screen_center.y - offset_y * cos(rotation_rad));
}

void esp_builder::draw(const gui_legacy::draw_adapter &draw, const esp_box_t &box, float fractional, float clr_a)
{
	vec2 cur;

	// render top.
	cur.y = box.first.y - 2.f;
	for (const auto &item : bars[esp_side_top])
	{
		es->draw_bar(draw, std::make_pair(vec2(box.first.x - 1, cur.y - 4), vec2(box.second.x + 1, cur.y)),
					 saturate(item.color, fractional).alpha(clr_a), item.fill, item.fill_old,
					 item.content_extra.empty() ? std::nullopt : std::optional(item.content_extra));

		cur.y -= 6.f;
	}

	for (const auto &item : std::ranges::reverse_view(items[esp_side_top]))
	{
		if (item.type == esp_item::item_type_text)
		{
			draw.text({box.first.x + (box.second.x - box.first.x) / 2.f, cur.y},
					  saturate(item.color, fractional).alpha(clr_a), util::utf8_decode(item.content).c_str(),
					  FNV1A("segoeuib13"), gui_legacy::horizontal_alignment::center,
					  gui_legacy::vertical_alignment::bottom);

			cur.y -= 15.f;
		}
		else if (item.type == esp_item::item_type_icon)
		{
			draw.icon({box.first.x + (box.second.x - box.first.x) * 0.5f, cur.y - 14.f}, 12.f,
					  saturate(item.color, fractional).alpha(clr_a), item.content.c_str());
			cur.y -= 14.f;
		}
	}

	// render bottom.
	cur.y = box.second.y + 2.f;

	for (const auto &item : bars[esp_side_bottom])
	{
		es->draw_bar(draw, std::make_pair(vec2(box.first.x - 1, cur.y), vec2(box.second.x + 1, cur.y + 4)),
					 saturate(item.color, fractional).alpha(clr_a), item.fill, item.fill_old,
					 item.content_extra.empty() ? std::nullopt : std::optional(item.content_extra));

		cur.y += 6.f;
	}

	for (const auto &item : items[esp_side_bottom])
	{
		if (item.type == esp_item::item_type_text)
		{
			draw.text({box.first.x + (box.second.x - box.first.x) / 2.f, cur.y},
					  saturate(item.color, fractional).alpha(clr_a), util::utf8_decode(item.content).c_str(),
					  FNV1A("small8"), gui_legacy::horizontal_alignment::center, gui_legacy::vertical_alignment::top);

			cur.y += 9.f;
		}
		else if (item.type == esp_item::item_type_icon)
		{
			draw.icon({box.first.x + (box.second.x - box.first.x) * 0.5f, cur.y}, 12.f,
					  saturate(item.color, fractional).alpha(clr_a), item.content.c_str());
			cur.y += 14.f;
		}
	}

	// render left.
	cur.x = box.first.x - 2.f;
	cur.y = box.first.y;

	for (const auto &item : bars[esp_side_left])
	{
		es->draw_bar(draw, std::make_pair(vec2(cur.x - 4, box.first.y - 1), vec2(cur.x, box.second.y + 1)),
					 saturate(item.color, fractional).alpha(clr_a), item.fill, item.fill_old,
					 item.content_extra.empty() ? std::nullopt : std::optional(item.content_extra));

		cur.x -= 6.f;
	}

	for (const auto &item : items[esp_side_left])
	{
		if (item.type == esp_item::item_type_text)
		{
			draw.text({cur.x, cur.y}, saturate(item.color, fractional).alpha(clr_a),
					  util::utf8_decode(item.content).c_str(), FNV1A("small8"), gui_legacy::horizontal_alignment::right,
					  gui_legacy::vertical_alignment::top);

			cur.y += 9.f;
		}
		else if (item.type == esp_item::item_type_icon)
		{
			draw.icon({cur.x - 7.f, cur.y}, 12.f, saturate(item.color, fractional).alpha(clr_a), item.content.c_str());
			cur.y += 14.f;
		}
	}

	// render right.
	cur.x = box.second.x + 2.f;
	cur.y = box.first.y;

	for (const auto &item : bars[esp_side_right])
	{
		es->draw_bar(draw, std::make_pair(vec2(cur.x, box.first.y - 1), vec2(cur.x + 4, box.second.y + 1)),
					 saturate(item.color, fractional).alpha(clr_a), item.fill, item.fill_old,
					 item.content_extra.empty() ? std::nullopt : std::optional(item.content_extra));

		cur.x += 6.f;
	}

	for (const auto &item : items[esp_side_right])
	{
		if (item.type == esp_item::item_type_text)
		{
			draw.text({cur.x, cur.y}, saturate(item.color, fractional).alpha(clr_a),
					  util::utf8_decode(item.content).c_str(), FNV1A("small8"));

			cur.y += 9.f;
		}
		else if (item.type == esp_item::item_type_icon)
		{
			draw.icon({cur.x + 7.f, cur.y}, 12.f, saturate(item.color, fractional).alpha(clr_a), item.content.c_str());
			cur.y += 14.f;
		}
	}
}
} // namespace hacks
