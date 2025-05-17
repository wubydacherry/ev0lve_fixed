
#ifndef SDK_STATE_H
#define SDK_STATE_H

#include <sdk/macros.h>

namespace sdk
{
inline static constexpr auto flow_outgoing = 0;
inline static constexpr auto flow_incoming = 1;
inline static constexpr auto net_frames_mask = 63;
inline static constexpr auto sv_max_unlag = .2f;

class net_message
{
public:
	VIRTUAL(7, get_type(), int32_t(__thiscall *)(void *))();
	VIRTUAL(8, get_group(), int32_t(__thiscall *)(void *))();
	VIRTUAL(9, get_name(), const char *(__thiscall *)(void *))();
};

struct shared_esp_data
{
	uint16_t id;
	uint8_t user_id;
	uint8_t weapon_id;
	uint32_t server_tick;
	vec3 pos;
};

inline constexpr auto shared_esp_size = 20;

struct svc_msg_voicedata_t
{
	char pad[8];
	int client;
	int audible_mask;
	uint64_t xuid;
	void *data;
	uint8_t proximity;
	uint8_t caster;
	char pad_30[2];
	int format;
	int sequence_bytes;
	uint32_t section_number;
	uint32_t uncompressed_sample_offset;

	void get(uint8_t *d)
	{
		memcpy(d, &xuid, 8);
		memcpy(d + 8, &sequence_bytes, shared_esp_size - 8);
	}
};

struct clc_msg_voicedata_t
{
	char pad[20];
	void *data;
	uint64_t xuid;
	int format;
	int sequence_bytes;
	uint32_t section_number;
	uint32_t uncompressed_sample_offset;
	int cached_size;
	uint32_t flags;
	char pad1[255];

	void set(uint8_t *d)
	{
		memcpy((void *)(uintptr_t(this) + 24), d, 8);
		memcpy((void *)(uintptr_t(this) + 36), d + 8, shared_esp_size - 8);
	}
};

class net_channel
{
public:
	VIRTUAL(1, get_address(), const char *(__thiscall *)(void *))();
	VIRTUAL(9, get_latency(const int flow), float(__thiscall *)(void *, int))(flow);
	VIRTUAL(10, get_avg_latency(const int flow), float(__thiscall *)(void *, int))(flow);
	VIRTUAL(40, send_net_msg(const uintptr_t msg, bool reliable = false, bool voice = false),
			bool(__thiscall *)(void *, uintptr_t, bool, bool))
	(msg, reliable, voice);
	VIRTUAL(46, send_datagram(), int(__thiscall *)(void *, void *))(nullptr);

	char pad_0000[20];
	bool processing_messages;
	bool should_delete;
	char pad_0016[2];
	int out_sequence_nr;
	int in_sequence_nr;
	int out_sequence_nr_ack;
	int out_reliable_state;
	int in_reliable_state;
	int choked_packets;
	char pad_0030[1044];
};

struct network_string_table_t
{
	VIRTUAL(3, get_num_strings(), int32_t(__thiscall *)(void *))();
	VIRTUAL(11, get_string_user_data(int32_t num, int32_t *len), void *(__thiscall *)(void *, int32_t, int32_t *))
	(num, len);
};

class client_state_t
{
public:
	inline void request_full_update() { delta_tick = -1; }

	inline void process_sockets()
	{
		if (net_channel && !paused)
			((void(__fastcall *)(uint32_t, uintptr_t))game->engine.at(functions::net_channel::process_sockets))(
				0, uintptr_t(this) + 12);
	}

	void __declspec(noinline) netmsg_tick_ctor(uintptr_t thisptr, int32_t tick)
	{
		const auto ctor = game->engine.at(functions::net_channel::netmsg_tick_ctor);
		const auto host = game->engine.at(functions::net_channel::host);
		const auto xmm0_param = *(float *)*(uintptr_t *)(host + 0x4); // host_framestarttime_std_deviation
		const auto xmm3_param = *(float *)*(uintptr_t *)(host + 0x8 + 0x4); // host_computationtime_std_deviation
		const auto xmm2_param = *(float *)*(uintptr_t *)(host + 0x10 + 0x4); // host_computationtime

		__asm
		{
				movss xmm0, xmm0_param
				push xmm0_param
				movss xmm3, xmm3_param
				movss xmm2, xmm2_param
				push tick
				mov ecx, thisptr
				mov eax, [ctor]
				call eax
		}
	}

	inline void send_netmsg_tick()
	{
		if (!net_channel || paused)
			return;

		auto netmsg_tick_dtor = (void(__thiscall *)(void *))game->engine.at(functions::net_channel::netmsg_tick_dtor);
		uint8_t net_msg[0x44];
		netmsg_tick_ctor((uintptr_t)net_msg, delta_tick); // CNetMsg_Tick_Constructor
		net_channel->send_net_msg((uintptr_t)net_msg);
		netmsg_tick_dtor(net_msg); // CNetMsg_Tick_Destructor
	}

	MEMBER(0x52c0, user_info_table, network_string_table_t *);

	uint8_t pad_0000[156];
	net_channel *net_channel;
	uint8_t pad_00A0[104];
	uint32_t signon_state;
	uint32_t pad;
	double next_cmd_time;
	uint8_t pad_010C[80];
	uint32_t cur_clock_offset;
	uint32_t server_tick;
	uint32_t client_tick;
	int32_t delta_tick;
	bool paused;
	uint8_t pad_0179[275];
	char level_name[260];
	uint8_t pad_0390[18844];
	int32_t last_command;
	int32_t choked_commands;
	int32_t last_command_ack;
	int32_t last_server_tick;
	int32_t command_ack;
};
} // namespace sdk

#endif // SDK_STATE_H
