#ifndef LOADING_65BFEF982D7840ECB1E8CF7850DEA6F6_H
#define LOADING_65BFEF982D7840ECB1E8CF7850DEA6F6_H

#include <gui/control.h>
#include <ren/misc.h>
#include <ren/renderer.h>

namespace evo::gui
{
class loading : public control
{
public:
	loading(const control_id &id, const ren::vec2 &p = {}) : control(id, p, {16.f, 16.f})
	{
		using namespace ren;

		anim = std::make_shared<anim_float>(0.f, 0.7f);
		anim->on_finished = [](const std::shared_ptr<ren::anim<float>> &f) { f->direct(0.f, 360.f); };
		anim->direct(0.f, 360.f);

		type = ctrl_loading;
	}

	void render() override;

private:
	std::shared_ptr<ren::anim_float> anim;
};
} // namespace evo::gui

#endif // LOADING_65BFEF982D7840ECB1E8CF7850DEA6F6_H
