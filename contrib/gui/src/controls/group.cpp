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
	d->add_rect_filled_rounded(header, colors.base.gray, 4.f, rnd_t);
	d->add_rect_filled_rounded(body, colors.base.gray_dark, 4.f, rnd_b);
	d->add_shadow_line(body.height(7.f), shadow_down, .1f);
	d->add_rect_rounded(area_abs(), colors.outlines.outline_dark, 4.f);
	d->font = draw.fonts[GUI_HASH("gui_bold")];
	//d->font = /*ctx->res.fonts.bold;*/ctx->res.fonts.main;
	d->add_text(header.tl() + vec2{12.f, 12.f}, text, colors.texts.enabled);

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
