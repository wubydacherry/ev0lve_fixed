
#include <base/debug_overlay.h>
#include <gui/gui.h>
#include <sdk/cs_player.h>
#include <sdk/debug_overlay.h>
#include <sdk/engine_client.h>
#include <sdk/math.h>
#include <sdk/model_info_client.h>
#include <util/misc.h>

using namespace sdk;

void debug_interface::draw_player(cs_player_t *const player, const mat3x4 *mat, const color clr, const float duration,
								  std::optional<int32_t> only_hitbox) const
{
	if (!player->get_studio_hdr())
		return;

	const auto hdr = player->get_studio_hdr()->hdr;
	const auto set = hdr->get_hitbox_set(player->get_hitbox_set());

	for (auto i = 0; i < set->numhitboxes; i++)
	{
		const auto hitbox = set->get_hitbox(i);

		if (!hitbox || (only_hitbox.has_value() && *only_hitbox != i))
			continue;

		if (hitbox->radius > -1.f)
		{
			vec3 min, max;
			vector_transform(hitbox->bbmin, mat[hitbox->bone], min);
			vector_transform(hitbox->bbmax, mat[hitbox->bone], max);

			game->debug_overlay->capsule_overlay(min, max, hitbox->radius, clr.red(), clr.green(), clr.blue(),
												 clr.alpha(), duration);
		}
	}
}

void debug_overlay::draw(const gui_legacy::draw_adapter &draw)
{
	if (!game->engine_client->is_ingame())
		values_under_observation.clear();

	draw_observation_list(draw);
}

void debug_overlay::add_value_to_observation(const std::string description, const debug_variant &value)
{
	values_under_observation.insert_or_assign(description, value);
}

void debug_overlay::draw_observation_list(const gui_legacy::draw_adapter &draw) const
{
	static constexpr auto item_height = 16;

	if (values_under_observation.empty())
		return;

	const auto screen = evo::ren::draw.display;
	const auto table =
		std::make_pair(vec2(screen.x - 300, screen.y / 3),
					   vec2(screen.x - 50, screen.y / 3 + item_height * (values_under_observation.size() + 1)));

	const auto half = vec2(table.second.x - table.first.x, 0) / 2;
	draw.rect(table.first, table.second, color::white());
	draw.line(table.first + half, table.second - half, color::white());
	draw.line(table.first + vec2(0, item_height), vec2(table.second.x, table.first.y + item_height), color::white());
	draw.text(table.first + half / 2 + vec2(0, item_height / 2), color::white(), XOR_STR_UTF8("Description"),
			  gui_legacy::draw_adapter::default_font, gui_legacy::horizontal_alignment::center,
			  gui_legacy::vertical_alignment::center);
	draw.text(table.first + half * 1.5f + vec2(0, item_height / 2), color::white(), XOR_STR_UTF8("Value"),
			  gui_legacy::draw_adapter::default_font, gui_legacy::horizontal_alignment::center,
			  gui_legacy::vertical_alignment::center);

	auto height = item_height / 2.f;
	for (const auto &val : values_under_observation)
	{
		height += item_height;
		draw.text(table.first + vec2(half.x / 2.f, height), color::white(), util::utf8_decode(val.first).c_str(),
				  gui_legacy::draw_adapter::default_font, gui_legacy::horizontal_alignment::center,
				  gui_legacy::vertical_alignment::center);

		std::wstringstream value;
		std::visit(overloaded{[&](float arg) { value << std::setprecision(3) << arg; },
							  [&](uint32_t arg) { value << arg; }, [&](int32_t arg) { value << arg; },
							  [&](vec3 arg) {
								  value << std::setprecision(2) << arg.x << XOR_STR_UTF8(" - ") << arg.y
										<< XOR_STR_UTF8(" - ") << arg.z;
							  }},
				   val.second);
		draw.text(table.first + vec2(half.x * 1.5f, height), color::white(), value.str().c_str(),
				  gui_legacy::draw_adapter::default_font, gui_legacy::horizontal_alignment::center,
				  gui_legacy::vertical_alignment::center);
	}
}
