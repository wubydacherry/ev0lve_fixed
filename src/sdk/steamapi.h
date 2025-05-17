
#ifndef SDK_STEAMAPI_H
#define SDK_STEAMAPI_H

#include <sdk/macros.h>

namespace sdk
{
struct steam_id_t
{
	explicit steam_id_t(uint64_t v)
	{
		account_id = v & 0xFFFFFFFFull;
		account_instance = (v >> 32ull) & 0xFFFFull;
		account_type = int((v >> 52ull) & 0xFull);
		universe = int((v >> 56ull) & 0xFFull);
	}

	uint32_t account_id : 32;
	unsigned int account_instance : 20;
	int account_type : 4;
	int universe : 8;
};

class steam_friends_t
{
public:
	VIRTUAL(34, get_small_friend_avatar(steam_id_t xuid), int(__thiscall *)(void *, steam_id_t))(xuid);
};

class steam_utils_t
{
public:
	VIRTUAL(5, get_image_size(int image, uint32_t *w, uint32_t *h),
			bool(__thiscall *)(void *, int, uint32_t *, uint32_t *))
	(image, w, h);
	VIRTUAL(6, get_image_rgba(int image, uint8_t *rgba, int size), bool(__thiscall *)(void *, int, uint8_t *, int))
	(image, rgba, size);
};

class steam_context_t
{
public:
	void *client;
	void *user;
	steam_friends_t *friends;
	steam_utils_t *utils;
};
} // namespace sdk

#endif // SDK_STEAMAPI_H
