#include <gui/controls/text_input.h>
#include <gui/gui.h>

GUI_NAMESPACE;

void text_input::render()
{
	control::render();
	if (!is_visible)
		return;

	caret_anim->animate();

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);
	d->add_rect_filled(r, colors.bg_bottom);
	d->add_rect(r, colors.outline);

	const auto base_offset = vec2{5.f - scroll_offset, size.y * 0.5f - 8.f};
	const auto old_clip = d->g.clip_rect;

	d->override_clip_rect(r.shrink(5.f));
	d->font = cur_font;
	if (value.empty() && !placeholder.empty())
		d->add_text(r.tl() + base_offset, placeholder, colors.text_dark);

	d->add_text(r.tl() + base_offset, value, colors.text);
	d->g.clip_rect = old_clip;

	if (is_locked_by_me())
	{
		const auto caret_offset = r.tl() + vec2{base_offset.x, 4.f} + vec2{cursor_offset + 2.f, 0.f};
		const auto caret_size = vec2{0.f, r.height() - 8.f};

		d->add_line(caret_offset, caret_offset + caret_size, colors.accent.mod_a(caret_anim->value));
	}
}

void text_input::create()
{
	caret_anim = std::make_shared<anim_float>(0.f, 0.25f);
	caret_anim->on_finished = [this](const std::shared_ptr<anim<float>> &a)
	{
		if (is_locked_by_me())
		{
			a->direct(1.f - a->end);
		}
	};
}

void text_input::on_mouse_down(char key)
{
	if (key == m_left)
	{
		if (is_mouse_on_me)
		{
			if (!is_locked_by_me())
			{
				lock_input();
				caret_anim->direct(1.f);
			}
			else
				position_cursor_by_mouse();
		}
		else
		{
			unlock_input();
			caret_anim->direct(0.f);
		}
	}
}

void text_input::on_text_input(const std::vector<uint32_t> &text)
{
	// we don't want to do to anything if input was backspace or not locked by us
	if (!is_locked_by_me() || text[0] == '\b' || input.is_key_down(VK_CONTROL) || input.is_key_down(VK_MENU))
		return;

	// account for max length
	if ((raw_value.size() + 1) > max_length.value_or(INT_MAX))
		return;

	const auto old_value = raw_value;

	std::string new_data{};
	for (const auto &v : text)
	{
		if (v < 0x20)
			continue;

		raw_value.emplace_back(v);
		new_data += utf8_encode(v);
	}

	// account for regex
	if (!test_regex(new_data))
	{
		raw_value = old_value;
		return;
	}

	auto new_data_width = calc_text_width(new_data);
	if (!old_value.empty() && cursor_pos != 0)
		new_data_width += calc_char_advance(raw_value[cursor_pos], raw_value[cursor_pos - 1], true);

	// adjust cursor offset
	cursor_offset += new_data_width;
	cursor_pos += (int)text.size();

	update();

	// perform scrolling if text is out of bounds
	if (does_text_exceed_width())
		scroll_offset += new_data_width;
}

void text_input::on_key_down(uint16_t key)
{
	if (!is_locked_by_me())
		return;

	// handle copy/paste
	if (input.is_key_down(VK_CONTROL) && (key == 'C' || key == 'V'))
	{
		if (key == 'C')
			clipboard::set(value);
		else
		{
			const auto v = clipboard::get();
			if (v.empty() || !test_regex(v))
				return;

			std::vector<uint32_t> new_data;
			const auto old_empty = raw_value.empty();
			for (int i{}; i < v.size();)
			{
				uint32_t cp;
				i += utf8_decode(v.data() + i, cp);
				new_data.emplace_back(cp);
				raw_value.emplace_back(cp);
			}

			auto new_data_width = calc_text_width(v);
			if (!old_empty && cursor_pos != 0)
				new_data_width += calc_char_advance(raw_value[cursor_pos], raw_value[cursor_pos - 1], true);

			cursor_offset += new_data_width;
			cursor_pos += (int)new_data.size();

			update();

			if (does_text_exceed_width())
				scroll_offset += new_data_width;
		}

		return;
	}

	// escape / return
	if (key == VK_ESCAPE || key == VK_RETURN)
	{
		unlock_input();
		caret_anim->direct(0.f);
		return;
	}

	// backspace
	if (key == VK_BACK && !raw_value.empty() && cursor_pos)
	{
		// adjust cursor offset by width of last char and decrement cursor position
		const auto last_char_width = calc_char_width_at_cursor(1, 2);
		raw_value.erase(raw_value.begin() + cursor_pos - 1);

		cursor_offset -= last_char_width;
		cursor_pos--;

		update();

		// also handle scroll
		if (scroll_offset > 0.f)
			scroll_offset -= last_char_width;

		return;
	}

	// delete
	if (key == VK_DELETE && cursor_pos != raw_value.size() && !raw_value.empty())
	{
		const auto last_char_width = calc_char_width_at_cursor(0, -1);
		raw_value.erase(raw_value.begin() + cursor_pos);

		update();

		if (scroll_offset > 0.f)
			scroll_offset -= last_char_width;

		return;
	}

	// move
	if (key == VK_LEFT || key == VK_RIGHT)
	{
		const auto adjust = key == VK_LEFT ? -1 : 1;
		const auto new_pos = cursor_pos + adjust;

		// check if we can move
		if (new_pos < 0 || new_pos > raw_value.size())
			return;

		// make temp text that is limited to new cursor pos
		std::string temp_value{};
		for (auto i = 0; i < new_pos; i++)
			temp_value += utf8_encode(raw_value[i]);

		const auto old_pos = cursor_pos;

		// update offset and position
		cursor_pos = new_pos;

		if (cursor_pos == 0)
		{
			cursor_offset = 0.f;
			scroll_offset = 0.f;

			return;
		}

		cursor_offset = calc_text_width(temp_value);

		if (cursor_pos == raw_value.size() && does_text_exceed_width())
		{
			scroll_offset = total_width - (size.x - 10.f) + 4.f;
			return;
		}

		// update scrolling, if necessary
		if (does_text_exceed_width() && is_cursor_out_of_bounds())
		{
			const auto new_char = raw_value[new_pos];
			const auto old_char = raw_value[old_pos];

			scroll_offset += calc_char_width(new_char, old_char, adjust == -1) * (float)adjust;
		}

		return;
	}
}

