#include <gui/controls/tab.h>
#include <gui/controls/tabs_layout.h>

GUI_NAMESPACE;

void tabs_layout::update_selected(std::shared_ptr<control> c)
{
	const auto s = c->as<tab>();
	if (!s)
		return;

	for_each_control(
		[&s](std::shared_ptr<control> &m)
		{
			const auto m_s = m->as<tab>();
			if (!m_s)
				return;

			(m_s->id != s->id) ? m_s->unselect() : s->select();
		});
}

void tabs_layout::reset_stack()
{
	if (auto_stretch)
	{
		const auto c_w = size.x / (direction == td_horizontal ? static_cast<float>(count()) : 1.f);
		for_each_control(
			[c_w](std::shared_ptr<control> &c)
			{
				c->margin = {};
				c->size.x = c_w;
			});
	}

	layout::reset_stack();
}

void tabs_layout::render()
{
	const auto r = area_abs().padding_bottom(direction == td_horizontal ? 0.f : -10.f);

	auto &d = draw.layers[ctx->content_layer];
	if (direction == td_vertical)
	{
		d->g.anti_alias = true;
		d->add_rect_filled_rounded(r, colors.base.gray_darker, 3.f, rnd_bl);
		d->g.anti_alias = false;

		d->add_line(r.tr(), r.br(), colors.base.gray);
	}

	layout::render();

	// :>
	if (direction == td_vertical)
	{
		d->g.anti_alias = true;
		d->add_shadow_line(r.height(7.f), shadow_down, .36f);
		d->g.anti_alias = false;
	}
}
