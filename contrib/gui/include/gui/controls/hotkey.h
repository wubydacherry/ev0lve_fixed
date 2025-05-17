#ifndef HOTKEY_58EFDE7C243D435F95AC6CBEB58E3167_H
#define HOTKEY_58EFDE7C243D435F95AC6CBEB58E3167_H

#include <functional>
#include <gui/control.h>
#include <gui/gui.h>
#include <utility>

namespace evo::gui
{
inline static const std::unordered_map<uint16_t, std::string> key_table = {
	{VK_LBUTTON, XOR("Mouse LEFT")},
	{VK_RBUTTON, XOR("Mouse RIGHT")},
	{VK_MBUTTON, XOR("Mouse MIDDLE")},
	{VK_XBUTTON1, XOR("Mouse BACK")},
	{VK_XBUTTON2, XOR("Mouse FORWARD")},
	{VK_BACK, XOR("Backspace")},
	{VK_TAB, XOR("TAB")},
	{VK_RETURN, XOR("Enter")},
	{VK_SHIFT, XOR("Shift")},
	{VK_CONTROL, XOR("Control")},
	{VK_MENU, XOR("Alt")},
	{VK_CAPITAL, XOR("Caps Lock")},
	{VK_SPACE, XOR("Space")},
	{VK_PRIOR, XOR("Page UP")},
	{VK_NEXT, XOR("Page DOWN")},
	{VK_END, XOR("End")},
	{VK_HOME, XOR("Home")},
	{VK_LEFT, XOR("Left")},
	{VK_UP, XOR("Up")},
	{VK_RIGHT, XOR("Right")},
	{VK_DOWN, XOR("Down")},
	{VK_INSERT, XOR("Insert")},
	{VK_DELETE, XOR("Delete")},
	{VK_NUMPAD0, XOR("Numpad 0")},
	{VK_NUMPAD1, XOR("Numpad 1")},
	{VK_NUMPAD2, XOR("Numpad 2")},
	{VK_NUMPAD3, XOR("Numpad 3")},
	{VK_NUMPAD4, XOR("Numpad 4")},
	{VK_NUMPAD5, XOR("Numpad 5")},
	{VK_NUMPAD6, XOR("Numpad 6")},
	{VK_NUMPAD7, XOR("Numpad 7")},
	{VK_NUMPAD8, XOR("Numpad 8")},
	{VK_NUMPAD9, XOR("Numpad 9")},
	{VK_MULTIPLY, XOR("Numpad MULTIPLY")},
	{VK_ADD, XOR("Numpad ADD")},
	{VK_SEPARATOR, XOR("Numpad SEPARATOR")},
	{VK_SUBTRACT, XOR("Numpad SUBTRACT")},
	{VK_DECIMAL, XOR("Decimal Point")},
	{VK_DIVIDE, XOR("Numpad DIVIDE")},
	{VK_F1, XOR("F1")},
	{VK_F2, XOR("F2")},
	{VK_F3, XOR("F3")},
	{VK_F4, XOR("F4")},
	{VK_F5, XOR("F5")},
	{VK_F6, XOR("F6")},
	{VK_F7, XOR("F7")},
	{VK_F8, XOR("F8")},
	{VK_F9, XOR("F9")},
	{VK_F10, XOR("F10")},
	{VK_F11, XOR("F11")},
	{VK_F12, XOR("F12")},
	{VK_OEM_1, XOR("Semicolon (;)")},
	{VK_OEM_PLUS, XOR("Plus (+)")},
	{VK_OEM_COMMA, XOR("Comma (,)")},
	{VK_OEM_MINUS, XOR("Minus (-)")},
	{VK_OEM_PERIOD, XOR("Period (.)")},
	{VK_OEM_2, XOR("Slash (/)")},
	{VK_OEM_3, XOR("Tilde (~)")},
	{VK_OEM_4, XOR("Open Brace ([)")},
	{VK_OEM_5, XOR("Backslash (\\)")},
	{VK_OEM_6, XOR("Close Brace (])")},
	{VK_OEM_7, XOR("Quote (')")},
	{'0', XOR("0")},
	{'1', XOR("1")},
	{'2', XOR("2")},
	{'3', XOR("3")},
	{'4', XOR("4")},
	{'5', XOR("5")},
	{'6', XOR("6")},
	{'7', XOR("7")},
	{'8', XOR("8")},
	{'9', XOR("9")},
	{'A', XOR("A")},
	{'B', XOR("B")},
	{'C', XOR("C")},
	{'D', XOR("D")},
	{'E', XOR("E")},
	{'F', XOR("F")},
	{'G', XOR("G")},
	{'H', XOR("H")},
	{'I', XOR("I")},
	{'J', XOR("J")},
	{'K', XOR("K")},
	{'L', XOR("L")},
	{'M', XOR("M")},
	{'N', XOR("N")},
	{'O', XOR("O")},
	{'P', XOR("P")},
	{'Q', XOR("Q")},
	{'R', XOR("R")},
	{'S', XOR("S")},
	{'T', XOR("T")},
	{'U', XOR("U")},
	{'V', XOR("V")},
	{'W', XOR("W")},
	{'X', XOR("X")},
	{'Y', XOR("Y")},
	{'Z', XOR("Z")},
};

inline static std::string key_to_string(uint16_t k)
{
	return key_table.find(k) != key_table.end() ? key_table.at(k) : XOR("None");
}

class hotkey : public control
{
public:
	hotkey(control_id id, uint32_t *value, const ren::vec2 &pos = {}, const ren::vec2 &sz = {110.f, 24.f})
		: control(std::move(id), pos, sz), value(value)
	{
		margin = {2.f, 2.f, 2.f, 2.f};
		is_taking_keys = true;
		type = ctrl_hotkey;
	}

	void on_mouse_down(char key) override;
	void on_key_down(uint16_t key) override;
	void render() override;

	std::function<void(uint32_t)> callback = {};
	uint32_t *value{};
};
} // namespace evo::gui

#endif // HOTKEY_58EFDE7C243D435F95AC6CBEB58E3167_H
