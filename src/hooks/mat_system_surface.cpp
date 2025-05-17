
#include <base/hook_manager.h>
#include <menu/menu.h>

using namespace sdk;
using namespace menu;

namespace hooks
{
namespace mat_system_surface
{
void __fastcall lock_cursor(mat_surface_t *surf, uint32_t edx)
{
	if (menu::menu.is_open())
		surf->unlock_cursor();
	else
		hook_manager.lock_cursor->call(surf, edx);
}
} // namespace mat_system_surface
} // namespace hooks
