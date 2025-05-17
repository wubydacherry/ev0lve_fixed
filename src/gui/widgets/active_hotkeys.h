#ifndef ACTIVE_HOTKEYS_82C4FBC0CAF04B8097E19F9150C0174B_H
#define ACTIVE_HOTKEYS_82C4FBC0CAF04B8097E19F9150C0174B_H

#include <gui/controls/widget.h>
#include <gui/misc.h>

namespace gui::widgets
{
using namespace evo::ren;
using namespace evo::gui;

constexpr auto active_hotkeys_id = GUI_HASH("widget.active_hotkeys");

class active_hotkeys : public widget
{
public:
	explicit active_hotkeys(value_param<vec2> &position)
		: widget(evo::gui::control_id{active_hotkeys_id, XOR_STR("widget.active_hotkeys")}, position,
				 XOR_STR("Active hotkeys"))
	{
	}

	void render() override;
};
} // namespace gui::widgets

#endif // ACTIVE_HOTKEYS_82C4FBC0CAF04B8097E19F9150C0174B_H
