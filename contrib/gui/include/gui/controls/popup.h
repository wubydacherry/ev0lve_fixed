#ifndef POPUP_694B3C5E6CFA46D0A1962EED742859C1_H
#define POPUP_694B3C5E6CFA46D0A1962EED742859C1_H

#include <gui/controls/control_container.h>
#include <gui/traits.h>

#include <utility>

namespace evo::gui
{
enum popup_type
{
	pt_none,
	pt_hotkey_popup
};

class popup : public control_container
{
public:
	popup(control_id id, const ren::vec2 &p, const ren::vec2 &s, bool is_action = false)
		: control_container(std::move(id), p, s)
	{
		is_window = true;
		is_action_popup = is_action;
		priority = ip_popup;

		create();
		type = ctrl_popup;
	}

	CALLBACK_TRAIT_RESET(control_container);
	CALLBACK_TRAIT_FRC(control_container);
	CALLBACK_TRAIT_ME(control_container);
	CALLBACK_TRAIT_ML(control_container);
	CALLBACK_TRAIT_MW(control_container);
	CALLBACK_TRAIT_KD(control_container);
	CALLBACK_TRAIT_KU(control_container);
	CALLBACK_TRAIT_TI(control_container);
	CALLBACK_TRAIT_DPC(control_container);

	void add(const std::shared_ptr<control> &c) override;

	void on_mouse_move(const ren::vec2 &p, const ren::vec2 &d) override;
	void on_mouse_down(char key) override;
	void on_mouse_up(char key) override;

	void open();
	void close();

	void begin_render();
	void render() override;
	void end_render();

	bool allow_move{false};
	popup_type p_type{pt_none};

private:
	void create();

	std::shared_ptr<ren::anim_float> anim;
	float old_alpha_c{}, old_alpha_s{};
};
} // namespace evo::gui

#endif // POPUP_694B3C5E6CFA46D0A1962EED742859C1_H
