
#include <base/cfg.h>
#include <hacks/skinchanger.h>
#include <menu/macros.h>
#include <menu/menu.h>

#include <gui/helpers.h>

USE_NAMESPACES;

void menu::group::skinchanger_select(std::shared_ptr<layout> &e)
{
	const auto selected_type = MAKE("skins.select.type", combo_box, cfg.skins.selected_type);
	selected_type->callback = [&](bits &value)
	{
		hacks::skin_changer->fill_weapon_list();

		// reset scroll if type changed
		const auto skin_list = ctx->find<list>(GUI_HASH("skins.select.skin_list"));
		if (!skin_list)
			return;
		skin_list->set_scroll(0);
	};
	selected_type->add({MAKE("skins.select.type.weapons", selectable, XOR("Weapons")),
						MAKE("skins.select.type.knife", selectable, XOR("Knife")),
						MAKE("skins.select.type.gloves", selectable, XOR("Gloves")),
						MAKE("skins.select.type.agents", selectable, XOR("Agents")),
						MAKE("skins.select.type.facemasks", selectable, XOR("Facemasks"))});
	ADD_INLINE("Item type", selected_type);
	const auto weapon_list = MAKE("skins.select.weapon_list", combo_box, cfg.skins.selected_entity);
	weapon_list->legacy_mode = true;
	weapon_list->callback = [&](bits &value)
	{
		const auto search_bar = ctx->find<text_input>(GUI_HASH("skins.select.search"));
		if (!search_bar)
			return;
		// clear search bar. This also refills the skin list
		search_bar->set_value("");

		// reset scroll if weapon changed
		const auto skin_list = ctx->find<list>(GUI_HASH("skins.select.skin_list"));
		if (!skin_list)
			return;
		skin_list->set_scroll(0);
	};
	ADD_INLINE("Item", weapon_list);

	const auto search_bar = MAKE("skins.select.search", text_input, vec2{}, vec2{220.f, 24.f});
	search_bar->placeholder = XOR("Search");
	search_bar->margin.mins = {2.f, 7.f};
	search_bar->callback = [](std::string &value)
	{ hacks::skin_changer->fill_skin_list(static_cast<int>(cfg.skins.selected_entity.get()), value); };
	ADD_LINE("SkinSearch", search_bar);

	const auto skin_list = MAKE("skins.select.skin_list", list, cfg.skins.selected_skin, vec2(), vec2(220.f, 240.f));
	skin_list->adjust_margin({2.f, 2.f, 2.f, 2.f});
	skin_list->legacy_mode = true;
	skin_list->callback = [&](bits &value)
	{
		hacks::skinchanger::update_gui_values();
		hacks::skinchanger::update_gui_components();

		const auto settings = ctx->find<layout>(GUI_HASH("skins.settings_content"));
		if (settings)
			settings->is_visible = true;
	};
	e->add(skin_list);

	const auto filter_by_item = MAKE("skins.select.filter_by_item", checkbox, cfg.skins.filter_by_item);
	filter_by_item->callback = [](bool checked)
	{
		const auto search_bar = ctx->find<text_input>(GUI_HASH("skins.select.search"));
		if (!search_bar)
			return;
		// clear search bar. This also refills the skin list
		search_bar->set_value("");
		const auto skin_list = ctx->find<list>(GUI_HASH("skins.select.skin_list"));
		if (!skin_list)
			return;
		skin_list->set_scroll(0);
	};

	ADD_INLINE("Filter by Item", filter_by_item);

	const auto filter_by_team = MAKE("skins.select.filter_by_team", checkbox, cfg.skins.filter_by_team);
	filter_by_team->callback = [](bool checked) { hacks::skinchanger::update_gui_components(); };

	ADD_INLINE("Filter by Team", filter_by_team);
}

