#ifndef BOMB_214BE43500664121B6435E6EEB063F8B_H
#define BOMB_214BE43500664121B6435E6EEB063F8B_H

#include <gui/controls/widget.h>
#include <gui/misc.h>

namespace gui::widgets
{
using namespace evo::ren;
using namespace evo::gui;

constexpr auto bomb_id = GUI_HASH("widget.bomb");

class bomb : public widget
{
public:
	explicit bomb(value_param<vec2> &position)
		: widget(evo::gui::control_id{bomb_id, XOR_STR("widget.bomb")}, position, XOR_STR("Bomb info"))
	{
		size = {300.f, 72.f};
		size_anim.direct(size);
	}

	void reset() override;
	void render() override;
};
} // namespace gui::widgets

#endif // BOMB_214BE43500664121B6435E6EEB063F8B_H
