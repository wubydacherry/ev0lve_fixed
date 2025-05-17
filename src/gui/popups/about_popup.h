
#ifndef GUI_POPUPS_ABOUT_POPUP_H
#define GUI_POPUPS_ABOUT_POPUP_H

#include <gui/controls/popup.h>

namespace gui::popups
{
class about_popup : public evo::gui::popup
{
	using string_t = std::pair<std::string, char>;

public:
	explicit about_popup(evo::gui::control_id id) : evo::gui::popup(std::move(id), {}, {}) { allow_move = true; }

	void on_first_render_call() override;
	void render() override;

private:
	std::vector<std::tuple<string_t, string_t>> credits{
		{XOR_STR_STORE("raxer"), XOR_STR_STORE("Core / Rage / Visuals")},
		{XOR_STR_STORE("imi-tat0r"), XOR_STR_STORE("Core / Misc / Skins")},
		{XOR_STR_STORE("panzerfaust"), XOR_STR_STORE("Core / API / GUI")},
	};

	std::vector<std::tuple<string_t, string_t>> special_thanks{
		{XOR_STR_STORE("nardow"), XOR_STR_STORE("Design")},
		{XOR_STR_STORE("enQ"), XOR_STR_STORE("Design")},
		{XOR_STR_STORE("Rest"), XOR_STR_STORE("Design")},
		{XOR_STR_STORE("Alpha & Beta Team"), XOR_STR_STORE("Testing")},
	};
};
} // namespace gui::popups

#endif // GUI_POPUPS_ABOUT_POPUP_H
