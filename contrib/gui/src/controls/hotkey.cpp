#include <gui/controls/hotkey.h>
#include <gui/gui.h>

GUI_NAMESPACE;

void hotkey::render()
{
	control::render();
	if (!is_visible)
		return;

	const auto r = area_abs();

	auto &d = draw.layers[ctx->content_layer];
	d->add_shadow_rect(r, 4.f);
	d->add_rect_filled(r, colors.bg_bottom);
	d->add_rect(r, colors.outline);

	std::string text = XOR("None");
	if (const auto k = key_table.find(*value); k != key_table.end())
		text = k->second;

	d->font = draw.fonts[GUI_HASH("gui_main")];
	d->add_text(r.tl() + vec2{5.f, 3.f}, text, is_locked_by_me() ? colors.accent : colors.text);
}

void hotkey::on_mouse_down(char key)
{
	if (key == m_left && !is_input_locked())
		lock_input();
	else if (is_locked_by_me())
	{
		switch (key)
		{
		default:
		case m_left:
			*value = VK_LBUTTON;
			break;
		case m_right:
			*value = VK_RBUTTON;
			break;
		case m_middle:
			*value = VK_MBUTTON;
			break;
		case m_back:
			*value = VK_XBUTTON1;
			break;
		case m_forward:
			*value = VK_XBUTTON2;
			break;
		}

		if (callback)
			callback(*value);

		unlock_input();
	}
}

void hotkey::on_key_down(uint16_t key)
{
	if (!is_locked_by_me())
		return;

	*value = key == VK_ESCAPE ? 0 : key;
	if (callback)
		callback(*value);

	unlock_input();
}
