#include <base/crash_handler.h>

#include "game.h"
#include <DbgHelp.h>
#include <Psapi.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <util/definitions.h>

crash_handler_t::crash_handler_t()
{
	// fill in exception code descriptors.
	exception_codes[EXCEPTION_ACCESS_VIOLATION] = XOR_STR("Access violation");
	exception_codes[EXCEPTION_ARRAY_BOUNDS_EXCEEDED] = XOR_STR("Array bounds exceeded");
	exception_codes[EXCEPTION_BREAKPOINT] = XOR_STR("Breakpoint");
	exception_codes[EXCEPTION_DATATYPE_MISALIGNMENT] = XOR_STR("Datatype misalignment");
	exception_codes[EXCEPTION_FLT_DENORMAL_OPERAND] = XOR_STR("Float denormal operand");
	exception_codes[EXCEPTION_FLT_DIVIDE_BY_ZERO] = XOR_STR("Float divide by zero");
	exception_codes[EXCEPTION_FLT_INEXACT_RESULT] = XOR_STR("Float inexact result");
	exception_codes[EXCEPTION_FLT_INVALID_OPERATION] = XOR_STR("Float invalid operation");
	exception_codes[EXCEPTION_FLT_OVERFLOW] = XOR_STR("Float overflow");
	exception_codes[EXCEPTION_FLT_STACK_CHECK] = XOR_STR("Float stack check");
	exception_codes[EXCEPTION_FLT_UNDERFLOW] = XOR_STR("Float underflow");
	exception_codes[EXCEPTION_ILLEGAL_INSTRUCTION] = XOR_STR("Illegal instruction");
	exception_codes[EXCEPTION_IN_PAGE_ERROR] = XOR_STR("In page error");
	exception_codes[EXCEPTION_INT_DIVIDE_BY_ZERO] = XOR_STR("Integer divide by zero");
	exception_codes[EXCEPTION_INT_OVERFLOW] = XOR_STR("Integer overflow");
	exception_codes[EXCEPTION_INVALID_DISPOSITION] = XOR_STR("Invalid disposition");
	exception_codes[EXCEPTION_NONCONTINUABLE_EXCEPTION] = XOR_STR("Noncontinuable exception");
	exception_codes[EXCEPTION_PRIV_INSTRUCTION] = XOR_STR("Privileged instruction");
	exception_codes[EXCEPTION_SINGLE_STEP] = XOR_STR("Single step");
	exception_codes[EXCEPTION_STACK_OVERFLOW] = XOR_STR("Stack overflow");
}

void crash_handler_t::trace()
{
	// make sure ev0lve/crashes directory exists.
	std::filesystem::create_directories(XOR_STR("ev0lve/crashes"));

	// get current date and time via localtime.
	const auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	const auto time = std::localtime(&in_time_t);

	// create a new crash dump file with the current date and time.
	std::ofstream dump(tfm::format(XOR_STR("ev0lve/crashes/%02d-%02d-%02d_%02d-%02d-%02d.txt"), time->tm_mday,
								   time->tm_mon + 1, time->tm_year + 1900, time->tm_hour, time->tm_min, time->tm_sec));
	if (!dump.is_open())
	{
		MessageBoxA(nullptr, XOR_STR("Failed to create crash dump file."), XOR_STR("Critical error"),
					XOR_32(MB_ICONERROR | MB_OK | MB_TOPMOST));
		return;
	}

	// start writing.
	XOR_STR_STACK(ver, game->version);
	dump << XOR_STR("internal v") << ver << XOR_STR("\n");
	dump << XOR_STR("----------------------------\n\n");

	if (ex)
	{
		// write exception information.
		std::string exception_name{XOR_STR("Unknown exception")};
		if (exception_codes.find(ex->ExceptionRecord->ExceptionCode) != exception_codes.end())
			exception_name = exception_codes[ex->ExceptionRecord->ExceptionCode];
		dump << tfm::format(XOR_STR("%s (0x%08X) at 0x%08X\n\n"), exception_name, ex->ExceptionRecord->ExceptionCode,
							ex->ExceptionRecord->ExceptionAddress);

		// dump registers (fit 4 per line).
		dump << XOR_STR("Registers:\n");
		dump << XOR_STR("----------------------------\n");
		dump << tfm::format(XOR_STR("EAX: 0x%08X  EBX: 0x%08X  ECX: 0x%08X  EDX: 0x%08X\n"), ex->ContextRecord->Eax,
							ex->ContextRecord->Ebx, ex->ContextRecord->Ecx, ex->ContextRecord->Edx);
		dump << tfm::format(XOR_STR("ESI: 0x%08X  EDI: 0x%08X  EBP: 0x%08X  ESP: 0x%08X\n"), ex->ContextRecord->Esi,
							ex->ContextRecord->Edi, ex->ContextRecord->Ebp, ex->ContextRecord->Esp);
		dump << tfm::format(XOR_STR("EIP: 0x%08X  EFL: 0x%08X  CS:  0x%08X  SS:  0x%08X\n"), ex->ContextRecord->Eip,
							ex->ContextRecord->EFlags, ex->ContextRecord->SegCs, ex->ContextRecord->SegSs);
		dump << tfm::format(XOR_STR("DS:  0x%08X  ES:  0x%08X  FS:  0x%08X  GS:  0x%08X\n\n"), ex->ContextRecord->SegDs,
							ex->ContextRecord->SegEs, ex->ContextRecord->SegFs, ex->ContextRecord->SegGs);
	}
	else
		dump << std_ex.what() << XOR_STR("\n\n");

	// dump all modules.
	dump << XOR_STR("Modules:\n");
	dump << XOR_STR("----------------------------\n");
	dump << tfm::format(XOR_STR("%-30s %-20s %-20s\n"), XOR_STR("Name"), XOR_STR("Base"), XOR_STR("Size"));
	dump << XOR_STR("----------------------------\n");

	dump << tfm::format(XOR_STR("%-30s 0x%-18X 0x%-18X\n"), XOR_STR("internal"), game->base, 0);
	if (!for_each_module([&](const std::string &name, uintptr_t base, uintptr_t size)
						 { dump << tfm::format(XOR_STR("%-30s 0x%-18X 0x%-18X\n"), name, base, size); }))
		dump << XOR_STR("Failed to enumerate modules.\n");
	dump << XOR_STR("\n");

	if (ex)
	{
		// dump callstack.
		dump << XOR_STR("\nCallstack:\n");
		dump << XOR_STR("----------------------------\n");
		dump << tfm::format(XOR_STR("%-30s %-20s %-20s\n"), XOR_STR("Module"), XOR_STR("Address (.text+X)"),
							XOR_STR("Function"));
		dump << XOR_STR("----------------------------\n");

		if (!stackwalk(
				[&](const trace_func_t &trace)
				{ dump << tfm::format(XOR_STR("%-30s 0x%-18X %-20s\n"), trace.mod, trace.address, trace.name); }))
			dump << XOR_STR("Failed to retrieve callstack.\n");
	}
}

