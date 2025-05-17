
#ifndef SDK_MDL_CACHE_H
#define SDK_MDL_CACHE_H

#include <sdk/macros.h>

namespace sdk
{
class mdl_cache_t
{
public:
	VIRTUAL(33, begin_lock(), void(__thiscall *)(void *))();
	VIRTUAL(34, end_lock(), void(__thiscall *)(void *))();
	VIRTUAL(48, lock_studio_hdr(uint16_t handle), void *(__thiscall *)(void *, uint16_t))(handle);
	VIRTUAL(49, unlock_studio_hdr(uint16_t handle), void(__thiscall *)(void *, uint16_t))(handle);
};
} // namespace sdk

#endif // SDK_MDL_CACHE_H
