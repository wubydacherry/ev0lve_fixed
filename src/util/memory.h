
#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include <TlHelp32.h>
#include <util/definitions.h>
#include <util/fnv1a.h>
#include <util/value_obfuscation.h>

namespace util
{
uintptr_t find_module(const uint32_t hash);
uintptr_t find_export(const uint32_t hash, const uint32_t exp);

__forceinline uintptr_t traverse_stack(uintptr_t *ebp, uintptr_t addr)
{
	if (ebp == nullptr)
		return 0;

	const auto next = *(uintptr_t **)(ebp);
	if (ebp[1] == addr)
		return uintptr_t(next);

	return traverse_stack(next, addr);
}

__forceinline __declspec(naked) uintptr_t *get_ebp()
{
	__asm
	{
			mov eax, ebp
			retn
	}
}
} // namespace util

#define LOOKUP_MODULE(name) util::find_module(FNV1A(name))
#define FIND_EXPORT(name, exp) util::find_export(FNV1A(name), FNV1A(exp))

#endif // UTIL_MEMORY_H
