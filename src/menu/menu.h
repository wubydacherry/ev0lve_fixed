
#ifndef MENU_MENU_H
#define MENU_MENU_H

#include <gui/controls.h>
#include <gui/gui.h>

namespace menu
{
namespace tools
{
using namespace evo::gui;
using namespace evo::ren;

std::shared_ptr<control> make_stacked_groupboxes(uint64_t id, const vec2 &size,
												 const std::vector<std::shared_ptr<control>> &groups);
} // namespace tools

namespace group
{
using namespace evo::gui;
using namespace evo::ren;

void rage_tab(std::shared_ptr<layout> &e);
void rage_general(std::shared_ptr<layout> &e);
void rage_antiaim(std::shared_ptr<layout> &e);
void rage_fakelag(std::shared_ptr<layout> &e);
void rage_desync(std::shared_ptr<layout> &e);

std::shared_ptr<layout> rage_weapon(const std::string &group, int num, const vec2 &size, bool vis = false);

void visuals_tab(std::shared_ptr<layout> &e);
std::shared_ptr<layout> visuals_tab_other();
std::shared_ptr<layout> visuals_tab_enemy();
std::shared_ptr<layout> visuals_tab_team();
std::shared_ptr<layout> visuals_tab_local();

void visuals_enemy_esp(std::shared_ptr<layout> &e);
void visuals_enemy_chams(std::shared_ptr<layout> &e);
void visuals_enemy_preview(std::shared_ptr<layout> &e);
void visuals_team_esp(std::shared_ptr<layout> &e);
void visuals_team_chams(std::shared_ptr<layout> &e);
void visuals_team_preview(std::shared_ptr<layout> &e);
void visuals_local_viewmodel(std::shared_ptr<layout> &e);
void visuals_local_chams(std::shared_ptr<layout> &e);
void visuals_local_preview(std::shared_ptr<layout> &e);

void visuals_other_general(std::shared_ptr<layout> &e);
void visuals_other_world(std::shared_ptr<layout> &e);
void visuals_other_removals(std::shared_ptr<layout> &e);
void visuals_other_objects(std::shared_ptr<layout> &e);
void visuals_other_misc(std::shared_ptr<layout> &e);

void visuals_players(std::shared_ptr<layout> &e);
void visuals_local(std::shared_ptr<layout> &e);

void misc_general(std::shared_ptr<layout> &e);
void misc_settings(std::shared_ptr<layout> &e);
void misc_helpers(std::shared_ptr<layout> &e);
void misc_configs(std::shared_ptr<layout> &e);

void scripts_general(std::shared_ptr<layout> &e);
void scripts_elements(std::shared_ptr<layout> &e);

void skinchanger_select(std::shared_ptr<layout> &e);
void skinchanger_settings(std::shared_ptr<layout> &e);
void skinchanger_preview(std::shared_ptr<layout> &e);

#ifndef NDEBUG
void debug_general(std::shared_ptr<layout> &e);
#endif
} // namespace group

class menu_manager
{
public:
	void create();
	void toggle();
	void finalize();

	bool is_open();

	bool did_finalize{};
	bool can_toggle{};
	std::weak_ptr<evo::gui::window> main_wnd{};
};

inline menu_manager menu{};
} // namespace menu

#endif // MENU_MENU_H
