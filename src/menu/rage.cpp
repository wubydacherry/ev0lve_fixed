
#include <base/cfg.h>
#include <menu/macros.h>
#include <menu/menu.h>

#include <gui/helpers.h>

USE_NAMESPACES;

void menu::group::rage_tab(std::shared_ptr<layout> &e)
{
	const auto side_bar = MAKE("rage.sidebar", tabs_layout, vec2{-10.f, -10.f}, vec2{200.f, 440.f}, td_vertical, true);
	side_bar->add(
		MAKE("rage.sidebar.general", child_tab, XOR("General"), GUI_HASH("rage.weapon_tab.general"))->make_selected());
	side_bar->add(MAKE("rage.sidebar.pistol", child_tab, XOR("Pistols"), GUI_HASH("rage.weapon_tab.pistol")));
	side_bar->add(
		MAKE("rage.sidebar.heavy_pistol", child_tab, XOR("Heavy pistols"), GUI_HASH("rage.weapon_tab.heavy_pistol")));
	side_bar->add(MAKE("rage.sidebar.automatic", child_tab, XOR("Automatic"), GUI_HASH("rage.weapon_tab.automatic")));
	side_bar->add(MAKE("rage.sidebar.awp", child_tab, XOR("AWP"), GUI_HASH("rage.weapon_tab.awp")));
	side_bar->add(MAKE("rage.sidebar.scout", child_tab, XOR("SSG-08"), GUI_HASH("rage.weapon_tab.scout")));
	side_bar->add(
		MAKE("rage.sidebar.auto_sniper", child_tab, XOR("Auto snipers"), GUI_HASH("rage.weapon_tab.auto_sniper")));

	side_bar->process_queues();
	side_bar->for_each_control([](std::shared_ptr<control> &c) { c->as<child_tab>()->make_vertical(); });

	const vec2 group_sz{284.f, 430.f};
	const vec2 group_half_sz = group_sz * vec2{1.f, 0.5f} - vec2{0.f, 5.f};
	const vec2 group_third_sz = group_sz * vec2{1.f, 0.33f} - vec2{0.f, 6.5f};

	const auto weapon_tabs = MAKE("rage.weapon_tabs", layout, vec2{}, group_half_sz, s_justify);
	weapon_tabs->make_not_clip();
	weapon_tabs->add(rage_weapon(XOR("general"), grp_general, group_half_sz, true));
	weapon_tabs->add(rage_weapon(XOR("pistol"), grp_pistol, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("heavy_pistol"), grp_heavypistol, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("automatic"), grp_automatic, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("awp"), grp_awp, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("scout"), grp_scout, group_half_sz));
	weapon_tabs->add(rage_weapon(XOR("auto_sniper"), grp_autosniper, group_half_sz));

	const auto groups = std::make_shared<layout>(evo::gui::control_id{GUI_HASH("rage.groups"), XOR_STR("rage.groups")},
												 vec2{200.f, 0.f}, vec2{580.f, 430.f}, s_justify);
	groups->add(tools::make_stacked_groupboxes(
		GUI_HASH("rage.groups.general_weapon"), group_sz,
		{make_groupbox(XOR("rage.general"), XOR("General"), group_half_sz, rage_general), weapon_tabs}));

	groups->add(tools::make_stacked_groupboxes(
		GUI_HASH("rage.groups.antiaim_fakelag_desync"), group_sz,
		{make_groupbox(XOR("rage.antiaim"), XOR("Anti-aim"), group_third_sz, rage_antiaim),
		 make_groupbox(XOR("rage.desync"), XOR("Desync"), group_third_sz, rage_desync),
		 make_groupbox(XOR("rage.fakelag"), XOR("Fakelag"), group_third_sz, rage_fakelag)}));

	e->add(side_bar);
	e->add(groups);
}

