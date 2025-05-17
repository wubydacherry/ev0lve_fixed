
#ifndef BASE_DRAW_MANAGER_H
#define BASE_DRAW_MANAGER_H

#include <detail/dx_adapter.h>
#include <gui_legacy/gui.h>

class draw_manager
{
public:
	using draw_impl = detail::dx_adapter;

	draw_manager();

	void draw(bool ontop = false);
	void reset() const;

	draw_impl adapter;

private:
	std::vector<std::shared_ptr<gui_legacy::drawable>> draw_list;
	std::vector<std::shared_ptr<gui_legacy::drawable>> draw_list_ontop;
};

extern draw_manager draw_mgr;

#endif // BASE_DRAW_MANAGER_H
