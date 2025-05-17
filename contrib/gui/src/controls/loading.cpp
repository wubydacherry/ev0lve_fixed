#include <gui/controls/loading.h>
#include <gui/gui.h>

GUI_NAMESPACE;

void loading::render()
{
	control::render();
	if (!is_visible)
		return;

	anim->animate();

	auto &d = draw.layers[ctx->content_layer];
	d->g.rotation = anim->value;
	d->g.texture = ctx->res.general.loader->obj;
	d->g.anti_alias = true;
	d->add_rect_filled(area_abs(), color::white());
	d->g.anti_alias = {};
	d->g.texture = {};
	d->g.rotation = {};
}
