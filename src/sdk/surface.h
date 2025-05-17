
#ifndef SDK_SURFACE_H
#define SDK_SURFACE_H

#include <sdk/macros.h>

namespace sdk
{
class mat_surface_t
{
public:
	VIRTUAL(66, unlock_cursor(), void(__thiscall *)(void *))();
	VIRTUAL(82, play_sound(const char *sample), void(__thiscall *)(void *, const char *))(sample);
};
} // namespace sdk

#define EMIT_TICK_SOUND() ::game->surface->play_sound(XOR_STR("buttons\\lightswitch2.wav"));
#define EMIT_SUCCESS_SOUND() ::game->surface->play_sound(XOR_STR("buttons\\blip2.wav"));
#define EMIT_ERROR_SOUND() ::game->surface->play_sound(XOR_STR("buttons\\blip1.wav"));

#endif // SDK_SURFACE_H
