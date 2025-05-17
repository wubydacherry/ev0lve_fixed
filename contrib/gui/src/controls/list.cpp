#include <gui/controls/list.h>
#include <gui/controls/selectable.h>

GUI_NAMESPACE;

void list::on_control_added(const std::shared_ptr<control> &c)
{
	const auto s = c->as<selectable>();
	if (s)
		s->is_odd = count() % 2 != 0;

	layout::on_control_added(c);
}

void list::update_selected(std::shared_ptr<control> c)
{
	const auto s = c->as<selectable>();
	if (!s)
		return;

	if (!allow_multiple)
		value.get().reset();

	auto n = 0;
	for_each_control(
		[&](std::shared_ptr<control> &m)
		{
			const auto m_s = m->as<selectable>();
			if (!m_s)
			{
				++n;
				return;
			}

			m_s->is_caring_about_hover = m_s->is_caring_about_parent = true;

			const auto v = m_s->value != -1 ? m_s->value : n;
			if (m_s->id != s->id)
			{
				if (!allow_multiple)
				{
					m_s->unselect();
					if (!legacy_mode)
						value->unset(v);
				}
			}
			else
			{
				if (allow_multiple)
				{
					s->is_selected ? s->unselect() : s->select();
					s->is_selected ? value->set(v) : value->unset(v);
				}
				else
				{
					s->select();

					if (!legacy_mode)
						value->set(v);
					else
						value = static_cast<bits>(v);
				}
			}

			n++;
		});

	if (callback)
		callback(value.get());

	run_universal_callbacks();
}

void list::reset()
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
				value->get(v) ? s->select() : s->unselect();
			else
				(static_cast<int>(value.get()) == s->value) ? s->select() : s->unselect();

			n++;
		});

	layout::reset();
}

void list::render()
{
	if (!is_visible)
		return;

	const auto r = area_abs();

	auto &l = draw.layers[ctx->content_layer];
	l->add_shadow_rect(r, 4.f);

	add_with_blur(l, r, [&r](auto &d) { d->add_rect_filled(r, color::white()); });
	l->add_rect_filled(r, colors.bg_bottom.mod_a(.8f));

	const auto old_clip = l->g.clip_rect;
	l->override_clip_rect(r);

	layout::render();

	l->g.clip_rect = old_clip;
	l->add_rect(r, colors.outline);

	if (show_spinner)
	{
		spinner_anim->animate();

		l->g.rotation = spinner_anim->value;
		l->g.texture = ctx->res.general.loader->obj;
		l->g.anti_alias = true;
		l->add_rect_filled({r.center() - vec2{8.f, 8.f}, r.center() + vec2{8.f, 8.f}}, colors.accent);
		l->g.anti_alias = {};
		l->g.texture = {};
		l->g.rotation = {};
	}
}
