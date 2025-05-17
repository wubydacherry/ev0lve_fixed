#ifndef GUI_TESTER_GROUP_H
#define GUI_TESTER_GROUP_H

#include <gui/control.h>
#include <gui/controls/control_container.h>

#include <utility>

namespace evo::gui
{
class group : public control_container
{
public:
	group(control_id id, const std::string &t, const ren::vec2 &p, const ren::vec2 &s)
		: control_container(std::move(id), p, s), text(t)
	{
		type = ctrl_group;

		is_breadcrumb = true;
		breadcrumb_name = t;
	}

	void add(const std::shared_ptr<control> &c) override;
	void remove(uint64_t id) override;
	void remove(const std::shared_ptr<control> &c) override;

	void render() override;

	std::string text{};
	std::string warning{};
};
} // namespace evo::gui

#endif // GUI_TESTER_GROUP_H
