#include <gui/controls/group.h>
#include <gui/gui.h>
#include <ren/renderer.h>

GUI_NAMESPACE;

void group::render()
{
	should_process_children = false;
	control_container::render();
	should_process_children = true;

	if (!is_visible)
		return;

	const auto r = area_abs();
	const auto header = r.height(32.f);
	const auto body = r.padding_top(32.f);

	auto &d = draw.layers[ctx->content_layer];

	// header
	d->add_rect_filled_rounded(header, colors.base.gray, 6.f, rnd_t);
	//header texture
	d->g.texture = ctx->res.general.pattern_group->obj;
	d->add_rect_filled(header, color::white().mod_a(0.45f));
	d->g.texture = {};
	//header line
	d->add_line(header.bl(), header.br(), colors.outline, 1.f);

	// body
	d->add_rect_filled_rounded(body, colors.base.gray, 6.f, rnd_b);

	//d->add_shadow_line(body.height(7.f), shadow_down, .1f);

	// outline
	//d->add_rect_rounded(area_abs(), colors.outlines.outline_dark, 6.f);

	// text
	d->font = draw.fonts[GUI_HASH("gui_bold")];
	d->add_text(header.tl() + vec2{10.f, 9.f}, text, colors.texts.enabled);

	for_each_control([](std::shared_ptr<control> &c) { c->render(); });

	// TODO: add out-of-bounds shadows
}

void group::add(const std::shared_ptr<control> &c)
{
	if (controls.empty())
		control_container::add(c);
	else
	{
		auto f = controls.front();
		if (f->is_container)
			f->as<control_container>()->add(c);
	}
}

void group::remove(uint64_t id)
{
	if (!controls.empty())
	{
		auto f = controls.front();
		if (f->is_container)
			f->as<control_container>()->remove(id);
	}
}

void group::remove(const std::shared_ptr<control> &c)
{
	if (!controls.empty())
	{
		auto f = controls.front();
		if (f->is_container)
			f->as<control_container>()->remove(c);
	}
}
