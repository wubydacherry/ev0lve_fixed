#ifndef TABS_LAYOUT_C0D23430468D419BB7F42FBD14818466_H
#define TABS_LAYOUT_C0D23430468D419BB7F42FBD14818466_H

#include <gui/controls/layout.h>

#include <utility>

namespace evo::gui
{
enum tab_direction : char
{
	td_vertical = s_vertical,
	td_horizontal = s_horizontal
};

class tabs_layout : public layout
{
public:
	tabs_layout(control_id id, const ren::vec2 &p, const ren::vec2 &sz, char dir, bool auto_stretch = false)
		: layout(std::move(id), p, sz, dir), auto_stretch(auto_stretch)
	{
		if (!(int)sz.x)
			size_to_parent_w = true;

		type = ctrl_tabs_layout;
	}

	void update_selected(std::shared_ptr<control> c);
	void render() override;

protected:
	void reset_stack() override;

private:
	bool auto_stretch{false};
};
} // namespace evo::gui

#endif // TABS_LAYOUT_C0D23430468D419BB7F42FBD14818466_H
