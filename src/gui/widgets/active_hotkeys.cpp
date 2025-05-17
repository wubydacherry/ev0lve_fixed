#include <base/cfg.h>
#include <base/game.h>
#include <gui/controls.h>
#include <gui/gui.h>
#include <gui/widgets/active_hotkeys.h>
#include <sdk/engine_client.h>

namespace gui::widgets
{
using namespace evo::gui;
using namespace evo::ren;

void active_hotkeys::render()
{
	max_alpha = cfg.indicators.alpha.get() / 100.f;

	widget::render();
	if (!is_visible)
		return;

	struct key_line
	{
		std::string name;
		std::string state;
	};

	const auto fnt = draw.fonts[GUI_HASH("gui_main")];

	// collect lines
	std::vector<key_line> lines{};
	float max_width{};
	for (const auto &[id, hk_weak] : ctx->hotkey_list)
	{
		if (hk_weak.expired())
			continue;

		// skip if no hotkeys/hidden
		const auto hk = hk_weak.lock();
		if (!hk->is_visible || hk->hotkey_type == hkt_none || hk->hotkeys.empty())
			continue;

		// skip if no value
		if (hk->type == evo::gui::ctrl_checkbox && !hk->as<checkbox>()->value.get())
			continue;

		// skip if no parent
		const auto p = hk->parent.lock();
		if (!p)
			continue;

		// skip if first element is not a label
		const auto first = p->as<control_container>()->at(0);
		if (!first || first->type != evo::gui::ctrl_label)
			continue;

		for (const auto &[hk_key, hk_val] : hk->hotkeys)
		{
			if (!hk_key)
				continue;

			const auto txt = first->as<label>()->text;
			std::string md{hk->hotkey_mode == hkb_hold ? XOR_STR("hold") : XOR_STR("toggle")};

			lines.emplace_back(key_line{txt, md});

			const auto w = fnt->get_text_size(txt).x + fnt->get_text_size(md).x + 40.f;
			if (max_width < w)
				max_width = w;
		}
	}

	max_width = std::clamp(max_width, 160.f, draw.display.x);

	// update visibility
	if ((lines.empty() && has_content || !game->engine_client->is_ingame()) && !is_forced_visibility)
	{
		has_content = false;
		size_anim.direct({160.f, 30.f});
		toggle_visibility(false);
		return;
	}

	if (!lines.empty() && !has_content && game->engine_client->is_ingame() || is_forced_visibility)
	{
		has_content = true;
		toggle_visibility(true);
	}

	if (alpha_anim.value == 0.f)
		return;

	// update size
	if (const auto h = ((float)lines.size() + 1.f) * 24.f + 10.f; h != size_anim.end.y)
		size_anim.direct({max_width, h});

	auto &d = draw.layers[ctx->content_layer];

	const auto old_alpha = d->g.alpha;
	const auto old_clip = d->g.clip_rect;
	d->g.alpha = alpha_anim.value;
	d->override_clip_rect(area_abs());
	d->font = fnt;

	// render lines
	float offset{32.f};
	for (const auto &[name, state] : lines)
	{
		d->add_text(pos_abs() + vec2{10.f, offset}, name, colors.text);
		d->add_text(pos_abs() + vec2(size.x - 10.f, offset), state, color(255, 255, 255, 160),
					text_params::with_h(align_right));
		offset += 24.f;
	}

	d->g.clip_rect = old_clip;
	d->g.alpha = old_alpha;
}
} // namespace gui::widgets
