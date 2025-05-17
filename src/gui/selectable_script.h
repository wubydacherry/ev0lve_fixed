
#ifndef GUI_SELECTABLE_SCRIPT_H
#define GUI_SELECTABLE_SCRIPT_H

#ifdef CSGO_LUA

#include <gui/controls/selectable.h>
#include <lua/engine.h>

namespace gui
{
class selectable_script : public evo::gui::selectable
{
public:
	selectable_script(evo::gui::control_id id, lua::script_file &f) : selectable(std::move(id), f.get_name()), file(f)
	{
		if (f.metadata.description.has_value() || f.metadata.author.has_value())
		{
			if (f.metadata.description.has_value())
				tooltip += *f.metadata.description + XOR_STR(".");
			if (f.metadata.author.has_value())
			{
				if (!tooltip.empty())
					tooltip += XOR_STR(" ");

				tooltip += XOR_STR("Author: ") + *f.metadata.author;
			}
		}
	}

	void render() override;

	lua::script_file &file;
};
} // namespace gui

#endif // CSGO_LUA
#endif // GUI_SELECTABLE_SCRIPT_H
