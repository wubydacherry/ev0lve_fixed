
#ifndef SDK_PREDICTION_H
#define SDK_PREDICTION_H

#include <sdk/cs_player.h>
#include <sdk/macros.h>

namespace sdk
{
class prediction_t
{
public:
	VIRTUAL(3,
			update(const int32_t start_frame, const bool valid_frame, const int32_t incoming_ack,
				   const int32_t outgoing_cmd),
			void(__thiscall *)(void *, int32_t, bool, int32_t, int32_t))
	(start_frame, valid_frame, incoming_ack, outgoing_cmd);
	VIRTUAL(13, set_local_view_angles(const angle &ang), void(__thiscall *)(void *, const angle &))(ang);
	VIRTUAL(14, is_active(), bool(__thiscall *)(void *))();

	void set_active(const bool active) { *reinterpret_cast<bool *>(uintptr_t(this) + 8) = active; }

	int32_t &get_predicted_commands() { return *reinterpret_cast<int32_t *>(uintptr_t(this) + 0x1c); }

	float &get_saved_time() { return *reinterpret_cast<float *>(uintptr_t(this) + 0x5c); }
};
} // namespace sdk

#endif // SDK_PREDICTION_H
