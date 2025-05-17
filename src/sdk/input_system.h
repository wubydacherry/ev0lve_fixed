
#ifndef SDK_INPUT_SYSTEM_H
#define SDK_INPUT_SYSTEM_H

#include <sdk/macros.h>

namespace sdk
{
class input_system_t
{
public:
	VIRTUAL(11, enable_input(const bool active), void(__thiscall *)(void *, bool))(active);
	VIRTUAL(39, reset_input_state(), void(__thiscall *)(void *))();
	VIRTUAL(52, get_attached_window(), HWND(__thiscall *)(void *))();
};
} // namespace sdk

#endif // SDK_INPUT_SYSTEM_H
