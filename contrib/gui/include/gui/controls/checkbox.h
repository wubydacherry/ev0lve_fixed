#ifndef GUI_TESTER_CHECKBOX_H
#define GUI_TESTER_CHECKBOX_H

#include <functional>
#include <ren/renderer.h>

#include <gui/control.h>
#include <gui/gui.h>
#include <gui/input.h>

#include <gui/values.h>

namespace evo::gui
{
class checkbox : public control
{
public:
	checkbox(const control_id &id, value_param<bool> &v, const ren::vec2 &p = {})
		: control(id, p, {12.f, 12.f}), value(v)
	{
		using namespace ren;

		margin = {2.f, 2.f, 2.f, 2.f};
		hotkey_type = hkt_checkbox;

		anim = std::make_shared<anim_color>(colors.outline_light.mod_a(0), 0.1f);
		ctx->track_accent_anim(anim);

		type = ctrl_checkbox;
	}

	void reset() override;
	void reset_internal();
	void update_hotkey_table() override;
	void update_hotkey_value(hotkey_update upd, uint32_t v) override;

	void on_mouse_enter() override;
	void on_mouse_leave() override;
	void on_mouse_down(char key) override;
	void on_mouse_up(char key) override;

	void render() override;

	bool is_radio{};
	value_param<bool> &value;
	std::function<void(bool)> callback{};

private:
	std::shared_ptr<ren::anim_color> anim;
};
} // namespace evo::gui

#endif // GUI_TESTER_CHECKBOX_H
