
#ifndef SDK_MEM_ALLOC_H
#define SDK_MEM_ALLOC_H

#include <sdk/macros.h>

namespace sdk
{
class mem_alloc_t
{
public:
	VIRTUAL(1, alloc(size_t s), void *(__thiscall *)(void *, size_t))(s);
	VIRTUAL(3, realloc(void *m, size_t s), void *(__thiscall *)(void *, void *, size_t))(m, s);
	VIRTUAL(5, free(void *m), void(__thiscall *)(void *, void *))(m);
};
} // namespace sdk

#endif // SDK_MEM_ALLOC_H
