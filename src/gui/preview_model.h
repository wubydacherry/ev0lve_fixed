#ifndef PREVIEW_MODEL_F7CB2C5109984A0BAE3A195B0CDA0742_H
#define PREVIEW_MODEL_F7CB2C5109984A0BAE3A195B0CDA0742_H

#include <gui/control.h>

namespace gui
{
class preview_model : public evo::gui::control
{
public:
	preview_model(evo::gui::control_id id) : evo::gui::control(std::move(id), {}, {})
	{
		size_to_parent_w = true;
		size_to_parent_h = true;
	}

	void on_mouse_move(const evo::ren::vec2 &p, const evo::ren::vec2 &d) override;
	void on_mouse_down(char key) override;
	void on_mouse_up(char key) override;

	void render() override;
};
} // namespace gui

#endif // PREVIEW_MODEL_F7CB2C5109984A0BAE3A195B0CDA0742_H
