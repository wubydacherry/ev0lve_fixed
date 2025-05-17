#include <gui/controls/button.h>
#include <gui/controls/hsv_slider.h>
#include <gui/gui.h>
#include <gui/popups/color_picker_popup.h>

GUI_NAMESPACE;

void color_picker_popup::on_first_render_call()
{
	popup::on_first_render_call();

	pos = calc_perfect_position(pos + vec2{0.f, picker->size.y + 4.f});

	// create hsv picker
	const auto p =
		std::make_shared<hsv_slider>(control_id{GUI_HASH("cpp_hsv"), ""}, picker->value, picker->allow_alpha);
	p->callback = picker->callback;
	p->universal_callbacks = picker->universal_callbacks;

	add(p);

	// copy/paste buttons
	const auto copy =
		std::make_shared<button>(control_id{GUI_HASH("cpp_copy"), ""}, XOR(""), ctx->res.icons.copy, vec2{205.f, 5.f});
	copy->tooltip = XOR("Copy");
	copy->callback = [this]()
	{
		static const auto make_color_part = [](uint8_t col) -> std::string
		{
			char temp_col[3]{};

			sprintf_s(temp_col, XOR("%02x"), col);
			return temp_col;
		};

		const auto &picker_v = picker->value.get();

		std::string buf_text{};
		buf_text += XOR("#");
		buf_text += make_color_part(uint8_t(picker_v.value.r * 255.f));
		buf_text += make_color_part(uint8_t(picker_v.value.g * 255.f));
		buf_text += make_color_part(uint8_t(picker_v.value.b * 255.f));

		if (picker->allow_alpha)
			buf_text += make_color_part(uint8_t(picker_v.value.a * 255.f));

		clipboard::set(buf_text);
	};

	const auto paste = std::make_shared<button>(control_id{GUI_HASH("cpp_paste"), ""}, XOR(""), ctx->res.icons.paste,
												vec2{205.f, 37.f});
	paste->tooltip = XOR("Paste");
	paste->callback = [this]()
	{
		const auto buf_text = clipboard::get();

		uint32_t r{}, g{}, b{}, a{};
		const auto ret = sscanf_s(buf_text.c_str(), XOR("#%02x%02x%02x%02x"), &r, &g, &b, &a);
		if (ret < 3)
			return;

		auto &picker_v = picker->value.get();
		picker_v.value.r = float(r) / 255.f;
		picker_v.value.g = float(g) / 255.f;
		picker_v.value.b = float(b) / 255.f;

		if (picker->allow_alpha && ret == 4)
			picker_v.value.a = float(a) / 255.f;

		find<hsv_slider>(GUI_HASH("cpp_hsv"))->reset();
	};

	add(copy);
	add(paste);

	ctx->top_sort = sort;
	ctx->active_popups.emplace_back(as<popup>());
}

void color_picker_popup::render()
{
	if (!is_visible)
		return;

	begin_render();

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);

	add_with_blur(d, r, [&r](auto &d) { d->add_rect_filled(r, color::white()); });
	d->add_rect_filled(r, colors.bg_bottom.mod_a(.8f));
	d->add_rect(r, colors.outline);

	popup::render();
}
