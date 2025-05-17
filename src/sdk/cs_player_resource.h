
#ifndef SDK_CS_PLAYER_RESOURCE_H
#define SDK_CS_PLAYER_RESOURCE_H

#include <sdk/entity.h>
#include <sdk/macros.h>

namespace sdk
{
class cs_player_resource_t
{
public:
	struct
	{
		AVAR(offsets::player_resource, health, int32_t);

		AVAR(offsets::csplayer_resource, has_helmet, bool);
		AVAR(offsets::csplayer_resource, has_defuser, bool);
		AVAR(offsets::csplayer_resource, armor, int32_t);
		VAR(offsets::csplayer_resource, player_c4, int32_t);
		VAR(offsets::csplayer_resource, clan, const char *);
		VAR(offsets::csplayer_resource, crosshair_codes, const char *);
		VAR(offsets::csplayer_resource, bombsite_center_a, vec3);
		VAR(offsets::csplayer_resource, bombsite_center_b, vec3);
	} * data;
};
} // namespace sdk

#endif // SDK_CS_PLAYER_RESOURCE_H
