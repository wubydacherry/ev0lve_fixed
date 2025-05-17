#ifndef HOTKEY_POPUP_76156DEAB04744B18FE7773968C8A6A8_H
#define HOTKEY_POPUP_76156DEAB04744B18FE7773968C8A6A8_H

#include <gui/controls/popup.h>
#include <gui/values.h>

namespace evo::gui
{
class hotkey_popup : public popup
{
public:
	hotkey_popup(control_id id, const std::shared_ptr<control> &c)
		: popup(std::move(id), c->pos_abs(), {160.f, 160.f}, true), remote(c)
	{
		p_type = pt_hotkey_popup;
	}

	void on_first_render_call() override;
	void render() override;

private:
	void update_key_table();

	std::shared_ptr<control> remote{};
	std::vector<uint32_t> temp_keys{};

	value_param<bool> value_toggle{true};
	value_param<bool> value_hold{};
};
} // namespace evo::gui

#endif // HOTKEY_POPUP_76156DEAB04744B18FE7773968C8A6A8_H
