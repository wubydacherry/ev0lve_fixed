
#ifndef SDK_ENGINE_CLIENT_H
#define SDK_ENGINE_CLIENT_H

#include <sdk/macros.h>
#include <sdk/vec2.h>
#include <sdk/vec3.h>

namespace sdk
{
struct model;
struct player_info
{
private:
	__int64 unknown = 0;

public:
	union
	{
		__int64 steam_id64;
		struct
		{
			__int32 xuid_low;
			__int32 xuid_high;
		};
	};

	char name[128];
	int user_id;
	char steam_id[20];

private:
	char pad[0x10]{};
	unsigned long steam_id_2 = 0;

public:
	char friends_name[128];
	bool fakeplayer;
	bool ishltv;
	unsigned int customfiles[4];
	unsigned char filesdownloaded;
};

class bsp_tree_query
{
public:
	VIRTUAL(6, list_leaves_in_box(sdk::vec3 *mins, sdk::vec3 *maxs, uint16_t *list, int32_t list_size),
			int32_t(__thiscall *)(void *, sdk::vec3 *, sdk::vec3 *, uint16_t *, int32_t))
	(mins, maxs, list, list_size);
};

class engine_client_t
{
private:
	VIRTUAL(5, get_screen_size(int32_t &x, int32_t &y), void(__thiscall *)(void *, int32_t &, int32_t &))(x, y);
	VIRTUAL(18, get_view_angles(angle &ang), void(__thiscall *)(void *, angle &))(ang);

public:
	VIRTUAL(7, clientcmd(const char *str), void(__thiscall *)(void *, const char *))(str);
	VIRTUAL(8, get_player_info(const uint32_t index, player_info *info),
			bool(__thiscall *)(void *, uint32_t, player_info *))
	(index, info);
	VIRTUAL(9, get_player_for_user_id(const int index), int(__thiscall *)(void *, int))(index);
	VIRTUAL(11, con_is_visible(), bool(__thiscall *)(void *))();
	VIRTUAL(12, get_local_player(), uint32_t(__thiscall *)(void *))();
	VIRTUAL(13, load_model(const char *name, bool prop), model *(__thiscall *)(void *, const char *, bool))(name, prop);
	VIRTUAL(14, get_last_timestamp(), float(__thiscall *)(void *))();
	VIRTUAL(19, set_view_angles(angle &ang), void(__thiscall *)(void *, angle &))(ang);
	VIRTUAL(20, get_max_clients(), uint32_t(__thiscall *)(void *))();
	VIRTUAL(22, binding_for_key(uint32_t key), const char *(__thiscall *)(void *, uint32_t))(key);
	VIRTUAL(26, is_ingame(), bool(__thiscall *)(void *))();
	VIRTUAL(27, is_connected(), bool(__thiscall *)(void *))();
	VIRTUAL(43, get_bsp_tree_query(), bsp_tree_query *(__thiscall *)(void *))();
	VIRTUAL(59, fire_events(), void(__thiscall *)(void *))();
	VIRTUAL(90, is_paused(), bool(__thiscall *)(void *))();
	VIRTUAL(114, clientcmd_unrestricted(const char *str, const char *flag),
			void(__thiscall *)(void *, const char *, const char *))
	(str, flag);

	inline vec2 get_screen_size()
	{
		int32_t x, y;
		get_screen_size(x, y);
		return {static_cast<float>(x), static_cast<float>(y)};
	}
	inline player_info get_player_info(const uint32_t index)
	{
		player_info info;
		get_player_info(index, &info);
		return info;
	}
	inline angle get_view_angles()
	{
		angle ang;
		get_view_angles(ang);
		return ang;
	}
};
} // namespace sdk

#endif // SDK_ENGINE_CLIENT_H
