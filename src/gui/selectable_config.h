#ifndef SELECTABLE_CONFIG_F71101E410A1415C8DDDB2E8CF1D1975_H
#define SELECTABLE_CONFIG_F71101E410A1415C8DDDB2E8CF1D1975_H

#include <gui/controls/selectable.h>

namespace gui
{
class selectable_config : public evo::gui::selectable
{
public:
	selectable_config(evo::gui::control_id id, const std::string &name, int remote_id = -1)
		: selectable(std::move(id), name), remote_id(remote_id)
	{
	}

	void render() override;

	int remote_id{};
};
} // namespace gui

#endif // SELECTABLE_CONFIG_F71101E410A1415C8DDDB2E8CF1D1975_H