void menu::group::rage_general(std::shared_ptr<layout> &e)
{
	const auto mode = MAKE("rage.general.mode", combo_box, cfg.rage.mode);
	mode->add({
		MAKE("rage.general.mode.just_in_time", selectable, XOR("Just in time"))
			->make_tooltip(XOR("Regular ragebot mode")),
		MAKE("rage.general.mode.ahead_of_time", selectable, XOR("Ahead of time"))
			->make_tooltip(XOR("Increases chance of hitting a running player but disables backtracking")),
	});

	ADD_INLINE("Mode", mode, MAKE("rage.general.enable", checkbox, cfg.rage.enable));
	ADD("FOV", "rage.general.fov", slider<float>, 0.f, 180.f, cfg.rage.fov, XOR("%.0f°"));

	const std::vector<std::string> only_visible_aliases{XOR("autowall"), XOR("auto wall")};
	ADD_TOOLTIP_ALIASES("Target visible only", "rage.general.only_visible",
						"Disables autowall, shoots only visible enemies", checkbox, only_visible_aliases,
						cfg.rage.only_visible);

	const std::vector<std::string> auto_engage_aliases{XOR("autofire"), XOR("auto fire"), XOR("autostop"),
													   XOR("auto stop")};
	ADD_TOOLTIP_ALIASES("Auto engage", "rage.general.auto_engage", "Enables automatic fire and stop", checkbox,
						auto_engage_aliases, cfg.rage.auto_engage);
	ADD("Target dormant", "rage.general.dormant", checkbox, cfg.rage.dormant);

	const auto fast_fire_mode = MAKE("rage.general.fast_fire_mode", combo_box, cfg.rage.fast_fire);
	fast_fire_mode->add({
		MAKE("rage.general.fast_fire_mode.default", selectable, XOR("Default"))
			->make_tooltip(XOR("Default (offensive) fast fire mode")),
		MAKE("rage.general.fast_fire_mode.peek", selectable, XOR("Peek"))
			->make_tooltip(XOR("Peeking (defensive) fast fire mode")),
	});

	const std::vector<std::string> fast_fire_aliases{XOR("doubletap"), XOR("double tap"), XOR("rapidfire"),
													 XOR("rapid fire")};
	ADD_INLINE_ALIASES("Fast fire", fast_fire_aliases, fast_fire_mode,
					   MAKE("rage.general.fast_fire", checkbox, cfg.rage.enable_fast_fire));
	ADD("Hide shot", "rage.general.hide_shot", checkbox, cfg.rage.hide_shot);
	ADD("Fake duck", "rage.general.fake_duck", checkbox, cfg.rage.fake_duck);
	ADD_INLINE("Fake ping",
			   MAKE("rage.general.fake_ping", slider<float>, 0.f, 1000.f, cfg.rage.fake_ping_amount, XOR("%.0f ms")),
			   MAKE("rage.general.fake_ping_enable", checkbox, cfg.rage.fake_ping));

	const auto slow_walk = MAKE("rage.general.slow_walk", combo_box, cfg.rage.slowwalk_mode);
	slow_walk->add({
		MAKE("rage.general.slow_walk.optimal", selectable, XOR("Prefer optimal")),
		MAKE("rage.general.slow_walk.prefer_speed", selectable, XOR("Prefer speed")),
	});

	ADD_INLINE("Slow walk", slow_walk, MAKE("rage.general.slow_walk_enable", checkbox, cfg.rage.slowwalk));
}

