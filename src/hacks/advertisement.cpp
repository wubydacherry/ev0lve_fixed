
// clang-format off
#include <hacks/advertisement.h>
#include <sdk/global_vars_base.h>
#include <base/cfg.h>
#include <sdk/math.h>
#include <sdk/client_state.h>
// clang-format on

using namespace sdk;
using namespace util;

namespace hacks
{
advertising advert;

void advertising::clantag_changer()
{
	const auto set_clantag =
		reinterpret_cast<void(__fastcall *)(const char *, const char *)>(game->engine.at(functions::set_clantag));

	XOR_STR_STACK(dec, clantag);
	const std::string clantag_dec = dec;
	std::string clantag_temp;

	// init locals
	const int cur_time = int(TICKS_TO_TIME(game->client_state->server_tick) * 2);

	// don't run if it shouldn't
	if (cur_time == last_update_time)
		return;

	if (cfg.misc.clantag_changer.get())
	{
		// mark as enabled
		enabled = true;

		// reset iterators
		if (cur_time % (clantag_dec.size() * 3) == 0)
			iter_scroll = iter_build = iter_pause = 0;

		// build
		if (iter_build)
			clantag_temp = clantag_dec.substr(0, iter_build);
		// scroll
		if (iter_scroll)
			clantag_temp = clantag_dec.substr(iter_scroll, clantag_dec.size() - 1);

		if (iter_build < clantag_dec.size())
			iter_build++;
		else if (iter_pause < clantag_dec.size())
			iter_pause++;
		else if (iter_scroll < clantag_dec.size())
			iter_scroll++;

		clantag_temp = XOR_STR("\4\4\4") + clantag_temp;
		set_clantag(clantag_temp.c_str(), clantag_temp.c_str());
	}
	// was enabled
	else if (enabled)
	{
		// reset iterators
		iter_scroll = iter_build = iter_pause = 0;

		// set original clanid
		set_clantag(XOR_STR(""), XOR_STR(""));

		// mark as disabled
		enabled = false;
	}

	// adjust timer
	last_update_time = int(TICKS_TO_TIME(game->client_state->server_tick) * 2);
}
} // namespace hacks
