#include <gui/controls.h>
#include <gui/gui.h>
#include <gui/popups/payment_popup.h>

#include <data/payments.h>

namespace gui::popups
{
using namespace evo::ren;
using namespace evo::gui;

void payment_popup::on_first_render_call()
{
	popup::on_first_render_call();

	pos = draw.display * 0.5f - size * 0.5f;
	spin = std::make_shared<anim_float>(0.f, 0.7f);
	spin->direct(0.f, 360.f);
	spin->on_finished = [](std::shared_ptr<evo::ren::anim<float>> f) { f->direct(0.f, 360.f); };

	const auto cancel_btn =
		std::make_shared<button>(evo::gui::control_id{GUI_HASH("ev0lve.payment.cancel")}, XOR_STR("Cancel"));
	cancel_btn->pos = {size.x * 0.5f - 105.f, size.y - 30.f};
	cancel_btn->is_visible = false;
	cancel_btn->callback = [&]()
	{
		if (on_close)
			on_close(!is_error);

		close();
	};

	const auto confirm_btn =
		std::make_shared<button>(evo::gui::control_id{GUI_HASH("ev0lve.payment.confirm")}, XOR_STR("Pay"));
	confirm_btn->pos = {size.x * 0.5f + 5.f, size.y - 30.f};
	confirm_btn->is_visible = false;
	confirm_btn->callback = [&]()
	{
		status = XOR_STR("Processing payment...");
		is_loading = true;

		if (ctx->find(GUI_HASH("ev0lve.payment.confirm")))
			ctx->find(GUI_HASH("ev0lve.payment.confirm"))->is_visible = false;

		const auto btn = ctx->find<button>(GUI_HASH("ev0lve.payment.cancel"));

		if (!btn)
			return;

		btn->is_visible = false;
		btn->text = XOR_STR("Close");
		btn->pos.x = size.x * 0.5f - 50.f;

#if 0
		std::thread(
			[&]()
			{
				const auto r = (evo::data::purchase_error)network::purchase_item(script_id, item_id);

				is_loading = false;
				is_final = true;
				is_error = r != evo::data::purchase_error::ok;
				if (ctx->find(GUI_HASH("ev0lve.payment.cancel")))
					ctx->find(GUI_HASH("ev0lve.payment.cancel"))->is_visible = true;

				switch (r)
				{
				case evo::data::purchase_error::ok:
					status = XOR_STR("Payment finished.");
					break;
				case evo::data::purchase_error::already_purchased:
					status = XOR_STR("Item was already purchased.");
					break;
				case evo::data::purchase_error::not_enough_points:
					status = XOR_STR("Not enough points.");
					break;
				case evo::data::purchase_error::system_error:
					status = XOR_STR("System error. Try again later.");
					break;
				}
			})
			.detach();
#endif
	};

	add(cancel_btn);
	add(confirm_btn);

	is_loading = true;
	status = XOR_STR("Loading info...");

	std::thread(
		[&]()
		{
			const auto i = std::optional<network::item_data>{};//std::optional<network::item_data>{{"data.name", 10}};//network::get_item_data(script_id, item_id);
			if (!i)
			{
				close();
				return;
			}

			item = *i;
			is_loading = false;

			if (ctx->find(GUI_HASH("ev0lve.payment.confirm")))
				ctx->find(GUI_HASH("ev0lve.payment.confirm"))->is_visible = true;

			if (ctx->find(GUI_HASH("ev0lve.payment.cancel")))
				ctx->find(GUI_HASH("ev0lve.payment.cancel"))->is_visible = true;
		})
		.detach();
}

void payment_popup::render()
{
	if (!is_visible)
		return;

	spin->animate();
	begin_render();

	const auto area = area_abs();
	const auto header = area_abs().height(24.f);
	const auto fnt = draw.fonts[GUI_HASH("gui_main")];
	const auto fnt_bold = draw.fonts[GUI_HASH("gui_bold")];

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(area, 12.f, true, 0.25f);
	d->g.anti_alias = true;

	add_with_blur(d, area, [&area](auto &d) { d->add_rect_filled_rounded(area, color::white(), 4.f); });
	d->add_rect_filled_rounded(area, colors.bg_bottom.mod_a(.8f), 4.f);
	d->add_rect_filled_rounded(header, colors.bg_block, 4.f, rnd_t);
	d->g.anti_alias = false;

	d->font = fnt_bold;
	d->add_text(header.center(), XOR("Payment"), colors.text, text_params::with_vh(align_center, align_center));
	d->add_line(header.bl(), header.br(), colors.accent);

	if (is_loading || is_final)
	{
		const auto center = area.center() + vec2{0.f, 0.f};
		if (!is_final)
		{
			d->g.rotation = spin->value;
			d->g.texture = ctx->res.general.loader->obj;
			d->add_rect_filled({center - 8.f, center + 8.f}, colors.accent);
			d->g.texture = {};
			d->g.rotation = {};
		}
		else
		{
			d->g.texture =
				draw.textures[is_error ? (uint64_t)GUI_HASH("icon_error") : (uint64_t)GUI_HASH("icon_success")]->obj;
			d->add_rect_filled(rect{center - 24.f, center + 24.f}.margin_bottom(8.f), colors.text);
			d->g.texture = {};
		}

		if (!status.empty())
		{
			d->font = fnt;
			d->add_text(center + vec2{0.f, 32.f}, status, colors.text,
						text_params::with_vh(align_center, align_center));
		}
	}
	else
	{
		auto cur = area.tl() + vec2{16.f, 32.f};
		d->font = fnt;
		d->add_text(cur, script_name + XOR_STR(" wants to purchase:"), colors.text);
		cur.y += 24.f;
		d->font = fnt_bold;
		d->add_text(cur, item.name, colors.text);
		cur.y += 48.f;

		d->font = fnt;
		d->add_text(cur, XOR_STR("Total: "), colors.text);
		cur.x += 64.f;
		d->font = fnt_bold;
		d->add_text(cur, std::to_string(item.price) + XOR_STR(" ev$"), colors.accent);
		cur.y += 24.f;
		cur.x -= 64.f;
		d->font = fnt;
		d->add_text(cur,
					XOR_STR("NOTE: This payment cannot be refunded. If you experience problems\nwith this item, "
							"please contact script developer."),
					colors.text_dark);
		cur.x += 64.f;
	}

	popup::render();
}
} // namespace gui::popups