void menu::group::rage_antiaim(std::shared_ptr<layout> &e)
{
	const auto disablers = MAKE("rage.antiaim.disablers", combo_box, cfg.antiaim.disablers);
	disablers->add({MAKE("rage.antiaim.disablers.round_end", selectable, XOR("Disable at round end")),
					MAKE("rage.antiaim.disablers.knife", selectable, XOR("Disable against knives")),
					MAKE("rage.antiaim.disablers.defuse", selectable, XOR("Disable when defusing")),
					MAKE("rage.antiaim.disablers.shield", selectable, XOR("Disable with riot shield"))});
	disablers->allow_multiple = true;
	ADD_INLINE("Mode", disablers, MAKE("rage.antiaim.enable", checkbox, cfg.antiaim.enable));

	const auto pitch = MAKE("rage.antiaim.pitch", combo_box, cfg.antiaim.pitch);
	pitch->add({MAKE("rage.antiaim.pitch.none", selectable, XOR("None")),
				MAKE("rage.antiaim.pitch.down", selectable, XOR("Down")),
				MAKE("rage.antiaim.pitch.up", selectable, XOR("Up"))});

	ADD_INLINE("Pitch", pitch);

	const auto yaw = MAKE("rage.antiaim.yaw", combo_box, cfg.antiaim.yaw);
	yaw->add({MAKE("rage.antiaim.yaw.view_angle", selectable, XOR("View angle")),
			  MAKE("rage.antiaim.yaw.at_target", selectable, XOR("At target")),
			  MAKE("rage.antiaim.yaw.freestanding", selectable, XOR("Freestanding"))});

	ADD_INLINE("Yaw", yaw);

	const auto yaw_override = MAKE("rage.antiaim.yaw_override", combo_box, cfg.antiaim.yaw_override);
	yaw_override->add({
		MAKE("rage.antiaim.yaw_override.none", selectable, XOR("None")),
		MAKE("rage.antiaim.yaw_override.left", selectable, XOR("Left")),
		MAKE("rage.antiaim.yaw_override.right", selectable, XOR("Right")),
		MAKE("rage.antiaim.yaw_override.back", selectable, XOR("Back")),
		MAKE("rage.antiaim.yaw_override.forward", selectable, XOR("Forward")),
	});

	ADD_INLINE("Yaw override", yaw_override);

	ADD("Yaw offset", "rage.antiaim.yaw_offset", slider<float>, -180.f, 180.f, cfg.antiaim.yaw_offset, XOR("%.0f°"));

	const auto yaw_modifier = MAKE("rage.antiaim.yaw_modifier", combo_box, cfg.antiaim.yaw_modifier);
	yaw_modifier->add({MAKE("rage.antiaim.yaw_modifier.none", selectable, XOR("None")),
					   MAKE("rage.antiaim.yaw_modifier.spin", selectable, XOR("Spin")),
					   MAKE("rage.antiaim.yaw_modifier.jitter", selectable, XOR("Jitter"))});

	ADD_INLINE("Yaw modifier", yaw_modifier);

	ADD("Yaw modifier amount", "rage.antiaim.yaw_modifier_amount", slider<float>, -180.f, 180.f,
		cfg.antiaim.yaw_modifier_amount, XOR("%.0f°"));

	const auto movement_mode = MAKE("rage.antiaim.movement_mode", combo_box, cfg.antiaim.movement_mode);
	movement_mode->add({
		MAKE("rage.antiaim.movement_mode.default", selectable, XOR("Default")),
		MAKE("rage.antiaim.movement_mode.skate", selectable, XOR("Skate")),
		MAKE("rage.antiaim.movement_mode.walk", selectable, XOR("Walk")),
		MAKE("rage.antiaim.movement_mode.opposite", selectable, XOR("Opposite")),
	});

	ADD_INLINE("Movement mode", movement_mode);
}

void menu::group::rage_fakelag(std::shared_ptr<layout> &e)
{
	const auto mode = MAKE("rage.fakelag.mode", combo_box, cfg.fakelag.mode);
	mode->add({MAKE("rage.fakelag.mode.none", selectable, XOR("None")),
			   MAKE("rage.fakelag.mode.dynamic", selectable, XOR("Dynamic")),
			   MAKE("rage.fakelag.mode.maximum", selectable, XOR("Maximum"))});
	ADD_INLINE("Mode", mode);
	ADD("Amount", "rage.fakelag.amount", slider<float>, 0.f, 14.f, cfg.fakelag.lag_amount, XOR("%.0f ticks"));
	ADD("Variance", "rage.fakelag.variance", slider<float>, 0.f, 14.f, cfg.fakelag.variance, XOR("%.0f ticks"));
}

