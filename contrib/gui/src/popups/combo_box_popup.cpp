#include <gui/controls/list.h>
#include <gui/popups/combo_box_popup.h>

GUI_NAMESPACE;

void combo_box_popup::on_first_render_call()
{
	popup::on_first_render_call();

	// create list
	const auto l = std::make_shared<list>(control_id{id + 1}, combo->value, vec2{}, vec2{160.f, 100.f});
	l->allow_multiple = combo->allow_multiple;
	l->callback = combo->callback;
	l->universal_callbacks = combo->universal_callbacks;
	l->legacy_mode = combo->legacy_mode;

	// enable auto-close if combo is not multi select
	if (!combo->allow_multiple)
	{
		l->callback = [&](bits &v)
		{
			if (combo->callback)
				combo->callback(v);

			for (const auto &[_, cbk_list] : combo->universal_callbacks)
			{
				for (const auto &cbk : cbk_list)
					cbk();
			}

			if (!ctx->active_popups.empty())
				ctx->active_popups.back()->close();
		};
	}

	// copy controls to list
	combo->copy(l);

	// force first render call to stack items and reset to update colors
	l->do_first_render_call();
	l->reset();

	// add to our container
	add(l);

	// set proper size
	size = {160.f, std::clamp(l->calc_max_y(), 0.f, 100.f)};
	l->size = size;

	pos = calc_perfect_position(pos);

	// set this as an active popup
	ctx->active_popups.emplace_back(as<popup>());
	ctx->top_sort = sort;
}
