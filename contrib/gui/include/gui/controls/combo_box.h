#ifndef COMBO_BOX_56F5835CBD604AAB9749A2702E6893AF_H
#define COMBO_BOX_56F5835CBD604AAB9749A2702E6893AF_H

#include <gui/controls/control_container.h>
#include <gui/values.h>

#include <utility>

namespace evo::gui
{
class combo_box : public control_container
{
public:
	combo_box(control_id id, value_param<bits> &v, const ren::vec2 &p = {})
		: control_container(std::move(id), p, {100.f, 20.f}), value(v)
	{
		should_process_children = false;
		margin = {2.f, 2.f, 2.f, 2.f};
		type = ctrl_combo_box;
	}

	void add(const std::shared_ptr<control> &c) override
	{
		control_container::add(c);
		if (!allow_multiple && !value.get().first_set_bit())
			value.get().set(0);
	}

	void add(const std::vector<std::shared_ptr<control>> &els)
	{
		for (const auto &e : els)
			add(e);
	}

	void on_mouse_down(char key) override;

	void reset() override;
	void render() override;

	value_param<bits> &value;
	bool allow_multiple{};
	std::function<void(bits &)> callback{};
	bool legacy_mode{};
};
} // namespace evo::gui

#endif // COMBO_BOX_56F5835CBD604AAB9749A2702E6893AF_H