void menu::group::rage_desync(std::shared_ptr<layout> &e)
{
	const auto foot_yaw = MAKE("rage.desync.foot_yaw", combo_box, cfg.antiaim.foot_yaw);
	foot_yaw->add({MAKE("rage.desync.foot_yaw.none", selectable, XOR("None")),
				   MAKE("rage.desync.foot_yaw.static", selectable, XOR("Static")),
				   MAKE("rage.desync.foot_yaw.opposite", selectable, XOR("Opposite")),
				   MAKE("rage.desync.foot_yaw.jitter", selectable, XOR("Jitter")),
				   MAKE("rage.desync.foot_yaw.freestanding", selectable, XOR("Freestanding"))});

	ADD_INLINE("Foot yaw", foot_yaw,
			   MAKE("rage.desync.foot_yaw_flip", checkbox, cfg.antiaim.foot_yaw_flip)->make_tooltip(XOR("Flip")));
	ADD_LINE("rage.desync.foot_yaw", MAKE("foot_yaw_label", label, XOR(" ")),
			 MAKE("rage.desync.foot_yaw_amount", slider<float>, 0.f, 180.f, cfg.antiaim.foot_yaw_amount, XOR("%.0f°")));
	ADD("Desync amount", "rage.desync.limit", slider<float>, 0.f, 60.f, cfg.antiaim.desync_limit, XOR("%.0f°"));

	const auto desync_triggers = MAKE("rage.desync.triggers", combo_box, cfg.antiaim.desync_triggers);
	desync_triggers->add(
		{MAKE("rage.desync.triggers.jump_impulse", selectable, XOR("On Jump Impulse")),
		 MAKE("rage.desync.triggers.avoid_extrapolation", selectable, XOR("Avoid Extrapolation")),
		 MAKE("rage.desync.triggers.avoid_lagcompensation", selectable, XOR("Avoid Lag Compensation"))});
	desync_triggers->allow_multiple = true;
	ADD_INLINE("Desync triggers", desync_triggers);

	const auto body_lean = MAKE("rage.desync.body_lean", combo_box, cfg.antiaim.body_lean);
	body_lean->add({MAKE("rage.desync.body_lean.none", selectable, XOR("None")),
					MAKE("rage.desync.body_lean.static", selectable, XOR("Static")),
					MAKE("rage.desync.body_lean.jitter", selectable, XOR("Jitter")),
					MAKE("rage.desync.body_lean.freestanding", selectable, XOR("Freestanding"))});
	ADD_INLINE("Body lean", body_lean,
			   MAKE("rage.desync.body_lean_flip", checkbox, cfg.antiaim.body_lean_flip)->make_tooltip(XOR("Flip")));
	ADD("Body lean amount", "rage.desync.body_lean_amount", slider<float>, 0.f, 100.f, cfg.antiaim.body_lean_amount,
		XOR("%.0f%%"));
	ADD("Ensure body lean", "rage.desync.ensure_body_lean", checkbox, cfg.antiaim.ensure_body_lean);
}

