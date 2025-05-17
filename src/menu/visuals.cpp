
#include <base/cfg.h>
#include <gui/helpers.h>
#include <gui/preview_model.h>
#include <menu/macros.h>
#include <menu/menu.h>
#include <sdk/convar.h>

USE_NAMESPACES;

#define MANAGE_CHAMS_COLORS_CB                                                                                         \
	const auto manage_chams_colors_cb = [&](const std::string &tab, const std::string &key, bits &value)               \
	{                                                                                                                  \
		const auto primary_color = ctx->find<color_picker>(                                                            \
			evo::gui::hash(XOR("visuals.") + tab + XOR(".chams.") + key + XOR(".primary_color")));                     \
		const auto secondary_color = ctx->find<color_picker>(                                                          \
			evo::gui::hash(XOR("visuals.") + tab + XOR(".chams.") + key + XOR(".secondary_color")));                   \
                                                                                                                       \
		if (primary_color)                                                                                             \
			primary_color->set_visible(value.first_set_bit().value_or(0) > 0);                                         \
		if (secondary_color)                                                                                           \
		{                                                                                                              \
			const auto val = value.first_set_bit().value_or(0);                                                        \
			secondary_color->set_visible(val == 3 || val == 4);                                                        \
		}                                                                                                              \
	};

