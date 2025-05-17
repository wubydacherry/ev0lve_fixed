
#ifndef UTIL_ANTI_DEBUG_H
#define UTIL_ANTI_DEBUG_H

#include <Psapi.h>
#include <Shlobj.h>
#include <TlHelp32.h>
#include <filesystem>
#include <util/fnv1a.h>

#define CRASH() __asm add esp, 0xFFFF0000
#define PROCESSOR_FEATURE_MAX 64
#define USER_SHARED_DATA PKUSER_SHARED_DATA(0x7FFE0000)

extern "C" NTSYSAPI NTSTATUS NTAPI NtSetDebugFilterState(ULONG ComponentId, ULONG Level, BOOLEAN State);
extern "C" NTSTATUS NTAPI LdrDisableThreadCalloutsForDll(PVOID BaseAddress);

typedef void(__cdecl *_PVFV)(void);
typedef int(__cdecl *_PIFV)(void);
typedef void(__cdecl *_PVFI)(int);

extern "C" _PIFV *__xi_a; // C initializers
extern "C" _PIFV *__xi_z;
extern "C" _PVFV *__xc_a; // C++ initializers
extern "C" _PVFV *__xc_z;

namespace util
{
typedef enum _NT_PRODUCT_TYPE
{
	NtProductWinNt = 1,
	NtProductLanManNt = 2,
	NtProductServer = 3
} NT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
	StandardDesign = 0,
	NEC98x86 = 1,
	EndAlternatives = 2
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _KSYSTEM_TIME
{
	ULONG LowPart;
	LONG High1Time;
	LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _KUSER_SHARED_DATA
{
	ULONG TickCountLowDeprecated;
	ULONG TickCountMultiplier;
	KSYSTEM_TIME InterruptTime;
	KSYSTEM_TIME SystemTime;
	KSYSTEM_TIME TimeZoneBias;
	USHORT ImageNumberLow;
	USHORT ImageNumberHigh;
	WCHAR NtSystemRoot[260];
	ULONG MaxStackTraceDepth;
	ULONG CryptoExponent;
	ULONG TimeZoneId;
	ULONG LargePageMinimum;
	ULONG AitSamplingValue;
	ULONG AppCompatFlag;
	ULONGLONG RNGSeedVersion;
	ULONG GlobalValidationRunlevel;
	LONG TimeZoneBiasStamp;
	ULONG NtBuildNumber;
	NT_PRODUCT_TYPE NtProductType;
	BOOLEAN ProductTypeIsValid;
	BOOLEAN Reserved0[1];
	USHORT NativeProcessorArchitecture;
	ULONG NtMajorVersion;
	ULONG NtMinorVersion;
	BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
	ULONG Reserved1;
	ULONG Reserved3;
	ULONG TimeSlip;
	ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
	ULONG BootId;
	LARGE_INTEGER SystemExpirationDate;
	ULONG SuiteMask;
	BOOLEAN KdDebuggerEnabled;
	union
	{
		UCHAR MitigationPolicies;
		struct
		{
			UCHAR NXSupportPolicy : 2;
			UCHAR SEHValidationPolicy : 2;
			UCHAR CurDirDevicesSkippedForDlls : 2;
			UCHAR Reserved : 2;
		};
	};
	USHORT CyclesPerYield;
	ULONG ActiveConsoleId;
	ULONG DismountCount;
	ULONG ComPlusPackage;
	ULONG LastSystemRITEventTickCount;
	ULONG NumberOfPhysicalPages;
	BOOLEAN SafeBootMode;
	UCHAR VirtualizationFlags;
	UCHAR Reserved12[2];
	union
	{
		ULONG SharedDataFlags;
		struct
		{
			ULONG DbgErrorPortPresent : 1;
			ULONG DbgElevationEnabled : 1;
			ULONG DbgVirtEnabled : 1;
			ULONG DbgInstallerDetectEnabled : 1;
			ULONG DbgLkgEnabled : 1;
			ULONG DbgDynProcessorEnabled : 1;
			ULONG DbgConsoleBrokerEnabled : 1;
			ULONG DbgSecureBootEnabled : 1;
			ULONG DbgMultiSessionSku : 1;
			ULONG DbgMultiUsersInSessionSku : 1;
			ULONG DbgStateSeparationEnabled : 1;
			ULONG SpareBits : 21;
		} DUMMYSTRUCTNAME2;
	} DUMMYUNIONNAME2;
	ULONG DataFlagsPad[1];
	ULONGLONG TestRetInstruction;
	LONGLONG QpcFrequency;
	ULONG SystemCall;
	ULONG SystemCallPad0;
	ULONGLONG SystemCallPad[2];
	union
	{
		KSYSTEM_TIME TickCount;
		ULONG64 TickCountQuad;
		struct
		{
			ULONG ReservedTickCountOverlay[3];
			ULONG TickCountPad[1];
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME3;
	ULONG Cookie;
	ULONG CookiePad[1];
	LONGLONG ConsoleSessionForegroundProcessId;
	ULONGLONG TimeUpdateLock;
	ULONGLONG BaselineSystemTimeQpc;
	ULONGLONG BaselineInterruptTimeQpc;
	ULONGLONG QpcSystemTimeIncrement;
	ULONGLONG QpcInterruptTimeIncrement;
	UCHAR QpcSystemTimeIncrementShift;
	UCHAR QpcInterruptTimeIncrementShift;
	USHORT UnparkedProcessorCount;
	ULONG EnclaveFeatureMask[4];
	ULONG TelemetryCoverageRound;
	USHORT UserModeGlobalLogger[16];
	ULONG ImageFileExecutionOptions;
	ULONG LangGenerationCount;
	ULONGLONG Reserved4;
	ULONGLONG InterruptTimeBias;
	ULONGLONG QpcBias;
	ULONG ActiveProcessorCount;
	UCHAR ActiveGroupCount;
	UCHAR Reserved9;
	union
	{
		USHORT QpcData;
		struct
		{
			UCHAR QpcBypassEnabled;
			UCHAR QpcShift;
		};
	};
	LARGE_INTEGER TimeZoneBiasEffectiveStart;
	LARGE_INTEGER TimeZoneBiasEffectiveEnd;
	XSTATE_CONFIGURATION XState;
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

const auto blacklisted_processes =
	std::array{FNV1A("windbg.exe"),		FNV1A("x64dbg.exe"),	FNV1A("x32dbg.exe"),	FNV1A("idaq.exe"),
			   FNV1A("idaq64.exe"),		FNV1A("procmon.exe"),	FNV1A("procmon64.exe"), FNV1A("ollydbg.exe"),
			   FNV1A("scylla_x64.exe"), FNV1A("scylla_x86.exe")};

inline void initialize_mutex()
{
	static auto initialized = false;

	if (!initialized)
	{
		CreateMutexExA(nullptr, XOR_STR_OT("$ IDA trusted_idbs"), 1, XOR_32(MUTEX_ALL_ACCESS));
		CreateMutexExA(nullptr, XOR_STR_OT("$ IDA registry mutex $"), 1, XOR_32(MUTEX_ALL_ACCESS));
		initialized = true;
	}
}

inline void wipe_crt_initalize_table()
{
#if defined(CSGO_SECURE) && !defined(_DEBUG)
	memset((void *)&__xi_a, 0, (uintptr_t)&__xi_z - (uintptr_t)&__xi_a);
	memset((void *)&__xc_a, 0, (uintptr_t)&__xc_z - (uintptr_t)&__xc_a);
#endif
}

inline void check_game_stream()
{
#if defined(CSGO_SECURE) && !defined(_DEBUG)
	static auto last_call = 0u;
	const auto tick = (uint32_t)(USER_SHARED_DATA->TickCountQuad * USER_SHARED_DATA->TickCountMultiplier >> 24);

	if (tick < last_call + 5000)
		return;

	last_call = tick;

	const auto quit_with_error = []()
	{
		MessageBoxA(nullptr, XOR_STR("Game streaming is not allowed"), XOR_STR("Critical error"),
					XOR_32(MB_APPLMODAL | MB_OK | MB_ICONERROR));
		quick_exit(0);
	};

	const auto check_service = [quit_with_error](SC_HANDLE scm, const char *name)
	{
		const auto service = OpenService(scm, name, GENERIC_READ);
		if (!service)
		{
			CloseServiceHandle(scm);
			return;
		}

		SERVICE_STATUS status{};
		if (!QueryServiceStatus(service, &status))
		{
			CloseServiceHandle(service);
			CloseServiceHandle(scm);
			return;
		}

		if (status.dwCurrentState == SERVICE_RUNNING)
			quit_with_error();

		CloseServiceHandle(service);
	};

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		CRASH();

	PROCESSENTRY32 proc{};
	if (!Process32First(snapshot, &proc))
		CRASH();

	do
	{
		const auto hash = util::fnv1a(proc.szExeFile);
		if (hash == FNV1A("nvstreamer.exe") || hash == FNV1A("steam_monitor.exe"))
			quit_with_error();
	} while (Process32Next(snapshot, &proc));

	CloseHandle(snapshot);

	const auto scm = OpenSCManager(NULL, XOR_STR(SERVICES_ACTIVE_DATABASE), XOR_32(SC_MANAGER_CONNECT));
	if (!scm)
		quick_exit(0);

	check_service(scm, XOR_STR("Parsec")); // main parsec service

	CloseServiceHandle(scm);

	// previous checks may fail, also check on parsec's lock file
	char appdata_path[256]{};
	SHGetSpecialFolderPathA(nullptr, appdata_path, CSIDL_APPDATA, false);

	std::string lock_path{appdata_path};
	lock_path += XOR_STR("\\Parsec\\lock");

	if (std::filesystem::exists(lock_path))
	{
		const auto lock = CreateFileA(lock_path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (!lock || lock == INVALID_HANDLE_VALUE)
			quit_with_error();

		CloseHandle(lock);
	}
#endif
}

inline void check_integrity()
{
#if defined(CSGO_SECURE) && !defined(_DEBUG)
	static auto last_call = 0u;
	const auto tick = (uint32_t)(USER_SHARED_DATA->TickCountQuad * USER_SHARED_DATA->TickCountMultiplier >> 24);

	if (tick < last_call + 300)
		return;

	last_call = tick;

	const auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;

	if (!peb || peb->BeingDebugged || (USER_SHARED_DATA->KdDebuggerEnabled & 1) ||
		(USER_SHARED_DATA->KdDebuggerEnabled & 2)		 // debugger or kernel debugger?
		|| (*(uint32_t *)(uintptr_t(peb) + 0x68)) & 0x70 // NtGlobalFlags
		|| NtSetDebugFilterState(0, 0, true) == STATUS_SUCCESS)
		CRASH();

	char process_name[MAX_PATH];
	DWORD process_id[1024], total, count;
	HMODULE mod;

	if (!EnumProcesses(process_id, sizeof(process_id), &total))
		CRASH();

	for (auto i = 0u; i < total / sizeof(DWORD); i++)
	{
		const auto p = OpenProcess(XOR_32(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ), false, process_id[i]);

		if (p != nullptr && EnumProcessModules(p, &mod, sizeof(mod), &count) &&
			GetModuleBaseName(p, mod, process_name, sizeof(process_name) / sizeof(char)))
			for (auto &proc : blacklisted_processes)
				if (FNV1A_CMP_IM(process_name, proc))
					CRASH();
	}

	initialize_mutex();
	check_game_stream();
#endif
}
} // namespace util

#endif // UTIL_ANTI_DEBUG_H
