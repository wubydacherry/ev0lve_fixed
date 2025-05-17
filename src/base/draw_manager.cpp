
#include <base/debug_overlay.h>
#include <base/draw_manager.h>
#include <detail/player_list.h>
#include <hacks/esp.h>
#include <hacks/misc.h>
#include <hacks/visuals.h>

using namespace detail;

namespace hacks
{
std::shared_ptr<esp> es = std::make_shared<esp>();
std::shared_ptr<visuals> vis = std::make_shared<visuals>();
} // namespace hacks

#ifdef NDEBUG
std::shared_ptr<debug_interface> debug = std::make_shared<debug_interface>();
#else
std::shared_ptr<debug_interface> debug = std::make_shared<debug_overlay>();
#endif

draw_manager draw_mgr;

draw_manager::draw_manager()
{
	draw_list.emplace_back(hacks::es);
	draw_list.emplace_back(hacks::vis);

	draw_list_ontop.emplace_back(debug);
}

void draw_manager::draw(const bool ontop)
{
	player_list.refresh_local();
	for (const auto &item : (ontop ? draw_list_ontop : draw_list))
		item->draw(adapter);
}

void draw_manager::reset() const { adapter.reset(); }
