#include <gui/controls/button.h>
#include <gui/controls/checkbox.h>
#include <gui/controls/hotkey.h>
#include <gui/controls/layout.h>
#include <gui/gui.h>
#include <gui/popups/hotkey_popup.h>

GUI_NAMESPACE;

void hotkey_popup::render()
{
	begin_render();

	const auto r = area_abs();

	// bounding box
	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);

	add_with_blur(d, r, [&r](auto &d) { d->add_rect_filled(r, color::white()); });
	d->add_rect_filled(r, colors.bg_bottom.mod_a(.8f));
	d->add_rect(r, colors.outline);

	// behavior descriptions
	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(r.tl() + vec2{25.f, 5.f}, XOR("Toggle"), color::white());
	d->add_text(r.tl() + vec2{25.f, 25.f}, XOR("Hold"), color::white());

	popup::render();
}

void hotkey_popup::on_first_render_call()
{
	popup::on_first_render_call();

	// shrink size if we can have only one hotkey
	if (!remote->allow_multiple_keys)
		size.y = 90.f;

	pos = calc_perfect_position(pos + vec2{0.f, remote->size.y + 4.f});

	// reserve up to 64 keys
	temp_keys.reserve(64);

	// copy hotkeys
	for (const auto &kv : remote->hotkeys)
		temp_keys.emplace_back(kv.first);

	// if empty, emplace one temporarily
	if (temp_keys.empty())
		temp_keys.emplace_back(0);

	if (!remote->allow_multiple_keys && temp_keys.size() > 1)
		temp_keys.resize(1);

	// determine current hotkey mode
	switch (remote->hotkey_mode)
	{
	case hkb_toggle:
		value_toggle = true;
		value_hold = false;
		break;
	case hkb_hold:
		value_toggle = false;
		value_hold = true;
		break;
	}

	// create togglers
	const auto beh_toggle =
		std::make_shared<checkbox>(control_id{GUI_HASH("active_hk_toggle")}, value_toggle, vec2{5.f, 5.f});
	const auto beh_hold =
		std::make_shared<checkbox>(control_id{GUI_HASH("active_hk_hold")}, value_hold, vec2{5.f, 25.f});

	beh_toggle->is_radio = true;
	beh_hold->is_radio = true;

	// radio mode for checkboxes, also updates remote control's hotkey mode
	beh_toggle->callback = [beh_hold, this](bool v)
	{
		if (v)
			remote->hotkey_mode = hkb_toggle;

		update_key_table();

		beh_hold->value = !v;
		beh_hold->reset_internal();
	};

	beh_hold->callback = [beh_toggle, this](bool v)
	{
		if (v)
			remote->hotkey_mode = hkb_hold;

		update_key_table();

		beh_toggle->value = !v;
		beh_toggle->reset_internal();
	};

	// force reset to apply values
	beh_toggle->reset();
	beh_hold->reset();

	add(beh_toggle);
	add(beh_hold);

	const auto keys_list = std::make_shared<layout>(control_id{GUI_HASH("active_hk_list")}, vec2{10.f, 50.f},
													vec2{140.f, 100.f}, s_vertical);

	auto c = 1;
	for (auto &k : temp_keys)
	{
		// create hotkey and place it in a list
		const auto hk = std::make_shared<hotkey>(control_id{id + c++}, &k);
		hk->callback = [this](uint32_t) { update_key_table(); };

		keys_list->add(hk);
	}

	keys_list->do_first_render_call();
	add(keys_list);

	// add insertion key if needed
	if (remote->allow_multiple_keys)
	{
		const auto add_btn = std::make_shared<button>(control_id{GUI_HASH("active_hk_add")}, XOR("Add"), nullptr,
													  vec2{100.f, 15.f}, vec2{40.f, 20.f});
		add_btn->callback = [this, keys_list]()
		{
			temp_keys.push_back(0);

			const auto hk = std::make_shared<hotkey>(control_id{id + temp_keys.size()}, &temp_keys.back());
			hk->callback = [this](uint32_t) { update_key_table(); };

			keys_list->add(hk);
		};

		add(add_btn);
	}

	ctx->active_popups.emplace_back(as<popup>());
	ctx->top_sort = sort;
}

void hotkey_popup::update_key_table()
{
	// TODO: implement value pull from target if type is not a checkbox

	remote->hotkeys.clear();
	for (const auto &k : temp_keys)
	{
		uint32_t value{};
		switch (remote->hotkey_type)
		{
		case hkt_checkbox:
			value = true;
			break;
		}

		remote->hotkeys[k] = value;
	}

	// make sure we got new hotkeys in the config
	remote->update_hotkey_table();
}
