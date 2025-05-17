#ifndef SDK_PANORAMA_H
#define SDK_PANORAMA_H

#include <sdk/cutlvector.h>
#include <sdk/v8.h>

namespace sdk
{
class ui_panel_t
{
public:
	VIRTUAL(9, get_id(), const char *(__thiscall *)(void *))();
	VIRTUAL(218, get_js_context_parent(), ui_panel_t *(__thiscall *)(void *))();
};

struct ui_panel_info_t
{
	char _pad_16[16];
	ui_panel_t *panel;
	char _pad_18[4];
};

class ui_engine_t
{
public:
	VIRTUAL(36, is_valid_panel_ptr(void *panel), bool(__thiscall *)(void *, void *))(panel);
	VIRTUAL(113, run_script(ui_panel_t *panel, const char *script),
			void *(__thiscall *)(void *, void *, const char *, const char *, int, int, int, bool))
	(panel, script, XOR_STR("panorama/layout/base.xml"), 8, 10, 0, false);
	VIRTUAL(129, get_isolate(), sdk::v8::isolate_t *(__thiscall *)(void *))();

	FUNC(game->panorama.at(functions::ui_engine::run_compiled_script),
		 run_compiled_script(void *unk, ui_panel_t *panel, void *script),
		 v8::local_t *(__thiscall *)(void *, void *, void *, void *, int, bool))
	(unk, panel, script, 0, false);
	FUNC(game->panorama.at(functions::ui_engine::compile), compile(ui_panel_t *panel, const char *script),
		 v8::local_t *(__thiscall *)(void *, void *, const char *, const char *))
	(panel, script, XOR_STR(""));
	FUNC(game->panorama.at(functions::ui_engine::get_panel_context), get_panel_context(ui_panel_t *panel),
		 v8::context_t ***(__thiscall *)(void *, ui_panel_t *))
	(panel);

	char _pad_27[240];
	cutlvector<ui_panel_info_t> panels;
};

class panorama_ui_engine_t
{
public:
	VIRTUAL(11, get_ui_engine(), ui_engine_t *(__thiscall *)(void *))();
};
} // namespace sdk

#endif // SDK_PANORAMA_H
