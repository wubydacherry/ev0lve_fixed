#ifndef COLOR_PICKER_POPUP_F0496442B54C4CBA91AFA1FD03DBD410_H
#define COLOR_PICKER_POPUP_F0496442B54C4CBA91AFA1FD03DBD410_H

#include <gui/controls/color_picker.h>
#include <gui/controls/popup.h>

namespace evo::gui
{
class color_picker_popup : public popup
{
public:
	color_picker_popup(control_id id, const std::shared_ptr<color_picker> &picker)
		: popup(std::move(id), picker->pos_abs(), {240.f, picker->allow_alpha ? 200.f : 160.f}, true), picker(picker)
	{
	}

	void on_first_render_call() override;
	void render() override;

private:
	std::shared_ptr<color_picker> picker{};
};
} // namespace evo::gui

#endif // COLOR_PICKER_POPUP_F0496442B54C4CBA91AFA1FD03DBD410_H
