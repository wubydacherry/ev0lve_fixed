#include <network/helpers.h>
#include <network/load_indicator.h>
#include <ren/adapter.h>
#include <ren/renderer.h>

#include <base/cfg.h>
#include <base/game.h>
#include <gui/gui.h>
#include <menu/menu.h>
#include <sdk/surface.h>

#include <sdk/global_vars_base.h>

namespace network
{
using namespace evo::ren;
using namespace evo::gui;

load_indicator load_ind;

void load_indicator::begin()
{
	alpha = std::make_shared<anim_float>(0.f, 0.5f, ease_linear);
	spinner = std::make_shared<anim_float>(0.f, 1.f, ease_linear);

	alpha->direct(1.f);

	spinner->direct(360.f);
	spinner->on_finished = [&](const std::shared_ptr<anim<float>> &a)
	{
		if (did_begin)
			spinner->direct(0.f, 360.f);
	};

	did_begin = true;
}

void load_indicator::end()
{
	is_ending = true;

	if (game->globals->realtime - logo_time >= logo_fps && logo_i < 70)
	{
		++logo_i;
		logo_time = game->globals->realtime;
	}

	if (logo_i == 70)
	{
		alpha->direct(0.f);
		alpha->on_finished = [&](const std::shared_ptr<anim<float>> &a)
		{
			std::thread(
				[&]()
				{
					EMIT_SUCCESS_SOUND();

					did_begin = false;

					first_frame = nullptr;
					logo_frames.clear();

					alpha = nullptr;
					spinner = nullptr;

					ctx->reset();
					menu::menu.can_toggle = true;
				})
				.detach();
		};
	}
}

void load_indicator::render()
{
	if (!did_begin)
		return;
	is_ending = true;
	auto &d = draw.layers[63];
	d->g.alpha = alpha->value;

	const auto a = rect(0.f, 0.f).size(draw.display);
	add_with_blur(d, a, [&a](auto &d) { d->add_rect_filled(a, color::white()); });
	d->add_rect_filled(a, color(0.f, 0.f, 0.f, 0.7f));

	if (first_frame && !is_ending)
	{
		d->g.texture = first_frame->obj;
		d->g.rotation = spinner->value;
		d->add_rect_filled(rect(draw.display * 0.5f - vec2({64.f, 64.f})).size({128.f, 128.f}), color::white());
		d->g.rotation = {};
		d->g.texture = {};
	}

	// render progress bar
	if (!is_ending)
	{
		const auto bar_size = vec2(250.f, 4.f);
		const auto bar_rect =
			rect(vec2(0.f, 0.f), bar_size).translate(draw.display * 0.5f - bar_size * 0.5f + vec2{0.f, 136.f});

		d->add_rect_filled(bar_rect, color(18, 18, 18, 255));

		// calculate fill
		auto fill = (float)current_section / (float)sec_max;
		fill += section_progress / (float)sec_max;

		d->add_rect_filled(bar_rect.width(bar_rect.width() * fill), color(0, 172, 245, 255));
	}

	if (is_ending)
	{
		if (logo_i < 70)
			end();

		d->g.texture = logo_frames[logo_i >= 70 ? 69 : logo_i]->obj;
		d->add_rect_filled(rect(draw.display * 0.5f - vec2({64.f, 64.f})).size({128.f, 128.f}), color::white());
		d->g.texture = {};
	}

	d->g.alpha = {};

	alpha->animate();
	spinner->animate();

	perform_task();
}

void load_indicator::perform_task()
{
	if (current_section == sec_max)
		return;

	if (current_section == sec_resource_load && !did_finish_section(sec_resource_load))
	{
		//load_required_resources();
		finished_sections.emplace_back(sec_resource_load);
		return;
	}

	if (current_section == sec_script_load && !did_finish_section(sec_script_load))
	{
#ifdef CSGO_LUA
		// refresh available scripts
		const auto refresh = ctx->find<button>(GUI_HASH("scripts.general.refresh"));
		if (refresh)
			refresh->callback();
#endif

		finished_sections.emplace_back(sec_script_load);
		return;
	}

	if (current_section == sec_config_load && !did_finish_section(sec_config_load))
	{
		// refresh available configs
		const auto refresh = ctx->find<button>(GUI_HASH("misc.configs.refresh"));
		if (refresh)
			refresh->callback();

		finished_sections.emplace_back(sec_config_load);
		return;
	}
}

void load_indicator::begin_next_section()
{
	section_progress = 0.f;

	if (current_section + 1 >= sec_max)
	{
		current_section = sec_max;
		end();
		return;
	}

	current_section = (section)((int)current_section + 1);
}

bool load_indicator::did_finish_section(section s) const
{
	return std::find(finished_sections.begin(), finished_sections.end(), s) != finished_sections.end();
}
} // namespace network