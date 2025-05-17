
#ifndef BASE_EVENT_LOG_H
#define BASE_EVENT_LOG_H

#include <base/debug_overlay.h>
#include <base/game.h>
#include <map>
#include <sdk/client_mode.h>
#include <sdk/color.h>
#include <sdk/game_event_manager.h>
#include <util/misc.h>
#include <utility>

namespace sdk
{
class game_event;
}

class event_log
{
protected:
	struct event_element
	{
		int color = 0x01;
		std::pair<std::string, char> enc_text;
		std::string dec_text{};

		event_element(int col, std::pair<std::string, char> txt) : enc_text(std::move(txt))
		{
			color = col;
			dec_text.clear();
		}

		event_element(int col, std::string txt) : dec_text(std::move(txt)) { color = col; }
	};

public:
	std::map<int, sdk::color> console_colors = {
		{0x01, sdk::color(255, 255, 255)}, {0x02, sdk::color(255, 0, 0)},	  {0x03, sdk::color(185, 130, 240)},
		{0x04, sdk::color(0, 255, 0)},	   {0x05, sdk::color(190, 255, 140)}, {0x06, sdk::color(165, 255, 80)},
		{0x07, sdk::color(255, 60, 60)},   {0x08, sdk::color(200, 200, 200)}, {0x09, sdk::color(225, 210, 125)},
		{0x0A, sdk::color(175, 195, 215)}, {0x0B, sdk::color(95, 150, 216)},  {0x0C, sdk::color(75, 105, 255)},
		{0x0D, sdk::color(175, 195, 215)}, {0x0E, sdk::color(210, 40, 230)},  {0x0F, sdk::color(235, 75, 75)},
		{0x10, sdk::color(225, 175, 50)}};

	event_log()
	{
		prefix.clear();

		prefix.emplace_back(event_element(0x01, XOR_STR_STORE("[")));
		prefix.emplace_back(event_element(0x0C, XOR_STR_STORE("ev0")));
		prefix.emplace_back(event_element(0x01, XOR_STR_STORE("lve] ")));

		log.clear();
	}

	void add(int color, std::pair<std::string, char> text) { log.emplace_back(event_element(color, std::move(text))); }

	void add(int color, std::string text) { log.emplace_back(event_element(color, std::move(text))); }

	void output();
	void force_output();

	std::vector<std::tuple<std::wstring, float, float>> &get_log_text_only() { return log_text_only; }

	void on_player_hurt(sdk::game_event *event);
	void on_purchase(sdk::game_event *event);

	void reset()
	{
		log.clear();
		log_text_only.clear();
	}

private:
	template <typename... A> void output_console(int clr, const char *format, A... args);
	template <typename... A> void output_console(const char *format, A... args);

	template <typename... A> void output_chat(int clr, const char *format, A... args);
	template <typename... A> void output_chat(const char *format, A... args);

	std::vector<event_element> prefix;
	std::vector<event_element> log;
	std::vector<std::tuple<std::wstring, float, float>> log_text_only;
	bool is_forced_output{};
};

template <typename... A> inline void event_log::output_console(int clr, const char *format, A... args)
{
	char out[1024];
	sprintf_s(out, 1024, format, args...);
	auto cl = console_colors[clr];
	game->cvar->print(cl, out);
}

template <typename... A> inline void event_log::output_console(const char *format, A... args)
{
	char out[1024];
	sprintf_s(out, 1024, format, args...);
	auto cl = sdk::color::white();
	game->cvar->print(cl, out);
}

template <typename... A> inline void event_log::output_chat(int clr, const char *format, A... args)
{
	char out[1024];
	sprintf_s(out, 1024, format, args...);
	std::string cl = {static_cast<char>(clr), 0};
	cl.append(out);
	game->client_mode->chat_element->output_chat(0, 0, cl.c_str());
}

template <typename... A> inline void event_log::output_chat(const char *format, A... args)
{
	char out[1024];
	sprintf_s(out, 1024, format, args...);
	game->client_mode->chat_element->output_chat(0, 0, out);
}

extern event_log eventlog;

#endif // BASE_EVENT_LOG_H
