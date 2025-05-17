#include <base/cfg.h>
#include <base/game.h>
#include <gui/controls.h>
#include <gui/gui.h>
#include <gui/widgets/spectators.h>
#include <sdk/cs_player.h>
#include <sdk/engine_client.h>
#include <sdk/steamapi.h>

namespace gui::widgets
{
using namespace evo::gui;
using namespace evo::ren;

void spectators::render()
{
	max_alpha = cfg.indicators.alpha.get() / 100.f;

	widget::render();
	//if (!is_visible)
	return;
}
#if 0
	struct user_info
	{
		uint64_t xuid;
		std::string name;
	};

	const auto fnt = draw.fonts[GUI_HASH("gui_main")];
	const auto lp =
		(sdk::cs_player_t *)game->client_entity_list->get_client_entity(game->engine_client->get_local_player());

	// collect observers
	std::vector<user_info> lines{};
	float max_width{};
	if (game->engine_client->is_ingame() && lp && lp->is_alive())
	{
		game->client_entity_list->for_each_player(
			[&](sdk::cs_player_t *player)
			{
				const auto obs_ent =
					game->client_entity_list->get_client_entity_from_handle(player->get_observer_target());
				if (obs_ent != lp || player->is_dormant())
					return;

				const auto info = game->engine_client->get_player_info(player->index());
				const auto w = fnt->get_text_size(info.name).x + 60.f;
				if (max_width < w)
					max_width = w;

				lines.emplace_back(user_info{(uint64_t)info.steam_id64, info.name});

				// maintain texture
				if (!avatars.contains(info.steam_id64))
				{
					const auto avatar_id =
						game->steam_context->friends->get_small_friend_avatar(sdk::steam_id_t(info.steam_id64));
					if (!avatar_id)
						return;

					uint32_t width{}, height{};
					if (!game->steam_context->utils->get_image_size(avatar_id, &width, &height))
						return;

					std::vector<uint8_t> data(width * height * 4);
					if (!game->steam_context->utils->get_image_rgba(avatar_id, data.data(), (int)data.size()))
						return;

					const auto &tex = avatars[info.steam_id64] =
						std::make_shared<texture>(data.data(), width, height, width * 4);
					tex->create();
				}
			});
	}

	// destroy unused textures
	for (auto it = avatars.begin(); it != avatars.end();)
	{
		if (std::find_if(lines.begin(), lines.end(), [&](const user_info &info) { return info.xuid == it->first; }) ==
			lines.end())
			it = avatars.erase(it);
		else
			++it;
	}

	max_width = std::clamp(max_width, 160.f, 400.f);

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
	for (const auto &[xuid, name] : lines)
	{
		if (avatars.contains(xuid))
		{
			d->g.texture = avatars[xuid]->obj;
			d->g.anti_alias = true;
			d->add_circle_filled(pos_abs() + vec2{20.f, offset + 10.f}, 10.f, color::white(), 32);
			d->g.texture = {};
			d->g.anti_alias = false;
		}

		d->add_text(pos_abs() + vec2{38.f, offset}, name, color::white());
		offset += 24.f;
	}

	d->g.clip_rect = old_clip;
	d->g.alpha = old_alpha;
}
#endif
} // namespace gui::widgets