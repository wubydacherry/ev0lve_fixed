
#ifndef UTIL_MEM_GUARD_H
#define UTIL_MEM_GUARD_H

#include <util/memory.h>

namespace util
{
class mem_guard
{
public:
	template <typename F>
	explicit mem_guard(const DWORD protection, const F address, const size_t wanted_size = page_size)
		: address(reinterpret_cast<PVOID>(address)), protection(PAGE_EXECUTE_READ), wanted_size(wanted_size),
		  is_attached(true)
	{
		ULONG size = wanted_size;
		NtProtectVirtualMemory(CURRENT_PROCESS, &this->address, &size, protection, &this->protection);
	}

	// Warning! This detaches the guard from the protected memory region.
	PVOID detach()
	{
		if (!is_attached)
			RUNTIME_ERROR("Mem guard is already detached.");

		is_attached = false;
		return address;
	}

	mem_guard(mem_guard const &other) = delete;
	mem_guard &operator=(mem_guard const &other) = delete;
	mem_guard(mem_guard &&other) = delete;
	mem_guard &operator=(mem_guard &&other) = delete;

	~mem_guard()
	{
		if (!is_attached)
			return;

		ULONG size = wanted_size;
		NtProtectVirtualMemory(CURRENT_PROCESS, &address, &size, protection, &protection);
	}

private:
	PVOID address;
	DWORD protection;
	size_t wanted_size;
	bool is_attached;
};
} // namespace util

#endif // UTIL_MEM_GUARD_H
