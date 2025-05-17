
#ifndef GUI_TESTER_LABEL_H
#define GUI_TESTER_LABEL_H

#include <gui/control.h>
#include <gui/controls/control_container.h>
#include <gui/gui.h>
#include <gui/misc.h>

#include <ren/misc.h>
#include <ren/renderer.h>

#include <utility>

namespace evo::gui
{
class label : public control
{
public:
	label(control_id id, const std::string &s, const ren::color &c = colors.text, bool b = false,
		  const ren::vec2 &p = {})
		: control(std::move(id), p, {}), text(s), col(c), bold(b)
	{
		using namespace ren;

		fnt = draw.fonts[bold ? uint64_t(GUI_HASH("gui_main")) : uint64_t(GUI_HASH("gui_main"))]; //gui_bold gui_main
		size = fnt->get_text_size(s);

		margin = {2.f, 2.f, 2.f, 2.f};
		type = ctrl_label;
	}

	explicit label(const std::string &s, const ren::color &c = colors.text, bool b = false, const ren::vec2 &p = {})
		: label({hash(s), s}, s, c, b, p)
	{
		type = ctrl_label;
	}

	void on_draw_param_changed(char w) override;
	void render() override;

	void set_text(const std::string &t)
	{
		text = t;
		process_draw_param_changes(dp_text);
	}

	std::string text{};
	ren::color col{};
	bool bold{};
	bool is_new{};

private:
	std::shared_ptr<ren::font_base> fnt{};
};
} // namespace evo::gui

#endif // GUI_TESTER_LABEL_H
