#ifndef COMBO_BOX_POPUP_48512B0ABE7D47C39820E527E9FF330B_H
#define COMBO_BOX_POPUP_48512B0ABE7D47C39820E527E9FF330B_H

#include <gui/controls/combo_box.h>
#include <gui/controls/popup.h>

namespace evo::gui
{
class combo_box_popup : public popup
{
public:
	combo_box_popup(control_id id, const std::shared_ptr<combo_box> &p)
		: popup(std::move(id), p->area_abs().bl() + ren::vec2{0.f, 4.f}, {}, true), combo(p)
	{
	}

	void on_first_render_call() override;

private:
	std::shared_ptr<combo_box> combo{};
};
} // namespace evo::gui

#endif // COMBO_BOX_POPUP_48512B0ABE7D47C39820E527E9FF330B_H