std::shared_ptr<layout> menu::group::rage_weapon(const std::string &group, int num, const vec2 &size, bool vis)
{
	auto group_container = MAKE_RUNTIME(XOR("rage.weapon_tab.") + group, layout, vec2{}, size, s_justify);
	group_container->make_not_clip();
	group_container->is_visible = vis;
	group_container->add(make_groupbox(
		XOR("rage.weapon.") + group, XOR("Weapon"), size - vec2{0.f, 4.f},
		[num, group](std::shared_ptr<layout> &e)
		{
			const auto group_n = XOR("rage.weapon.") + group + XOR(".");

			const auto auto_scope =
				MAKE_RUNTIME(group_n + XOR("auto_scope"), checkbox, cfg.rage.weapon_cfgs[num].auto_scope);
			ADD_INLINE("Auto scope", auto_scope);

			const auto secure_point =
				MAKE_RUNTIME(group_n + XOR("secure_point"), combo_box, cfg.rage.weapon_cfgs[num].secure_point);
			secure_point->add({
				MAKE_RUNTIME(group_n + XOR("secure_point.disabled"), selectable, XOR("Disabled"),
							 cfg_t::secure_point_disabled),
				MAKE_RUNTIME(group_n + XOR("secure_point.default"), selectable, XOR("Default"),
							 cfg_t::secure_point_default),
				MAKE_RUNTIME(group_n + XOR("secure_point.prefer"), selectable, XOR("Prefer"),
							 cfg_t::secure_point_prefer),
				MAKE_RUNTIME(group_n + XOR("secure_point.force"), selectable, XOR("Force"), cfg_t::secure_point_force),
			});

			const auto secure_point_enable =
				MAKE_RUNTIME(group_n + XOR("secure_fast_fire"), checkbox, cfg.rage.weapon_cfgs[num].secure_fast_fire);
			secure_point_enable->make_tooltip(XOR("Secure fast fire"));
			secure_point_enable->hotkey_type = hkt_none;

			ADD_INLINE("Secure point", secure_point, secure_point_enable);

			const auto delay_shot =
				MAKE_RUNTIME(group_n + XOR("delay_shot"), checkbox, cfg.rage.weapon_cfgs[num].delay_shot);
			delay_shot->make_tooltip(XOR("Delays shot while player is not in danger"));
			delay_shot->hotkey_type = hkt_none;
			ADD_INLINE("Delay shot", delay_shot);

			const auto dynamic =
				MAKE_RUNTIME(group_n + XOR("dynamic"), checkbox, cfg.rage.weapon_cfgs[num].dynamic_min_dmg);
			dynamic->make_tooltip(
				XOR("Dynamic mode. Attempts to deal as much damage as possible and lowers to set value over time"));
			dynamic->hotkey_type = hkt_none;

			ADD_RUNTIME("Hit chance", group_n + XOR("hit_chance"), slider<float>, 0.f, 100.f,
						cfg.rage.weapon_cfgs[num].hitchance_value, XOR("%.0f%%"));
			ADD_INLINE("Minimal damage",
					   MAKE_RUNTIME(group_n + XOR("minimal_damage"), slider<float>, 0.f, 100.f,
									cfg.rage.weapon_cfgs[num].min_dmg, XOR("%.0fhp")),
					   dynamic);

			const auto hitboxes = MAKE_RUNTIME(group_n + XOR("hitboxes"), combo_box, cfg.rage.weapon_cfgs[num].hitbox);
			hitboxes->allow_multiple = true;
			hitboxes->add({
				MAKE_RUNTIME(group_n + XOR("hitboxes.head"), selectable, XOR("Head")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.arms"), selectable, XOR("Arms")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.upper_body"), selectable, XOR("Upper body")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.lower_body"), selectable, XOR("Lower body")),
				MAKE_RUNTIME(group_n + XOR("hitboxes.legs"), selectable, XOR("Legs")),
			});

			ADD_INLINE("Hitboxes", hitboxes);
			const auto avoid_hitboxes =
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes"), combo_box, cfg.rage.weapon_cfgs[num].avoid_hitbox);
			avoid_hitboxes->allow_multiple = true;
			avoid_hitboxes->add({
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.head"), selectable, XOR("Head")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.arms"), selectable, XOR("Arms")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.upper_body"), selectable, XOR("Upper body")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.lower_body"), selectable, XOR("Lower body")),
				MAKE_RUNTIME(group_n + XOR("avoid_hitboxes.legs"), selectable, XOR("Legs")),
			});

			ADD_INLINE("Avoid insecure hitboxes", avoid_hitboxes);

			const auto lethal = MAKE_RUNTIME(group_n + XOR("lethal"), combo_box, cfg.rage.weapon_cfgs[num].lethal);
			lethal->allow_multiple = true;
			lethal->add({MAKE_RUNTIME(group_n + XOR("lethal.secure_points"), selectable, XOR("Force secure points")),
						 MAKE_RUNTIME(group_n + XOR("lethal.body_aim"), selectable, XOR("Force body aim")),
						 MAKE_RUNTIME(group_n + XOR("lethal.limbs"), selectable, XOR("Avoid limbs"))});

			ADD_INLINE("When lethal", lethal);

			ADD_RUNTIME("Multipoint", group_n + XOR("multipoint"), slider<float>, 0.f, 100.f,
						cfg.rage.weapon_cfgs[num].multipoint, XOR("%.0f%%"));

			if (num == grp_general)
				return;

			cfg.rage.weapon_cfgs[num].on_anything_changed = [group](bool status)
			{
				const auto box_deco = ctx->find<gui::group>(hash(XOR("rage.weapon.") + group));
				if (!box_deco)
					return;

				if (!status)
					box_deco->warning = XOR("Not configured. General config applies!");
				else
					box_deco->warning.clear();
			};
		}));

	return group_container;
}