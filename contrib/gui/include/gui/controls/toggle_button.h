#ifndef TOGGLE_BUTTON_33FCC1D522FB4352AC01A8959DB3515F_H
#define TOGGLE_BUTTON_33FCC1D522FB4352AC01A8959DB3515F_H

#include <functional>
#include <ren/renderer.h>

#include <gui/control.h>
#include <gui/gui.h>
#include <gui/values.h>

namespace evo::gui
{
class toggle_button : public control
{
public:
	toggle_button(const control_id &id, value_param<bool> &v, const std::shared_ptr<ren::texture> &x = {},
				  const ren::vec2 &p = {}, const ren::vec2 &s = {30.f, 30.f})
		: control(id, p, s), tex(x), value(v)
	{
		using namespace ren;
		anim = std::make_shared<anim_color>(colors.text, 0.15f);

		margin = {};
		ctx->track_accent_anim(anim);

		spinner_anim = std::make_shared<anim_float>(0.f, 0.7f);
		spinner_anim->on_finished = [](const std::shared_ptr<ren::anim<float>> &f) { f->direct(0.f, 360.f); };
		spinner_anim->direct(0.f, 360.f);

		type = ctrl_toggle_button;
	}

	void reset() override;
	void reset_internal();
	void on_mouse_enter() override;
	void on_mouse_leave() override;
	void on_mouse_down(char key) override;

	void render() override;

	bool show_spinner{};
	std::shared_ptr<ren::texture> tex{};
	ren::vec2 icon_size{};
	value_param<bool> &value;
	std::function<void(bool)> callback{};

private:
	std::shared_ptr<ren::anim_float> spinner_anim{};
	std::shared_ptr<ren::anim_color> anim;
};
} // namespace evo::gui

#endif // TOGGLE_BUTTON_33FCC1D522FB4352AC01A8959DB3515F_H
