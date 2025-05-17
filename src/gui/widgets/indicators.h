#ifndef INDICATORS_FF90C222D93F427584591EDBAF59BDFF_H
#define INDICATORS_FF90C222D93F427584591EDBAF59BDFF_H

#include <gui/controls/widget.h>
#include <gui/misc.h>

namespace gui::widgets
{
using namespace evo::ren;
using namespace evo::gui;

constexpr auto indicators_id = GUI_HASH("widget.indicators");

class indicators : public widget
{
public:
	explicit indicators(value_param<vec2> &position)
		: widget(evo::gui::control_id{indicators_id, XOR_STR("widget.indicators")}, position, XOR_STR("Indicators"))
	{
		size.y = 130;
		size_anim.direct(size);
	}

	void reset() override;
	void render() override;
};
} // namespace gui::widgets

#endif // INDICATORS_FF90C222D93F427584591EDBAF59BDFF_H
