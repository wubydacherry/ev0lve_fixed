#ifndef TEXT_INPUT_B30FC753F02149DE93E2058A754C4783_H
#define TEXT_INPUT_B30FC753F02149DE93E2058A754C4783_H

#include <gui/control.h>
#include <gui/misc.h>
#include <regex>
#include <ren/renderer.h>
#include <utility>

namespace evo::gui
{
class text_input : public control
{
public:
	text_input(control_id id, const ren::vec2 &pos = {}, const ren::vec2 &sz = {140.f, 24.f},
			   std::optional<std::regex> regex = std::nullopt, std::optional<int> max_length = std::nullopt)
		: control(std::move(id), pos, sz), regex(std::move(regex)), max_length(max_length)
	{
		using namespace ren;

		is_taking_keys = true;
		is_taking_text = true;
		margin = {2.f, 2.f, 2.f, 2.f};

		cur_font = draw.fonts[GUI_HASH("gui_main")];
		create();

		type = ctrl_text_input;
	}

	~text_input() { using namespace ren; }

	std::shared_ptr<control> make_placeholder(const std::string &plh)
	{
		placeholder = plh;
		return shared_from_this();
	}

	void on_mouse_down(char key) override;
	void on_key_down(uint16_t key) override;
	void on_text_input(const std::vector<uint32_t> &text) override;
	void render() override;

	std::string placeholder{};
	std::string value{};
	std::function<void(std::string &)> callback{};

	void set_value(const std::string &val);

private:
	std::shared_ptr<ren::font_base> cur_font{};
	std::vector<uint32_t> raw_value{};
	std::optional<std::regex> regex{};
	std::optional<int> max_length{};
	std::shared_ptr<ren::anim_float> caret_anim;

	bool test_regex(const std::string &new_data);

	void create();
	void update();

	float calc_text_width(const std::string &v);
	float calc_char_advance(uint32_t a, uint32_t b, bool invert);
	float calc_char_width(uint32_t c);
	float calc_char_width(uint32_t a, uint32_t b, bool invert);
	float calc_char_width_at_cursor(int offset, int next);

	void calc_total_width();
	void make_value();

	void position_cursor_by_mouse();

	bool does_text_exceed_width();
	bool is_cursor_out_of_bounds();
	bool is_mouse_in_bounds(float w);

	int cursor_pos{};

	float total_width{};
	float scroll_offset{};
	float cursor_offset{};
};
} // namespace evo::gui

#endif // TEXT_INPUT_B30FC753F02149DE93E2058A754C4783_H
