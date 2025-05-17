#include <gui/controls/combo_box.h>
#include <gui/controls/selectable.h>
#include <gui/gui.h>
#include <gui/popups/combo_box_popup.h>

GUI_NAMESPACE;

void combo_box::render()
{
	control_container::render();

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);
	d->add_rect_filled(r, colors.bg_bottom);
	d->add_rect(r, colors.outline);

	const auto old_clip = d->g.clip_rect;
	d->override_clip_rect(r.padding_right(size.y));

	std::string text{};
	if (!empty())
	{
		const auto set = value.get().first_set_bit();
		if (allow_multiple)
		{
			if (set)
			{
				text.clear();

				auto n = 0;
				for_each_control(
					[&text, &n, this](std::shared_ptr<control> &m)
					{
						const auto m_s = m->as<selectable>();
						if (!m_s)
							return;

						const auto v = m_s->value != -1 ? m_s->value : n;
						if (value.get().test((char)v))
							text += (text.empty() ? "" : XOR(", ")) + m_s->text;

						n++;
					});
			}

			if (text.empty())
				text = XOR("None");
		}
		else
		{
			auto n = 0;
			for_each_control_logical(
				[&](std::shared_ptr<control> &m)
				{
					const auto m_s = m->as<selectable>();
					if (!m_s)
						return false;

					const auto v = m_s->value != -1 ? m_s->value : n;

					if ((legacy_mode && static_cast<int>(value.get()) == m_s->value) || (!legacy_mode && set == v))
					{
						text = m_s->text;
						return true;
					}

					n++;
					return false;
				});
		}
	}

	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(r.tl() + vec2{5.f, 1.f}, text, colors.text);
	d->g.clip_rect = old_clip;

	d->add_rect_filled_multicolor(
		r.padding_left(r.width() - size.y * 0.5f).height(r.height() - 2.f).translate({-size.y, 1.f}),
		{
			colors.bg_bottom.mod_a(0.f),
			colors.bg_bottom,
			colors.bg_bottom,
			colors.bg_bottom.mod_a(0.f),
		});

	d->g.texture = (!ctx->active_popups.empty() && ctx->active_popups.back()->id == id + 1) ? ctx->res.icons.up->obj
																							: ctx->res.icons.down->obj;
	d->add_rect_filled(r.padding_left(r.width() - size.y).shrink(4.f), color::white());
	d->g.texture = {};
}

void combo_box::on_mouse_down(char key)
{
	if (key == m_left)
	{
		const auto combo_popup =
			std::make_shared<combo_box_popup>(control_id{id + 1, ""}, shared_from_this()->as<combo_box>());
		combo_popup->open();
	}
}

void combo_box::reset()
{
	auto n = 0;
	for_each_control(
		[&](std::shared_ptr<control> &c)
		{
			const auto s = c->as<selectable>();
			if (!s)
				return;

			const auto v = s->value != -1 ? s->value : n;

			if (!legacy_mode)
				value->get((char)v) ? s->select() : s->unselect();
			else
				(static_cast<int>(value.get()) == s->value) ? s->select() : s->unselect();

			n++;
		});

	if (callback)
		callback(value.get());

	run_universal_callbacks();
}