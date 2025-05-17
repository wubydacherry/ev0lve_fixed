
#ifndef NDEBUG

#include <base/game.h>
#include <gui/helpers.h>
#include <menu/macros.h>
#include <menu/menu.h>

USE_NAMESPACES;

void menu::group::debug_general(std::shared_ptr<layout> &e)
{
	ADD("Visualize matrices", "debug.matrices", slider<float>, 0.f, 64.f, game->dbg.visualized_matrices_index,
		XOR("%.0f"));
	ADD("Treat bots as real players", "debug.bots_as_real", checkbox, game->dbg.treat_bots_as_real);
	ADD("Allow fakelag on bot", "debug.bot_with_fakelag", checkbox, game->dbg.allow_fakelag_on_bot);
	ADD("Draw target shot matrix", "debug.target_shot_matrix", checkbox, game->dbg.draw_target_shot_matrix);
	ADD("Force resolver dir.", "debug.force_resolver_direction", slider<float>, -1.f, 7.f,
		game->dbg.force_resolver_direction, XOR("%.0f"));
	// ADD("Stepper test", "debug.stepper_test", slider<float>, 0.f, 150.f, game->dbg.stepper_test,
	//	ssa<float>{ssv<float>{150.f, XOR("Auto")}, ssv<float>{101.f, XOR("HP+%.0f")}, ssv<float>{0.f, XOR("%.0fhp")}});
}

#endif
