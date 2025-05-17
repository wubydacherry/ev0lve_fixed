
#ifndef GUI_POPUPS_PUBLIC_SOFTWARE_POPUP_H
#define GUI_POPUPS_PUBLIC_SOFTWARE_POPUP_H

#include <gui/controls/popup.h>

namespace gui::popups
{
class public_software_popup : public evo::gui::popup
{
	using string_t = std::pair<std::string, char>;

public:
	explicit public_software_popup(evo::gui::control_id id) : evo::gui::popup(std::move(id), {}, {})
	{
		allow_move = true;
	}

	void on_first_render_call() override;
	void render() override;

private:
	std::vector<std::tuple<string_t, string_t, string_t>> links{
		{XOR_STR_STORE("libressl"), XOR_STR_STORE("OpenSSL License"),
		 XOR_STR_STORE("https://github.com/ev0lve/libressl/blob/master/COPYING")},
		{XOR_STR_STORE("json"), XOR_STR_STORE("MIT License"),
		 XOR_STR_STORE("https://github.com/nlohmann/json/blob/develop/LICENSE.MIT")},
		{XOR_STR_STORE("gzip-hpp"), XOR_STR_STORE("BSD 2-Clause License"),
		 XOR_STR_STORE("https://github.com/mapbox/gzip-hpp/blob/master/LICENSE.md")},
		{XOR_STR_STORE("LuaJIT"), XOR_STR_STORE("MIT License"),
		 XOR_STR_STORE("https://github.com/LuaJIT/LuaJIT/blob/v2.1/COPYRIGHT")},
		{XOR_STR_STORE("miniz-cpp"), XOR_STR_STORE("MIT License"),
		 XOR_STR_STORE("https://github.com/ev0lve/miniz-cpp/blob/master/LICENSE.md")},
	};
};
} // namespace gui::popups

#endif // GUI_POPUPS_PUBLIC_SOFTWARE_POPUP_H