void menu::group::skinchanger_settings(std::shared_ptr<layout> &e)
{
	ADD("Wear", "skins.settings.wear", slider<float>, 0.f, 1.f, cfg.skins.wear, XOR("%.2f"), .01f);
	const auto custom_seed = MAKE("skins.settings.custom_seed", checkbox, cfg.skins.custom_seed);
	custom_seed->callback = [](bool checked)
	{
		auto seed = ctx->find<layout>(GUI_HASH("skins.settings.seed_content"));
		if (!seed)
			return;

		if (seed->is_visible != checked)
			hacks::skinchanger::update_gui_components();
	};

	ADD_INLINE("Custom seed", custom_seed);
	const auto seed_text = MAKE("skins.settings.seed", text_input, vec2{}, vec2{100.f, 24.f},
								std::regex(XOR_STR("^(0|[1-9][0-9]{0,2}|1000)$")), 4);
	seed_text->placeholder = XOR("Seed number");
	ADD_LINE("skins.settings.seed", MAKE("seed_label", label, XOR(" ")), seed_text);

	const auto stattrak_type = MAKE("skins.settings.stattrak_type", combo_box, cfg.skins.stattrak_type);
	stattrak_type->callback = [&](bits &value) { hacks::skinchanger::update_gui_components(); };
	stattrak_type->add({MAKE("skins.settings.stattrak_type.off", selectable, XOR("Off")),
						MAKE("skins.settings.stattrak_type.static", selectable, XOR("Static")),
						MAKE("skins.settings.stattrak_type.dynamic", selectable, XOR("Dynamic"))});
	ADD_INLINE("StatTrak", stattrak_type);

	const auto stattrak_text = MAKE("skins.settings.stattrak", text_input, vec2{}, vec2{100.f, 24.f},
									std::regex(XOR_STR("^(0|[-]|[1-9][0-9]{0,9}|-1)$")), 10);
	ADD_LINE("skins.settings.stattrak", MAKE("stattrak_label", label, XOR(" ")), stattrak_text);

	auto equip_skin = [](int32_t team)
	{
		const auto definition_index = ctx->find<combo_box>(GUI_HASH("skins.select.weapon_list"));
		const auto paint_kit = ctx->find<list>(GUI_HASH("skins.select.skin_list"));
		const auto wear = ctx->find<slider<float>>(GUI_HASH("skins.settings.wear"));
		const auto seed = ctx->find<text_input>(GUI_HASH("skins.settings.seed"));
		const auto stattrak = ctx->find<text_input>(GUI_HASH("skins.settings.stattrak"));
		const auto dyn_stattrak = ctx->find<combo_box>(GUI_HASH("skins.settings.stattrak_type"));

		const auto definition_index_value = static_cast<int32_t>(definition_index->value.get());
		const auto paint_kit_value = static_cast<int32_t>(paint_kit->value.get());
		const auto wear_value = wear->value.get();
		const auto seed_value = seed->value;
		const auto stattrak_value = stattrak->value;
		const auto dyn_stattrak_value = dyn_stattrak->value.get().first_set_bit().value_or(0) == 2;

		const auto equipped_to = hacks::skin_changer->get_skin_equip_state(definition_index_value, paint_kit_value);

#if 1
		if (equipped_to == team || equipped_to == 1)
			hacks::skin_changer->unequip(definition_index_value, team);
		else
			hacks::skin_changer->equip(
				definition_index_value, team, paint_kit_value, wear_value,
				seed_value.empty() ? 1 : std::stoi(seed_value),
				stattrak_value.empty() ? (dyn_stattrak_value ? 0 : -1) : std::stoi(stattrak_value), dyn_stattrak_value);

		hacks::skin_changer->save_skins();
		hacks::skinchanger::update_gui_components();
#endif
	};

	const auto skin_equip_ct = MAKE("skins.settings.equip_ct", button, XOR("Equip CT"));
	skin_equip_ct->size.x = 220.f;
	skin_equip_ct->callback = [&]() { equip_skin(3); };

	const auto skin_equip_t = MAKE("skins.settings.equip_t", button, XOR("Equip T"));
	skin_equip_t->size.x = 220.f;
	skin_equip_t->callback = [&]() { equip_skin(2); };

	const auto skin_equip_both = MAKE("skins.settings.equip_both", button, XOR("Equip Both"));
	skin_equip_both->size.x = 220.f;
	skin_equip_both->callback = [&]() { equip_skin(1); };

	e->add(skin_equip_both);
	e->add(skin_equip_ct);
	e->add(skin_equip_t);
}

void menu::group::skinchanger_preview(std::shared_ptr<layout> &e) {}