namespace menu::group
{
const vec2 group_sz{284.f, 430.f};
const vec2 group_half_sz = group_sz * vec2{1.f, 0.5f} - vec2{0.f, 5.f};
const vec2 group_third_sz = group_sz * vec2{1.f, 1.f / 3.f} - vec2{0.f, 7.4f};

void visuals_tab(std::shared_ptr<layout> &e)
{
	const auto side_bar =
		MAKE("visuals.sidebar", tabs_layout, vec2{-10.f, -10.f}, vec2{200.f, 440.f}, td_vertical, true);
	side_bar->add(
		MAKE("visuals.sidebar.enemy", child_tab, XOR("Enemies"), GUI_HASH("visuals.tab.enemy"))->make_selected());
	side_bar->add(MAKE("visuals.sidebar.team", child_tab, XOR("Teammates"), GUI_HASH("visuals.tab.team")));
	side_bar->add(MAKE("visuals.sidebar.local", child_tab, XOR("Local Player"), GUI_HASH("visuals.tab.local")));
	side_bar->add(MAKE("visuals.sidebar.other", child_tab, XOR("Other"), GUI_HASH("visuals.tab.other")));

	side_bar->process_queues();
	side_bar->for_each_control([](std::shared_ptr<control> &c) { c->as<child_tab>()->make_vertical(); });

	const auto tabs = MAKE("visuals.tabs", layout, vec2{200.f, 0.f}, vec2{580.f, 430.f}, s_none);
	tabs->make_not_clip();
	tabs->add(visuals_tab_other());
	tabs->add(visuals_tab_enemy());
	tabs->add(visuals_tab_team());
	tabs->add(visuals_tab_local());

	e->add(side_bar);
	e->add(tabs);
}

std::shared_ptr<layout> visuals_tab_other()
{
	const auto area = MAKE("visuals.tab.other", layout, vec2{}, vec2{580.f, 430.f}, s_justify);
	area->is_visible = false;

	area->add(tools::make_stacked_groupboxes(
		GUI_HASH("visuals.tab.other.general_world_removals"), group_sz,
		{make_groupbox(XOR("visuals.other.general"), XOR("General"), group_third_sz, visuals_other_general),
		 make_groupbox(XOR("visuals.other.world"), XOR("World"), group_third_sz, visuals_other_world),
		 make_groupbox(XOR("visuals.other.removals"), XOR("Removals"), group_third_sz, visuals_other_removals)}));

	area->add(tools::make_stacked_groupboxes(
		GUI_HASH("visuals.tab.other.objects_misc"), group_sz,
		{
			make_groupbox(XOR("visuals.other.objects"), XOR("Objects"), group_half_sz - vec2(0.f, 36.f),
						  visuals_other_objects),
			make_groupbox(XOR("visuals.other.misc"), XOR("Misc"), group_half_sz + vec2(0.f, 35.f), visuals_other_misc),
		}));

	return area;
}

void visuals_other_general(std::shared_ptr<layout> &e)
{
	ADD("Prefer icons", "visuals.other.general.prefer_icons", checkbox, cfg.player_esp.prefer_icons);
	ADD("Disable dormancy interpolation", "visuals.other.general.disable_dormancy_interp", checkbox,
		cfg.player_esp.disable_dormancy_interp);
	ADD_INLINE("Offscreen ESP",
			   MAKE("visuals.other.general.offscreen_esp", checkbox, cfg.player_esp.offscreen_esp)
				   ->make_tooltip(XOR_STR("Renders only if ESP options are selected in either team")),
			   MAKE("visuals.other.general.offscreen_esp_radius", slider<float>, 60.f, 600.f,
					cfg.player_esp.offscreen_radius, XOR("%.0fpx")),
			   MAKE("visuals.other.general.offscreen_esp_color", color_picker, cfg.player_esp.offscreen_color));

	ADD("Apply chams on ragdolls", "visuals.other.general.apply_on_ragdolls", checkbox, cfg.player_esp.apply_on_death);
	ADD("Penetration crosshair", "misc.helpers.penetration_crosshair", checkbox, cfg.misc.penetration_crosshair);
}

void visuals_other_world(std::shared_ptr<layout> &e)
{
	ADD_INLINE("Night mode", MAKE("visuals.other.world.night_mode", checkbox, cfg.local_visuals.night_mode),
			   MAKE("visuals.other.world.night_mode_value", slider<float>, 0.f, 100.f,
					cfg.local_visuals.night_mode_value, XOR("%.0f%%")),
			   MAKE("visuals.other.world.night_mode_color", color_picker, cfg.local_visuals.night_color, false));

	ADD_INLINE("Transparent props", MAKE("visuals.other.world.transparent_props", slider<float>, 0.f, 100.f,
										 cfg.local_visuals.transparent_prop_value, XOR("%.0f%%")));

	ADD("Skip post processing", "visuals.other.world.skip_post_processing", checkbox,
		cfg.local_visuals.disable_post_processing);
	ADD("Disable bloom effect", "visuals.other.world.disable_bloom_effect", checkbox, cfg.local_visuals.disable_bloom);
}

void visuals_other_removals(std::shared_ptr<layout> &e)
{
	ADD("Disable visual recoil", "visuals.other.removals.disable_visual_recoil", checkbox,
		cfg.removals.no_visual_recoil);
	ADD("Disable flashbang effect", "visuals.other.removals.disable_flashbang_effect", checkbox, cfg.removals.no_flash);
	ADD("Disable smoke effect", "visuals.other.removals.disable_smoke_effect", checkbox, cfg.removals.no_smoke);

	const auto scope_color =
		MAKE("visuals.other.removals.override_scope_color", color_picker, cfg.removals.scope_color);

	const auto scope = MAKE("visuals.other.removals.override_scope", combo_box, cfg.removals.scope);
	scope->add({MAKE("visuals.other.removals.override_scope.default", selectable, XOR("Default"), cfg_t::scope_default),
				MAKE("visuals.other.removals.override_scope.static", selectable, XOR("Static"), cfg_t::scope_static),
				MAKE("visuals.other.removals.override_scope.dynamic", selectable, XOR("Dynamic"), cfg_t::scope_dynamic),
				MAKE("visuals.other.removals.override_scope.small", selectable, XOR("Small"), cfg_t::scope_small),
				MAKE("visuals.other.removals.override_scope.small_dynamic", selectable, XOR("Small (dynamic)"),
					 cfg_t::scope_small_dynamic)});

	scope->callback = [scope_color, scope](bits &v)
	{ scope_color->set_visible(v.test(cfg_t::scope_small) || v.test(cfg_t::scope_small_dynamic)); };

	ADD_INLINE("Override scope", scope, scope_color);
}

void visuals_other_objects(std::shared_ptr<layout> &e)
{
	const auto weapons_esp = MAKE("visuals.other.objects.weapons", combo_box, cfg.world_esp.weapon);
	weapons_esp->allow_multiple = true;
	weapons_esp->add(
		{MAKE("visuals.other.objects.weapons.text", selectable, XOR("Text"), cfg_t::esp_text),
		 MAKE("visuals.other.objects.weapons.ammo", selectable, XOR("Ammo"), cfg_t::esp_ammo),
		 MAKE("visuals.other.objects.weapons.distance", selectable, XOR("Distance"), cfg_t::esp_distance)});

	ADD_INLINE("Weapon", weapons_esp,
			   MAKE("visuals.other.objects.weapons_color", color_picker, cfg.world_esp.weapon_color)
				   ->make_tooltip(XOR("Text")),
			   MAKE("visuals.other.objects.weapons_color", color_picker, cfg.world_esp.weapon_ammo_color)
				   ->make_tooltip(XOR("Ammo")));

	const auto objective_esp = MAKE("visuals.other.objects.objective", combo_box, cfg.world_esp.objective);
	objective_esp->allow_multiple = true;
	objective_esp->add(
		{MAKE("visuals.other.objects.objective.text", selectable, XOR("Text"), cfg_t::esp_text),
		 MAKE("visuals.other.objects.objective.distance", selectable, XOR("Distance"), cfg_t::esp_distance)});

	ADD_INLINE("Objective", objective_esp,
			   MAKE("visuals.other.objects.objective_color", color_picker, cfg.world_esp.objective_color));

	const auto nades_esp = MAKE("visuals.other.objects.nades", combo_box, cfg.world_esp.nades);
	nades_esp->allow_multiple = true;
	nades_esp->size.x = 80.f;
	nades_esp->add({MAKE("visuals.other.objects.nades.text", selectable, XOR("Text"), cfg_t::esp_text),
					MAKE("visuals.other.objects.nades.expiry", selectable, XOR("Expiry"), cfg_t::esp_expiry)});

	ADD_INLINE("Nades", nades_esp,
			   MAKE("visuals.other.objects.nades_fire_color", color_picker, cfg.world_esp.fire_color)
				   ->make_tooltip(XOR("Fire")),
			   MAKE("visuals.other.objects.nades_smoke_color", color_picker, cfg.world_esp.smoke_color)
				   ->make_tooltip(XOR("Smoke")),
			   MAKE("visuals.other.objects.nades_flash_color", color_picker, cfg.world_esp.flash_color)
				   ->make_tooltip(XOR("Flash")));

	const auto nade_warning =
		MAKE("visuals.other.objects.nade_warning_options", combo_box, cfg.world_esp.grenade_warning_options);
	nade_warning->allow_multiple = true;
	nade_warning->size.x = 60.f;
	nade_warning->add({
		MAKE("visuals.other.objects.nade_warning.only_near", selectable, XOR("Only on close impact"),
			 cfg_t::grenade_warning_only_near),
		MAKE("visuals.other.objects.nade_warning.only_hostile", selectable, XOR("Only for hostiles"),
			 cfg_t::grenade_warning_only_hostile),
		MAKE("visuals.other.objects.nade_warning.show_type", selectable, XOR("Display type"),
			 cfg_t::grenade_warning_show_type),
	});

	ADD_INLINE("Nade warning", MAKE("visuals.other.objects.nade_warning", checkbox, cfg.world_esp.grenade_warning),
			   nade_warning,
			   MAKE("visuals.other.objects.nade_warning.progress", color_picker, cfg.world_esp.grenade_warning_progress)
				   ->make_tooltip(XOR("Progress")),
			   MAKE("visuals.other.objects.nade_warning.damage", color_picker, cfg.world_esp.grenade_warning_damage)
				   ->make_tooltip(XOR("Damage")));

	ADD_INLINE("Visualize grenade path",
			   MAKE("visuals.other.objects.visualize_grenade_path", checkbox, cfg.world_esp.visualize_nade_path),
			   MAKE("visuals.other.objects.visualize_grenade_path_color", color_picker, cfg.world_esp.nade_path_color));
}

void visuals_other_misc(std::shared_ptr<layout> &e)
{
	ADD_INLINE("Thirdperson", MAKE("visuals.other.misc.thirdperson", checkbox, cfg.local_visuals.thirdperson),
			   MAKE("visuals.other.misc.thirdperson_distance", slider<float>, 50.f, 200.f,
					cfg.local_visuals.thirdperson_distance, XOR("%.0fu")));

	ADD("Thirdperson while dead", "visuals.other.misc.thirdperson_dead", checkbox, cfg.local_visuals.thirdperson_dead);
	ADD("Disable with grenade", "visuals.other.misc.disable_with_grenade", checkbox,
		cfg.local_visuals.thirdperson_nade);
	ADD("Blend alpha in thirdperson", "visuals.other.misc.alpha_blend", checkbox, cfg.local_visuals.alpha_blend);

	const auto hitmarker_style =
		MAKE("visuals.other.misc.hitmarker_style", combo_box, cfg.local_visuals.hitmarker_style);
	hitmarker_style->add({
		MAKE("visuals.other.misc.hitmarker_style.default", selectable, XOR("Default")),
		MAKE("visuals.other.misc.hitmarker_style.animated", selectable, XOR("Animated")),
	});

	ADD_INLINE("Hitmarker", MAKE("visuals.other.misc.hitmarker", checkbox, cfg.local_visuals.hitmarker),
			   hitmarker_style);
	ADD("Hitsound", "visuals.other.misc.hitsound", checkbox, cfg.local_visuals.hitsound);
	ADD_INLINE("FOV", MAKE("visuals.other.misc.fov", checkbox, cfg.local_visuals.fov_changer));

	ADD_LINE("visuals.other.misc.fov",
			 MAKE("visuals.other.misc.fov_value", slider<float>, -40.f, 40.f, cfg.local_visuals.fov, XOR("%.0f°"))
				 ->make_tooltip(XOR("Normal FOV override")),
			 MAKE("visuals.other.misc.fov2_value", slider<float>, 0.f, 100.f, cfg.local_visuals.fov2, XOR("%.0f%%"))
				 ->make_tooltip(XOR("Scoped FOV override")));

	ADD("Skip first zoom level", "visuals.other.misc.skip_first_scope", checkbox, cfg.local_visuals.skip_first_scope);
	ADD("Render viewmodel in zoom", "visuals.other.misc.viewmodel_in_scope", checkbox,
		cfg.local_visuals.viewmodel_in_scope);

	ADD_INLINE("Bullet impacts", MAKE("visuals.other.misc.bullet_impacts", checkbox, cfg.local_visuals.impacts),
			   MAKE("visuals.other.misc.bullet_impacts_server", color_picker, cfg.local_visuals.server_impacts)
				   ->make_tooltip(XOR("Server")),
			   MAKE("visuals.other.misc.bullet_impacts_client", color_picker, cfg.local_visuals.client_impacts)
				   ->make_tooltip(XOR("Client")));

	const auto hit_indicator = MAKE("visuals.other.misc.hit_indicator", combo_box, cfg.local_visuals.hit_indicator);
	hit_indicator->allow_multiple = true;
	hit_indicator->add({
		MAKE("visuals.other.misc.hit_indicator.on_hit", selectable, XOR("On hit")),
		MAKE("visuals.other.misc.hit_indicator.on_kill", selectable, XOR("On kill")),
	});

	ADD_INLINE("Hit indicator", hit_indicator,
			   MAKE("visuals.other.misc.hit_indicator_color", color_picker, cfg.local_visuals.hit_color));

	const auto aa_indicator = MAKE("visuals.other.misc.antiaim_indicator", combo_box, cfg.local_visuals.aa_indicator);
	aa_indicator->allow_multiple = true;
	aa_indicator->add({
		MAKE("visuals.other.misc.antiaim_indicator.real", selectable, XOR("Real")),
		MAKE("visuals.other.misc.antiaim_indicator.desync", selectable, XOR("Desync")),
	});

	ADD_INLINE("AA", aa_indicator,
			   MAKE("visuals.other.misc.antiaim_indicator_real", color_picker, cfg.local_visuals.clr_aa_real)
				   ->make_tooltip(XOR("Real")),
			   MAKE("visuals.other.misc.antiaim_indicator_desync", color_picker, cfg.local_visuals.clr_aa_desync)
				   ->make_tooltip(XOR("Desync")));

	ADD_INLINE("Aspect ratio", MAKE("visuals.other.misc.aspect_ratio", checkbox, cfg.local_visuals.aspect_changer),
			   MAKE("visuals.other.misc.aspect_ratio_value", slider<float>, 0.f, 100.f, cfg.local_visuals.aspect,
					XOR("%.0f%%")));
}

std::shared_ptr<layout> visuals_tab_enemy()
{
	auto area = MAKE("visuals.tab.enemy", layout, vec2{}, vec2{580.f, 430.f}, s_justify);
	area->add(tools::make_stacked_groupboxes(
		GUI_HASH("visuals.tab.enemy.esp_chams"), group_sz,
		{
			make_groupbox(XOR("visuals.enemy.esp"), XOR("ESP"), group_half_sz - vec2(0.f, 60.f), visuals_enemy_esp),
			make_groupbox(XOR("visuals.enemy.chams"), XOR("Chams"), group_half_sz + vec2(0.f, 60.f),
						  visuals_enemy_chams),
		}));

	const auto grp = make_groupbox(XOR("visuals.enemy.preview"), XOR("Preview"), group_sz, visuals_enemy_preview);
	grp->warning = XOR_STR("Model preview is not available at the moment. This issue will be resolved in the future.");

	area->add(grp);
	return std::move(area);
}

void visuals_enemy_esp(std::shared_ptr<layout> &e)
{
	const auto enemy_esp = MAKE("visuals.enemy.esp.enemy_esp", combo_box, cfg.player_esp.enemy);
	enemy_esp->allow_multiple = true;
	enemy_esp->add({
		MAKE("visuals.enemy.esp.enemy_esp.box", selectable, XOR("Box")),
		MAKE("visuals.enemy.esp.enemy_esp.text", selectable, XOR("Name")),
		MAKE("visuals.enemy.esp.enemy_esp.health_bar", selectable, XOR("Health bar")),
		MAKE("visuals.enemy.esp.enemy_esp.health_text", selectable, XOR("Health text")),
		MAKE("visuals.enemy.esp.enemy_esp.armor", selectable, XOR("Armor")),
		MAKE("visuals.enemy.esp.enemy_esp.ammo", selectable, XOR("Ammo")),
		MAKE("visuals.enemy.esp.enemy_esp.weapon", selectable, XOR("Weapon")),
		MAKE("visuals.enemy.esp.enemy_esp.flags", selectable, XOR("Flags")),
		MAKE("visuals.enemy.esp.enemy_esp.distance", selectable, XOR("Distance")),
		MAKE("visuals.enemy.esp.enemy_esp.warn_zeus", selectable, XOR("Zeus warning"), cfg_t::esp_warn_zeus),
		MAKE("visuals.enemy.esp.enemy_esp.warn_fastfire", selectable, XOR("Fast fire warning"),
			 cfg_t::esp_warn_fastfire),
		MAKE("visuals.enemy.esp.enemy_esp.software", selectable, XOR("Software"), cfg_t::esp_software),
	});

	ADD_INLINE("Options", enemy_esp,
			   MAKE("visuals.enemy.esp.enemy_esp_color", color_picker, cfg.player_esp.enemy_color));
	ADD_INLINE("Skeleton", MAKE("visuals.enemy.esp.enemy_skeleton", checkbox, cfg.player_esp.enemy_skeleton),
			   MAKE("visuals.enemy.esp.enemy_skeleton_color", color_picker, cfg.player_esp.enemy_skeleton_color));

	ADD_INLINE(
		"Visualize target scan", MAKE("visuals.enemy.esp.visualize_target_scan", checkbox, cfg.player_esp.target_scan),
		MAKE("visuals.enemy.esp.visualize_target_scan_secure", color_picker, cfg.player_esp.target_scan_secure, false)
			->make_tooltip(XOR("Secure")),
		MAKE("visuals.enemy.esp.visualize_target_scan_aim", color_picker, cfg.player_esp.target_scan_aim, false)
			->make_tooltip(XOR("Aim")));

	ADD("Reveal on radar", "visuals.enemy.esp.reveal_on_radar", checkbox, cfg.world_esp.enemy_radar);
	ADD("Shared ESP", "visuals.enemy.esp.shared_esp", checkbox, cfg.player_esp.shared_esp);
	ADD("Share with enemies", "visuals.enemy.esp.share_with_enemies", checkbox, cfg.player_esp.shared_esp_enemy);
}

void visuals_enemy_chams(std::shared_ptr<layout> &e)
{
	MANAGE_CHAMS_COLORS_CB;

	const auto enemy_chams = MAKE("visuals.enemy.chams.enemy_chams", combo_box, cfg.player_esp.enemy_chams.mode);
	enemy_chams->add({MAKE("visuals.enemy.chams.enemy_chams.disabled", selectable, XOR("Disabled")),
					  MAKE("visuals.enemy.chams.enemy_chams.default", selectable, XOR("Default")),
					  MAKE("visuals.enemy.chams.enemy_chams.flat", selectable, XOR("Flat")),
					  MAKE("visuals.enemy.chams.enemy_chams.glow", selectable, XOR("Glow")),
					  MAKE("visuals.enemy.chams.enemy_chams.pulse", selectable, XOR("Pulse")),
					  MAKE("visuals.enemy.chams.enemy_chams.acryl", selectable, XOR("Acryl"))});

	enemy_chams->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("enemy"), XOR_STR("enemy_chams"), value); };

	ADD_SECTION("Invisible", 250.f);
	ADD_INLINE(
		"Model", enemy_chams,
		MAKE("visuals.enemy.chams.enemy_chams.primary_color", color_picker, cfg.player_esp.enemy_chams.primary)
			->make_tooltip(XOR("Primary color"))
			->make_invisible(),
		MAKE("visuals.enemy.chams.enemy_chams.secondary_color", color_picker, cfg.player_esp.enemy_chams.secondary)
			->make_tooltip(XOR("Secondary color"))
			->make_invisible());

	const auto enemy_attachment_chams =
		MAKE("visuals.enemy.chams.enemy_attachment_chams", combo_box, cfg.player_esp.enemy_attachment_chams.mode);
	enemy_attachment_chams->add(
		{MAKE("visuals.enemy.chams.enemy_attachment_chams.disabled", selectable, XOR("Disabled")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams.default", selectable, XOR("Default")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams.flat", selectable, XOR("Flat")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams.glow", selectable, XOR("Glow")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams.pulse", selectable, XOR("Pulse")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams.acryl", selectable, XOR("Acryl"))});

	enemy_attachment_chams->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("enemy"), XOR_STR("enemy_attachment_chams"), value); };

	ADD_INLINE("Attachments", enemy_attachment_chams,
			   MAKE("visuals.enemy.chams.enemy_attachment_chams.primary_color", color_picker,
					cfg.player_esp.enemy_attachment_chams.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.enemy.chams.enemy_attachment_chams.secondary_color", color_picker,
					cfg.player_esp.enemy_attachment_chams.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto enemy_chams_vis =
		MAKE("visuals.enemy.chams.enemy_chams_visible", combo_box, cfg.player_esp.enemy_chams_visible.mode);
	enemy_chams_vis->add({MAKE("visuals.enemy.chams.enemy_chams_visible.disabled", selectable, XOR("Disabled")),
						  MAKE("visuals.enemy.chams.enemy_chams_visible.default", selectable, XOR("Default")),
						  MAKE("visuals.enemy.chams.enemy_chams_visible.flat", selectable, XOR("Flat")),
						  MAKE("visuals.enemy.chams.enemy_chams_visible.glow", selectable, XOR("Glow")),
						  MAKE("visuals.enemy.chams.enemy_chams_visible.pulse", selectable, XOR("Pulse")),
						  MAKE("visuals.enemy.chams.enemy_chams_visible.acryl", selectable, XOR("Acryl"))});

	enemy_chams_vis->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("enemy"), XOR_STR("enemy_chams_visible"), value); };

	e->add(MAKE("visible_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Visible", 250.f);
	ADD_INLINE("Model", enemy_chams_vis,
			   MAKE("visuals.enemy.chams.enemy_chams_visible.primary_color", color_picker,
					cfg.player_esp.enemy_chams_visible.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.enemy.chams.enemy_chams_visible.secondary_color", color_picker,
					cfg.player_esp.enemy_chams_visible.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto enemy_attachment_chams_vis =
		MAKE("visuals.enemy.chams.enemy_chams_visible", combo_box, cfg.player_esp.enemy_attachment_chams_visible.mode);
	enemy_attachment_chams_vis->add(
		{MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.disabled", selectable, XOR("Disabled")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.default", selectable, XOR("Default")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.flat", selectable, XOR("Flat")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.glow", selectable, XOR("Glow")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.pulse", selectable, XOR("Pulse")),
		 MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.acryl", selectable, XOR("Acryl"))});

	enemy_attachment_chams_vis->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("enemy"), XOR_STR("enemy_attachment_chams_visible"), value); };

	ADD_INLINE("Attachments", enemy_attachment_chams_vis,
			   MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.primary_color", color_picker,
					cfg.player_esp.enemy_attachment_chams_visible.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.enemy.chams.enemy_attachment_chams_visible.secondary_color", color_picker,
					cfg.player_esp.enemy_attachment_chams_visible.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto enemy_chams_his =
		MAKE("visuals.enemy.chams.enemy_chams_history", combo_box, cfg.player_esp.enemy_chams_history.mode);
	enemy_chams_his->add({MAKE("visuals.enemy.chams.enemy_chams_history.disabled", selectable, XOR("Disabled")),
						  MAKE("visuals.enemy.chams.enemy_chams_history.default", selectable, XOR("Default")),
						  MAKE("visuals.enemy.chams.enemy_chams_history.flat", selectable, XOR("Flat")),
						  MAKE("visuals.enemy.chams.enemy_chams_history.glow", selectable, XOR("Glow")),
						  MAKE("visuals.enemy.chams.enemy_chams_history.pulse", selectable, XOR("Pulse")),
						  MAKE("visuals.enemy.chams.enemy_chams_history.acryl", selectable, XOR("Acryl"))});

	e->add(MAKE("other_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Other", 250.f);

	ADD_INLINE("History", enemy_chams_his,
			   MAKE("visuals.enemy.chams.enemy_chams_history.primary_color", color_picker,
					cfg.player_esp.enemy_chams_history.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.enemy.chams.enemy_chams_history.secondary_color", color_picker,
					cfg.player_esp.enemy_chams_history.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	enemy_chams_his->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("enemy"), XOR_STR("enemy_chams_history"), value); };

	ADD_INLINE("Glow", MAKE("visuals.enemy.chams.glow_enemy", checkbox, cfg.player_esp.glow_enemy),
			   MAKE("visuals.enemy.chams.glow_enemy_color", color_picker, cfg.player_esp.enemy_color_glow));
}

void visuals_enemy_preview(std::shared_ptr<layout> &e)
{
	e->direction = s_none;
	e->add(MAKE("visuals.enemy.preview.preview", ::gui::preview_model));
}

std::shared_ptr<layout> visuals_tab_team()
{
	auto area = MAKE("visuals.tab.team", layout, vec2{}, vec2{580.f, 430.f}, s_justify);
	area->is_visible = false;
	area->add(tools::make_stacked_groupboxes(
		GUI_HASH("visuals.tab.team.esp_chams"), group_sz,
		{
			make_groupbox(XOR("visuals.team.esp"), XOR("ESP"), group_half_sz - vec2(0.f, 90.f), visuals_team_esp),
			make_groupbox(XOR("visuals.team.chams"), XOR("Chams"), group_half_sz + vec2(0.f, 90.f), visuals_team_chams),
		}));

	const auto grp = make_groupbox(XOR("visuals.team.preview"), XOR("Preview"), group_sz, visuals_team_preview);
	grp->warning = XOR_STR("Model preview is not available at the moment. This issue will be resolved in the future.");

	area->add(grp);
	return std::move(area);
}

void visuals_team_esp(std::shared_ptr<layout> &e)
{
	const auto team_esp = MAKE("visuals.team.esp.team_esp", combo_box, cfg.player_esp.team);
	team_esp->allow_multiple = true;
	team_esp->add({
		MAKE("visuals.team.esp.team_esp.box", selectable, XOR("Box")),
		MAKE("visuals.team.esp.team_esp.text", selectable, XOR("Name")),
		MAKE("visuals.team.esp.team_esp.health_bar", selectable, XOR("Health bar")),
		MAKE("visuals.team.esp.team_esp.health_text", selectable, XOR("Health text")),
		MAKE("visuals.team.esp.team_esp.armor", selectable, XOR("Armor")),
		MAKE("visuals.team.esp.team_esp.ammo", selectable, XOR("Ammo")),
		MAKE("visuals.team.esp.team_esp.weapon", selectable, XOR("Weapon")),
		MAKE("visuals.team.esp.team_esp.flags", selectable, XOR("Flags")),
		MAKE("visuals.team.esp.team_esp.distance", selectable, XOR("Distance")),
		MAKE("visuals.team.esp.team_esp.warn_zeus", selectable, XOR("Zeus warning"), cfg_t::esp_warn_zeus),
		MAKE("visuals.team.esp.team_esp.warn_fastfire", selectable, XOR("Fast fire warning"), cfg_t::esp_warn_fastfire),
		MAKE("visuals.team.esp.team_esp.software", selectable, XOR("Software"), cfg_t::esp_software),
	});

	ADD_INLINE("Options", team_esp, MAKE("visuals.team.esp.team_esp_color", color_picker, cfg.player_esp.team_color));
	ADD_INLINE("Skeleton", MAKE("visuals.team.esp.team_skeleton", checkbox, cfg.player_esp.team_skeleton),
			   MAKE("visuals.team.esp.team_skeleton_color", color_picker, cfg.player_esp.team_skeleton_color));
}

void visuals_team_chams(std::shared_ptr<layout> &e)
{
	MANAGE_CHAMS_COLORS_CB;

	ADD_SECTION("Invisible", 250.f);

	const auto team_chams = MAKE("visuals.team.chams.team_chams", combo_box, cfg.player_esp.team_chams.mode);
	team_chams->add({MAKE("visuals.team.chams.team_chams.disabled", selectable, XOR("Disabled")),
					 MAKE("visuals.team.chams.team_chams.default", selectable, XOR("Default")),
					 MAKE("visuals.team.chams.team_chams.flat", selectable, XOR("Flat")),
					 MAKE("visuals.team.chams.team_chams.glow", selectable, XOR("Glow")),
					 MAKE("visuals.team.chams.team_chams.pulse", selectable, XOR("Pulse")),
					 MAKE("visuals.team.chams.team_chams.acryl", selectable, XOR("Acryl"))});

	team_chams->callback = [&](bits &value) { manage_chams_colors_cb(XOR_STR("team"), XOR_STR("team_chams"), value); };

	ADD_INLINE("Model", team_chams,
			   MAKE("visuals.team.chams.team_chams.primary_color", color_picker, cfg.player_esp.team_chams.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.team.chams.team_chams.secondary_color", color_picker, cfg.player_esp.team_chams.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto team_attachment_chams =
		MAKE("visuals.team.chams.team_attachment_chams", combo_box, cfg.player_esp.team_attachment_chams.mode);
	team_attachment_chams->add({MAKE("visuals.team.chams.team_attachment_chams.disabled", selectable, XOR("Disabled")),
								MAKE("visuals.team.chams.team_attachment_chams.default", selectable, XOR("Default")),
								MAKE("visuals.team.chams.team_attachment_chams.flat", selectable, XOR("Flat")),
								MAKE("visuals.team.chams.team_attachment_chams.glow", selectable, XOR("Glow")),
								MAKE("visuals.team.chams.team_attachment_chams.pulse", selectable, XOR("Pulse")),
								MAKE("visuals.team.chams.team_attachment_chams.acryl", selectable, XOR("Acryl"))});

	team_attachment_chams->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("team"), XOR_STR("team_attachment_chams"), value); };

	ADD_INLINE("Attachments", team_attachment_chams,
			   MAKE("visuals.team.chams.team_attachment_chams.primary_color", color_picker,
					cfg.player_esp.team_attachment_chams.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.team.chams.team_attachment_chams.secondary_color", color_picker,
					cfg.player_esp.team_attachment_chams.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	e->add(MAKE("visible_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Visible", 250.f);

	const auto team_chams_vis =
		MAKE("visuals.team.chams.team_chams_visible", combo_box, cfg.player_esp.team_chams_visible.mode);
	team_chams_vis->add({MAKE("visuals.team.chams.team_chams_visible.disabled", selectable, XOR("Disabled")),
						 MAKE("visuals.team.chams.team_chams_visible.default", selectable, XOR("Default")),
						 MAKE("visuals.team.chams.team_chams_visible.flat", selectable, XOR("Flat")),
						 MAKE("visuals.team.chams.team_chams_visible.glow", selectable, XOR("Glow")),
						 MAKE("visuals.team.chams.team_chams_visible.pulse", selectable, XOR("Pulse")),
						 MAKE("visuals.team.chams.team_chams_visible.acryl", selectable, XOR("Acryl"))});

	team_chams_vis->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("team"), XOR_STR("team_chams_visible"), value); };

	ADD_INLINE("Model", team_chams_vis,
			   MAKE("visuals.team.chams.team_chams_visible.primary_color", color_picker,
					cfg.player_esp.team_chams_visible.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.team.chams.team_chams_visible.secondary_color", color_picker,
					cfg.player_esp.team_chams_visible.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto team_attachment_chams_vis = MAKE("visuals.team.chams.team_attachment_chams_visible", combo_box,
												cfg.player_esp.team_attachment_chams_visible.mode);
	team_attachment_chams_vis->add(
		{MAKE("visuals.team.chams.team_attachment_chams_visible.disabled", selectable, XOR("Disabled")),
		 MAKE("visuals.team.chams.team_attachment_chams_visible.default", selectable, XOR("Default")),
		 MAKE("visuals.team.chams.team_attachment_chams_visible.flat", selectable, XOR("Flat")),
		 MAKE("visuals.team.chams.team_attachment_chams_visible.glow", selectable, XOR("Glow")),
		 MAKE("visuals.team.chams.team_attachment_chams_visible.pulse", selectable, XOR("Pulse")),
		 MAKE("visuals.team.chams.team_attachment_chams_visible.acryl", selectable, XOR("Acryl"))});

	team_attachment_chams_vis->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("team"), XOR_STR("team_attachment_chams_visible"), value); };

	ADD_INLINE("Attachments", team_attachment_chams_vis,
			   MAKE("visuals.team.chams.team_attachment_chams_visible.primary_color", color_picker,
					cfg.player_esp.team_attachment_chams_visible.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.team.chams.team_attachment_chams_visible.secondary_color", color_picker,
					cfg.player_esp.team_attachment_chams_visible.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	e->add(MAKE("other_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Other", 250.f);

	ADD_INLINE("Glow", MAKE("visuals.team.chams.glow_team", checkbox, cfg.player_esp.glow_team),
			   MAKE("visuals.team.chams.glow_team_color", color_picker, cfg.player_esp.team_color_glow));
}

void visuals_team_preview(std::shared_ptr<layout> &e)
{
	e->direction = s_none;
	e->add(MAKE("visuals.team.preview.preview", ::gui::preview_model));
}

std::shared_ptr<layout> visuals_tab_local()
{
	auto area = MAKE("visuals.tab.local", layout, vec2{}, vec2{580.f, 430.f}, s_justify);
	area->is_visible = false;
	area->add(make_groupbox(XOR("visuals.local.viewmodel"), XOR("Viewmodel"), group_sz, visuals_local_viewmodel));
	area->add(make_groupbox(XOR("visuals.local.chams"), XOR("Chams"), group_sz, visuals_local_chams));
	return std::move(area);
}

void visuals_local_viewmodel(std::shared_ptr<layout> &e)
{
	const auto dis_bob = MAKE("visuals.local.viewmodel.disable_bob", checkbox, cfg.local_visuals.disable_bob);
	dis_bob->callback = [](bool v)
	{
		// todo: make this garbage via hook
		static const auto sway = game->cvar->find_var(XOR_STR("cl_wpn_sway_interp"));
		sway->min_val = 0.f;
		sway->flags &= ~FCVAR_ARCHIVE;
		sway->orig_flags = sway->flags | FCVAR_UNREGISTERED;
		sway->set_float(v ? 0.f : 0.1f);

		static const auto lat = game->cvar->find_var(XOR_STR("cl_bobamt_lat"));
		lat->min_val = 0.f;
		lat->flags &= ~FCVAR_ARCHIVE;
		lat->orig_flags = lat->flags | FCVAR_UNREGISTERED;
		lat->set_float(v ? 0.f : 0.4f);

		static const auto vert = game->cvar->find_var(XOR_STR("cl_bobamt_vert"));
		vert->min_val = 0.f;
		vert->flags &= ~FCVAR_ARCHIVE;
		vert->orig_flags = vert->flags | FCVAR_UNREGISTERED;
		vert->set_float(v ? 0.f : 0.25f);

		static const auto lower = game->cvar->find_var(XOR_STR("cl_bob_lower_amt"));
		lower->min_val = 0.f;
		lower->flags &= ~FCVAR_ARCHIVE;
		lower->orig_flags = lower->flags | FCVAR_UNREGISTERED;
		lower->set_float(v ? 0.f : 21.f);
	};

	ADD_INLINE("Disable bobbing", dis_bob);
	ADD_INLINE(
		"Field of view",
		MAKE("visuals.local.viewmodel.fov", slider<float>, 40.f, 120.f, cfg.local_visuals.vm_fov, XOR_STR("%.0f°")),
		MAKE("visuals.local.viewmodel.fov_enable", checkbox, cfg.local_visuals.vm_fov_enable));

	ADD("X offset", "visuals.local.viewmodel.x", slider<float>, -10.f, 10.f, cfg.local_visuals.vm_x, XOR_STR("%.2f"),
		0.01f);
	ADD("Y offset", "visuals.local.viewmodel.y", slider<float>, -10.f, 10.f, cfg.local_visuals.vm_y, XOR_STR("%.2f"),
		0.01f);
	ADD("Z offset", "visuals.local.viewmodel.z", slider<float>, -10.f, 10.f, cfg.local_visuals.vm_z, XOR_STR("%.2f"),
		0.01f);
}

void visuals_local_chams(std::shared_ptr<layout> &e)
{
	MANAGE_CHAMS_COLORS_CB;

	ADD_SECTION("General", 250.f);

	const auto local_chams = MAKE("visuals.local.chams.chams", combo_box, cfg.local_visuals.local_chams.mode);
	local_chams->add({MAKE("visuals.local.chams.chams.disabled", selectable, XOR("Disabled")),
					  MAKE("visuals.local.chams.chams.default", selectable, XOR("Default")),
					  MAKE("visuals.local.chams.chams.flat", selectable, XOR("Flat")),
					  MAKE("visuals.local.chams.chams.glow", selectable, XOR("Glow")),
					  MAKE("visuals.local.chams.chams.pulse", selectable, XOR("Pulse")),
					  MAKE("visuals.local.chams.chams.acryl", selectable, XOR("Acryl"))});

	local_chams->callback = [&](bits &value) { manage_chams_colors_cb(XOR_STR("local"), XOR_STR("chams"), value); };

	ADD_INLINE("Model", local_chams,
			   MAKE("visuals.local.chams.chams.primary_color", color_picker, cfg.local_visuals.local_chams.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.local.chams.chams.secondary_color", color_picker, cfg.local_visuals.local_chams.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto local_attachment_chams =
		MAKE("visuals.local.chams.attachment.chams", combo_box, cfg.local_visuals.local_attachment_chams.mode);
	local_attachment_chams->add({MAKE("visuals.local.chams.attachment.chams.disabled", selectable, XOR("Disabled")),
								 MAKE("visuals.local.chams.attachment.chams.default", selectable, XOR("Default")),
								 MAKE("visuals.local.chams.attachment.chams.flat", selectable, XOR("Flat")),
								 MAKE("visuals.local.chams.attachment.chams.glow", selectable, XOR("Glow")),
								 MAKE("visuals.local.chams.attachment.chams.pulse", selectable, XOR("Pulse")),
								 MAKE("visuals.local.chams.attachment.chams.acryl", selectable, XOR("Acryl"))});

	local_attachment_chams->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("local"), XOR_STR("attachment_chams"), value); };

	ADD_INLINE("Attachments", local_attachment_chams,
			   MAKE("visuals.local.chams.attachment_chams.primary_color", color_picker,
					cfg.local_visuals.local_attachment_chams.primary)
				   ->make_tooltip(XOR("Primary color"))
				   ->make_invisible(),
			   MAKE("visuals.local.chams.attachment_chams.secondary_color", color_picker,
					cfg.local_visuals.local_attachment_chams.secondary)
				   ->make_tooltip(XOR("Secondary color"))
				   ->make_invisible());

	const auto fake_chams = MAKE("visuals.local.chams.fake_chams", combo_box, cfg.local_visuals.fake_chams.mode);
	fake_chams->add({MAKE("visuals.local.chams.fake_chams.disabled", selectable, XOR("Disabled")),
					 MAKE("visuals.local.chams.fake_chams.default", selectable, XOR("Default")),
					 MAKE("visuals.local.chams.fake_chams.flat", selectable, XOR("Flat")),
					 MAKE("visuals.local.chams.fake_chams.glow", selectable, XOR("Glow")),
					 MAKE("visuals.local.chams.fake_chams.pulse", selectable, XOR("Pulse")),
					 MAKE("visuals.local.chams.fake_chams.acryl", selectable, XOR("Acryl"))});

	fake_chams->callback = [&](bits &value) { manage_chams_colors_cb(XOR_STR("local"), XOR_STR("fake_chams"), value); };

	ADD_INLINE(
		"Fake", fake_chams,
		MAKE("visuals.local.chams.fake_chams.primary_color", color_picker, cfg.local_visuals.fake_chams.primary)
			->make_tooltip(XOR("Primary color"))
			->make_invisible(),
		MAKE("visuals.local.chams.fake_chams.secondary_color", color_picker, cfg.local_visuals.fake_chams.secondary)
			->make_tooltip(XOR("Secondary color"))
			->make_invisible());

	e->add(MAKE("viewmodel_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Viewmodel", 250.f);

	const auto weapon_chams = MAKE("visuals.local.chams.weapon_chams", combo_box, cfg.local_visuals.weapon_chams.mode);
	weapon_chams->add({MAKE("visuals.local.chams.weapon_chams.disabled", selectable, XOR("Disabled")),
					   MAKE("visuals.local.chams.weapon_chams.default", selectable, XOR("Default")),
					   MAKE("visuals.local.chams.weapon_chams.flat", selectable, XOR("Flat")),
					   MAKE("visuals.local.chams.weapon_chams.glow", selectable, XOR("Glow")),
					   MAKE("visuals.local.chams.weapon_chams.pulse", selectable, XOR("Pulse")),
					   MAKE("visuals.local.chams.weapon_chams.acryl", selectable, XOR("Acryl"))});

	weapon_chams->callback = [&](bits &value)
	{ manage_chams_colors_cb(XOR_STR("local"), XOR_STR("weapon_chams"), value); };

	ADD_INLINE(
		"Weapon", weapon_chams,
		MAKE("visuals.local.chams.weapon_chams.primary_color", color_picker, cfg.local_visuals.weapon_chams.primary)
			->make_tooltip(XOR("Primary color"))
			->make_invisible(),
		MAKE("visuals.local.chams.weapon_chams.secondary_color", color_picker, cfg.local_visuals.weapon_chams.secondary)
			->make_tooltip(XOR("Secondary color"))
			->make_invisible());

	const auto arms_chams = MAKE("visuals.local.chams.arms_chams", combo_box, cfg.local_visuals.arms_chams.mode);
	arms_chams->add({MAKE("visuals.local.chams.arms_chams.disabled", selectable, XOR("Disabled")),
					 MAKE("visuals.local.chams.arms_chams.default", selectable, XOR("Default")),
					 MAKE("visuals.local.chams.arms_chams.flat", selectable, XOR("Flat")),
					 MAKE("visuals.local.chams.arms_chams.glow", selectable, XOR("Glow")),
					 MAKE("visuals.local.chams.arms_chams.pulse", selectable, XOR("Pulse")),
					 MAKE("visuals.local.chams.arms_chams.acryl", selectable, XOR("Acryl"))});

	arms_chams->callback = [&](bits &value) { manage_chams_colors_cb(XOR_STR("local"), XOR_STR("arms_chams"), value); };

	ADD_INLINE(
		"Arms", arms_chams,
		MAKE("visuals.local.chams.arms_chams.primary_color", color_picker, cfg.local_visuals.arms_chams.primary)
			->make_tooltip(XOR("Primary color"))
			->make_invisible(),
		MAKE("visuals.local.chams.arms_chams.secondary_color", color_picker, cfg.local_visuals.arms_chams.secondary)
			->make_tooltip(XOR("Secondary color"))
			->make_invisible());

	e->add(MAKE("other_spacing_pre", spacer, vec2(), vec2(0.f, 10.f), false));
	ADD_SECTION("Other", 250.f);

	ADD_INLINE("Glow", MAKE("visuals.local.chams.glow", checkbox, cfg.local_visuals.glow_local),
			   MAKE("visuals.local.chams.glow_color", color_picker, cfg.local_visuals.local_color_glow));
}

void visuals_local_preview(std::shared_ptr<layout> &e)
{
	e->direction = s_none;
	e->add(MAKE("visuals.local.preview.preview", ::gui::preview_model));
}
} // namespace menu::group