void text_input::calc_total_width() { total_width = calc_text_width(value); }

void text_input::make_value()
{
	value.clear();
	for (const auto &v : raw_value)
		value += utf8_encode(v);

	if (callback)
		callback(value);
}

void text_input::update()
{
	make_value();
	calc_total_width();
}

float text_input::calc_text_width(const std::string &v) { return cur_font->get_text_size(v).x; }

bool text_input::does_text_exceed_width() { return total_width > size.x - 10.f; }

bool text_input::is_cursor_out_of_bounds()
{
	const auto max_width = size.x - 10.f;
	const auto virtual_pos = cursor_offset - scroll_offset;

	if (virtual_pos < 0.f || virtual_pos > max_width)
		return true;

	return false;
}

void text_input::position_cursor_by_mouse()
{
	if (is_mouse_in_bounds(0.f))
	{
		cursor_offset = 0.f;
		cursor_pos = 0;

		return;
	}

	std::string temp_value{};
	int i = 0;
	for (const auto &v : raw_value)
	{
		temp_value += utf8_encode(v);

		const auto text_w = calc_text_width(temp_value);
		if (is_mouse_in_bounds(text_w))
		{
			const auto old_offset = cursor_offset;
			const auto old_pos = cursor_pos;

			cursor_offset = text_w;
			cursor_pos = i + 1;

			if (is_cursor_out_of_bounds())
			{
				cursor_offset = old_offset;
				cursor_pos = old_pos;
			}

			return;
		}

		i++;
	}

	cursor_offset = total_width;
	cursor_pos = (int)raw_value.size();
}

bool text_input::is_mouse_in_bounds(float w)
{
	const auto mouse_pos = ((input.cursor() - pos_abs()).x + scroll_offset) - w;
	return mouse_pos <= 8.f && mouse_pos >= -8.f;
}

float text_input::calc_char_width(uint32_t a, uint32_t b, bool invert)
{
	return calc_char_width(a) + calc_char_advance(a, b, invert);
}

float text_input::calc_char_advance(uint32_t a, uint32_t b, bool invert)
{
	if (a == ' ' || b == ' ' || a == '\t' || b == '\t')
		return 0.f;

	return cur_font->get_kerning(invert ? b : a, invert ? a : b);
}

float text_input::calc_char_width(uint32_t c)
{
	if (c == ' ' || c == '\t')
		return cur_font->height * 0.25f * (c == ' ' ? 1.f : 4.f);

	const auto &g = cur_font->get_glyph(c);
	return g.offset.x + g.size.x;
}

float text_input::calc_char_width_at_cursor(int offset, int next)
{
	auto last_char_width = calc_char_width(raw_value[cursor_pos - offset]);
	if (raw_value.size() > 1 && cursor_pos > raw_value.size() - next)
		last_char_width += calc_char_advance(raw_value[cursor_pos - offset], raw_value[cursor_pos - next], false);

	return last_char_width;
}

bool text_input::test_regex(const std::string &new_data)
{
	// test regex if we have one
	return !regex.has_value() || std::regex_search(new_data, regex.value());
}

void text_input::set_value(const std::string &val)
{
	cursor_offset = scroll_offset = 0.f;
	cursor_pos = 0;

	raw_value.clear();
	for (auto c : val)
		raw_value.emplace_back(c);

	update();
}
