
#ifndef SDK_ENGINE_SOUND_H
#define SDK_ENGINE_SOUND_H

#include <sdk/macros.h>
#include <sdk/vec3.h>

namespace sdk
{
struct keyvalues;

struct sfx_table
{
	VIRTUAL(0, get_name(char *buf, size_t len), const char *(__thiscall *)(void *, char *, size_t))(buf, len);
};

struct start_sound_params
{
	int32_t user_data;
	int32_t sound_source;
	int32_t ent_channel;
	sfx_table *sfx;
	vec3 origin;
	vec3 direction;
	float volume;
	int32_t sound_level;
	int32_t flags;
	int32_t pitch;
	float delay;
	int speaker_entity;
	int32_t initial_stream_position;
	int32_t skip_initial_samples;
	int32_t queued_guid;
	uint32_t sound_script_hash;
	const char *sound_entry_name;
	keyvalues *kv;
	float stack_elapsed_time;
	float stack_elapsed_stop_time;
	bool static_sound : 1;
	bool update_positions : 1;
	bool from_server : 1;
	bool tool_sound : 1;
	bool is_script_handle : 1;
	bool delayed_start : 1;
	bool in_eye_sound : 1;
	bool hrft_follow_entity : 1;
	bool hrft_bilinear : 1;
	bool hrft_lock : 1;
};
} // namespace sdk

#endif // SDK_ENGINE_SOUND_H
