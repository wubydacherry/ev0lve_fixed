
#ifndef GUI_PUBLIC_SOFTWARE_ITEM_H
#define GUI_PUBLIC_SOFTWARE_ITEM_H

#include <gui/control.h>

namespace gui
{
class public_software_item : public evo::gui::control
{
	using string_t = std::pair<std::string, char>;

public:
	public_software_item(evo::gui::control_id id, const string_t &name, const string_t &license, const string_t &url)
		: evo::gui::control(std::move(id), {}, {0.f, 40.f}), name(name), license(license), url(url)
	{
		size_to_parent_w = true;
		init();
	}

	void on_mouse_down(char key) override;
	void on_mouse_enter() override;
	void on_mouse_leave() override;

	void render() override;

private:
	void init();

	string_t name{};
	string_t license{};
	string_t url{};

	std::shared_ptr<evo::ren::anim_color> anim;
};
} // namespace gui

#endif // GUI_PUBLIC_SOFTWARE_ITEM_H
