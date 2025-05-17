
#ifndef SDK_LOCALIZE_H
#define SDK_LOCALIZE_H

#include <sdk/macros.h>

namespace sdk
{
class localize_t
{
public:
	VIRTUAL(12, find_safe(const char *str), const wchar_t *(__thiscall *)(void *, const char *))(str);
};
} // namespace sdk

#endif // SDK_LOCALIZE_H
