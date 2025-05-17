
#ifndef UTIL_DEFINITIONS_H
#define UTIL_DEFINITIONS_H

#pragma comment(lib, "ntdll")

extern "C"
{
	NTSYSAPI NTSTATUS NTAPI NtProtectVirtualMemory(HANDLE process_handle, PVOID *base_address,
												   PULONG number_of_bytes_to_protect, ULONG new_access_protection,
												   PULONG old_access_protection);
}

typedef struct _LDR_MODULE
{
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _PEB_LOADER_DATA
{
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LOADER_DATA, *PPEB_LOADER_DATA;

#define CURRENT_PROCESS HANDLE(-1)
constexpr auto page_size = 4096;
constexpr auto previous_state_mask = 0x40000000;

#endif // UTIL_DEFINITIONS_H
