
#include <util/memory.h>

namespace util
{
__forceinline static wchar_t *file_name_w(wchar_t *path)
{
	wchar_t *slash = path;

	while (path && *path)
	{
		if ((*path == '\\' || *path == '/' || *path == ':') && path[1] && path[1] != '\\' && path[1] != '/')
			slash = path + 1;
		path++;
	}

	return slash;
}

uintptr_t find_module(const uint32_t hash)
{
	const auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;

	if (!peb)
		return 0;

	const auto ldr = reinterpret_cast<PPEB_LOADER_DATA>(peb->Ldr);

	if (!ldr)
		return 0;

	const auto head = &ldr->InLoadOrderModuleList;
	auto current = head->Flink;

	while (current != head)
	{
		const auto module = CONTAINING_RECORD(current, LDR_MODULE, InLoadOrderModuleList);
		std::wstring wide(file_name_w(module->FullDllName.Buffer));
		std::string name(wide.begin(), wide.end());
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		if (FNV1A_CMP_IM(name.c_str(), hash))
			return reinterpret_cast<uintptr_t>(module->BaseAddress);

		current = current->Flink;
	}

	return 0;
}

uintptr_t find_export(const uint32_t hash, const uint32_t exp)
{
	const auto module = (uintptr_t)find_module(hash);

	if (!module)
		return 0;

	const auto dosHeader = PIMAGE_DOS_HEADER(module);
	const auto ntHeader = PIMAGE_NT_HEADERS(module + (uintptr_t)dosHeader->e_lfanew);

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || ntHeader->Signature != IMAGE_NT_SIGNATURE)
		return 0;

	const auto exports = PIMAGE_EXPORT_DIRECTORY(
		module + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	const auto names = (uintptr_t *)(module + exports->AddressOfNames);

	auto ordinal_index = (size_t)-1;
	for (uint32_t i = 0; i < exports->NumberOfFunctions; i++)
		if (FNV1A_CMP_IM((const char *)(module + names[i]), exp))
		{
			ordinal_index = i;
			break;
		}

	if (ordinal_index > exports->NumberOfFunctions)
		return 0;

	const auto ordinals = (uint16_t *)(module + exports->AddressOfNameOrdinals);
	const auto addresses = (uint32_t *)(module + exports->AddressOfFunctions);
	return uintptr_t(module + addresses[ordinals[ordinal_index]]);
}
} // namespace util
