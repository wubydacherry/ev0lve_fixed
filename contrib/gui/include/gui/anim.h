#ifndef GUI_TESTER_ANIM_H
#define GUI_TESTER_ANIM_H

#include <ren/renderer.h>
#include <ren/types/animator.h>

namespace evo::gui
{
class u_fc
{
public:
	u_fc(float fl, const ren::color &cl) : f(fl), c(cl) {}

	float f{};
	ren::color c{};
};

// shorthand for float + color animation
class anim_float_color : public ren::anim<u_fc>
{
public:
	anim_float_color(const float &f, const ren::color &c, float d, char i = ren::ease_linear)
		: ren::anim<u_fc>(u_fc(f, c), d, i)
	{
	}

	void animate() override;

	char type{};
};
} // namespace evo::gui

#endif // GUI_TESTER_ANIM_H
