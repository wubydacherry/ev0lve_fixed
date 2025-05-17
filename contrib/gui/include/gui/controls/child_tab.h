#ifndef CHILD_TAB_99D389BCE8FA48AB83690534B479BCC6_H
#define CHILD_TAB_99D389BCE8FA48AB83690534B479BCC6_H

#include <gui/controls/tab.h>

namespace evo::gui
{
class child_tab : public tab
{
public:
	child_tab(const control_id &id, const std::string &t, uint64_t l, const std::shared_ptr<ren::texture> &i = {})
		: tab(id, t, l, i)
	{
		using namespace ren;

		size.y = 28.f;
		margin = {0.f, 2.f, 0.f, 2.f};

		anim_bg = std::make_shared<anim_color>(colors.base.gray_darker, 0.15f);
		shift_anim = std::make_shared<anim_float>(0.f, 0.15f);

		type = ctrl_child_tab;

		is_breadcrumb = true;
		breadcrumb_name = t;
	}

	void on_first_render_call() override;

	void select() override;
	void unselect() override;

	void render() override;

	std::shared_ptr<child_tab> make_vertical()
	{
		is_horizontal = false;
		return as<child_tab>();
	}

	bool is_horizontal{true};

private:
	std::shared_ptr<ren::anim_color> anim_bg;
	std::shared_ptr<ren::anim_float> shift_anim;
};
} // namespace evo::gui

#endif // CHILD_TAB_99D389BCE8FA48AB83690534B479BCC6_H
