#ifndef MESSAGE_BOX_1B3311CE50234E10A87A4FC86E0CEBFA_H
#define MESSAGE_BOX_1B3311CE50234E10A87A4FC86E0CEBFA_H

#include <gui/controls/button.h>
#include <gui/controls/popup.h>
#include <ren/adapter.h>

namespace evo::gui
{
class message_box : public popup
{
public:
	message_box(control_id id, const std::string &title, const std::string &msg)
		: popup(std::move(id), {}, {}), title(title), message(msg)
	{
		allow_move = true;
	}

	void on_first_render_call() override;
	void render() override;

	std::string title{};
	std::string message{};
};
} // namespace evo::gui

#endif // MESSAGE_BOX_1B3311CE50234E10A87A4FC86E0CEBFA_H
