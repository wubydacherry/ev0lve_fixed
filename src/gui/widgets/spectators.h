#ifndef SPECTATORS_1EEC88DC2F6C455E9908CDB659DA8CD4_H
#define SPECTATORS_1EEC88DC2F6C455E9908CDB659DA8CD4_H

#include <gui/controls/widget.h>
#include <gui/misc.h>
#include <ren/types/texture.h>

namespace gui::widgets
{
using namespace evo::ren;
using namespace evo::gui;

constexpr auto spectators_id = GUI_HASH("widget.spectators");

class spectators : public widget
{
public:
	explicit spectators(value_param<vec2> &position)
		: widget(evo::gui::control_id{spectators_id, XOR_STR("widget.spectators")}, position, XOR_STR("Spectators"))
	{
	}

	void render() override;

protected:
	std::unordered_map<uint64_t, std::shared_ptr<texture>> avatars;
};
} // namespace gui::widgets

#endif // SPECTATORS_1EEC88DC2F6C455E9908CDB659DA8CD4_H
