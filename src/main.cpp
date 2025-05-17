
#include <base/game.h>
#include <thread>
#include <util/anti_debug.h>
#include <util/memory.h>

bool __stdcall DllMain(uintptr_t base, uint32_t reason_for_call, uint32_t reserved)
{
	if (reason_for_call == XOR_32(DLL_PROCESS_ATTACH))
	{
#ifdef _DEBUG
		AllocConsole();
		freopen( "CONOUT$", "w", stdout );

		printf("[ev0] initializing\n");

#endif

		LdrDisableThreadCalloutsForDll(PVOID(LOOKUP_MODULE("client.dll")));
		std::thread(
			[base, reserved]()
			{
				game = std::make_unique<game_t>(base, reserved);
				game->init();
			})
			.detach();
	}

	return true;
}
