#ifndef PAYMENT_ADFF50B06FCF42ADA5778B89D09E0A32_H
#define PAYMENT_ADFF50B06FCF42ADA5778B89D09E0A32_H

#include <gui/controls/popup.h>
#include <gui/gui.h>
#include <network/helpers.h>

#include <utility>

namespace gui::popups
{
class payment_popup : public evo::gui::popup
{
public:
	explicit payment_popup(evo::gui::control_id id, std::string item_id, std::string script_name, int script_id)
		: evo::gui::popup(std::move(id), {}, {400.f, 200.f}), item_id(std::move(item_id)),
		  script_name(std::move(script_name)), script_id(script_id)
	{
		allow_move = true;
	}

	void on_first_render_call() override;
	void render() override;

	std::function<void(bool)> on_close;

private:
	std::string item_id{}, script_name{};
	int script_id{};

	std::shared_ptr<evo::ren::anim_float> spin;
	std::atomic_bool is_loading{}, is_final{}, is_error{true};
	std::string status{};

	network::item_data item{};
};
} // namespace gui::popups

#endif // PAYMENT_ADFF50B06FCF42ADA5778B89D09E0A32_H
