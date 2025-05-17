
#ifndef SDK_INPUT_H
#define SDK_INPUT_H

#include <base/game.h>
#include <sdk/macros.h>
#include <sdk/vec3.h>

namespace sdk
{
inline static constexpr auto input_max = 150;

class user_cmd
{
public:
	enum buttons
	{
		attack = 1 << 0,
		jump = 1 << 1,
		duck = 1 << 2,
		forward = 1 << 3,
		back = 1 << 4,
		use = 1 << 5,
		cancel = 1 << 6,
		left = 1 << 7,
		right = 1 << 8,
		move_left = 1 << 9,
		move_right = 1 << 10,
		attack2 = 1 << 11,
		run = 1 << 12,
		reload = 1 << 13,
		alt1 = 1 << 14,
		alt2 = 1 << 15,
		score = 1 << 16,
		speed = 1 << 17,
		walk = 1 << 18,
		zoom = 1 << 19,
		weapon1 = 1 << 20,
		weapon2 = 1 << 21,
		bull_rush = 1 << 22,
		grenade1 = 1 << 23,
		grenade2 = 1 << 24,
		middle_attack = 1 << 25, // 14.5.2025
		use_or_reload = 1 << 26 //  14.5.2025
	};

	enum flags
	{
		on_ground = 1 << 0,
		ducking = 1 << 1
	};

	FUNC(game->client.at(functions::user_cmd_get_checksum), get_checksum(), uint32_t(__thiscall *)(user_cmd *))();

	virtual ~user_cmd() = default;

	int command_number;
	int tick_count;
	angle viewangles;
	vec3 aim_direction;
	float forwardmove;
	float sidemove;
	float upmove;
	int buttons;
	uint8_t impulse;
	int weapon_select;
	int weapon_select_subtype;
	int random_seed;
	short mousedx;
	short mousedy;
	bool predicted;
	angle headangles;
	int buttons2;

private:
	char pad[8];
};

class verified_user_cmd
{
public:
	user_cmd cmd;
	unsigned long crc;
};

class input_t
{
private:
	uint8_t pad[0xA9];

public:
	bool camera_in_third_person{};

	bool m_fCameraMovingWithMouse{};
	vec3 camera_offset;
	bool m_fCameraDistanceMove{};
	int m_nCameraOldX{};
	int m_nCameraOldY{};
	int m_nCameraX{};
	int m_nCameraY{};
	bool m_CameraIsOrthographic{};
	vec3 m_angPreviousViewAngles;
	vec3 m_angPreviousViewAnglesTilt;
	float m_flLastForwardMove{};
	int m_nClearInputState{};

	user_cmd *commands{};
	verified_user_cmd *verified_commands{};
};
} // namespace sdk

#endif // SDK_INPUT_H
