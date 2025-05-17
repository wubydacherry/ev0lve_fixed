
#ifndef SDK_CLIENT_MODE_H
#define SDK_CLIENT_MODE_H

#include <sdk/color.h>

namespace sdk
{
typedef void base_hud_chat_input_line;
typedef void hud_chat_history;
typedef void hud_chat_filter_button;
typedef void hud_chat_filter_panel;
typedef void base_hud_chat_line;

struct csgo_hud_chat_t
{
	char pad[0x60];
	bool is_open;
};

struct base_hud_chat_t
{
	template <typename... T>
	VIRTUAL(27, output_chat(int player_index, int filter, const char *format, T... args),
			void(__cdecl *)(void *, int, int, const char *, ...))
	(player_index, filter, format, args...);

	base_hud_chat_line *find_unused_chat_line() const { return chat_line; }

	char pad_0004[452]{};
	float history_fade_time{};
	float history_idle_time{};
	base_hud_chat_input_line *chat_input{};
	base_hud_chat_line *chat_line{};
	int32_t font_height{};
	hud_chat_history *chat_history{};
	hud_chat_filter_button *filters_button{};
	hud_chat_filter_panel *filter_panel{};
	color color_custom;
	int32_t message_mode{};
	int32_t visible_height{};
	uint32_t chat_font{};
	int32_t filter_flags{};
	bool entering_voice{};
};

struct client_mode_t
{
	char pad_0000[16];
	float replay_start_record_time;
	float replay_stop_record_time;
	void *replay_reminder_panel;
	base_hud_chat_t *chat_element;
	unsigned long cursor_none;
	void *weapon_selection;
	int root_size[2];
};
} // namespace sdk

#endif // SDK_CLIENT_MODE_H