__forceinline bool
crash_handler_t::for_each_module(const std::function<void(const std::string &, uintptr_t, uintptr_t)> &func)
{
	const auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;
	if (!peb)
		return false;

	const auto ldr = reinterpret_cast<PPEB_LOADER_DATA>(peb->Ldr);
	if (!ldr)
		return false;

	const auto head = &ldr->InLoadOrderModuleList;
	auto current = head->Flink;

	while (current != head)
	{
		const auto module = CONTAINING_RECORD(current, LDR_MODULE, InLoadOrderModuleList);
		std::wstring wide(module->FullDllName.Buffer);
		std::string name(wide.begin(), wide.end());
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		func(std::filesystem::path(name).filename().string(), reinterpret_cast<uintptr_t>(module->BaseAddress),
			 module->SizeOfImage);
		current = current->Flink;
	}

	return true;
}

__forceinline bool crash_handler_t::stackwalk(const std::function<void(const trace_func_t &func)> &func)
{
	if (!SymInitialize(GetCurrentProcess(), nullptr, 1))
		return false;

	STACKFRAME sf;
	memset(&sf, 0, sizeof(sf));
	sf.AddrPC.Offset = ex->ContextRecord->Eip;
	sf.AddrStack.Offset = ex->ContextRecord->Esp;
	sf.AddrFrame.Offset = ex->ContextRecord->Ebp;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	int i{};
	while (StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, ex->ContextRecord, nullptr,
					 SymFunctionTableAccess, SymGetModuleBase, nullptr))
	{
		// note: max depth is 32 frames.
		if (!sf.AddrFrame.Offset || i++ >= 32)
			break;

		trace_func_t trace;

		// try to get module name.
		const auto module = SymGetModuleBase(GetCurrentProcess(), sf.AddrPC.Offset);
		if (!module)
		{
			trace.mod = XOR_STR("internal");
			trace.address = sf.AddrPC.Offset - game->base - 0x1000;
		}
		else
		{
			char name[MAX_PATH];
			if (!GetModuleFileNameA((HINSTANCE)module, name, sizeof(name)))
				trace.mod = XOR_STR("unknown");
			else
			{
				std::string mod(name);
				std::transform(mod.begin(), mod.end(), mod.begin(), ::tolower);
				trace.mod = std::filesystem::path(mod).filename().string();
			}

			trace.address = sf.AddrPC.Offset - (uintptr_t)module;
		}

		// try to get function name.
		char sym_buff[sizeof(IMAGEHLP_SYMBOL) + 255];
		auto sym = reinterpret_cast<IMAGEHLP_SYMBOL *>(sym_buff);
		sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) + 255;
		sym->MaxNameLength = 254;

		trace.name =
			SymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, nullptr, sym) ? sym->Name : XOR_STR("unknown");
		func(trace);
	}

	SymCleanup(GetCurrentProcess());
	return true;
}