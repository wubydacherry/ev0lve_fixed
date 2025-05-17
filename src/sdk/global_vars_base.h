
#ifndef SDK_GLOBAL_VARS_BASE_H
#define SDK_GLOBAL_VARS_BASE_H

#include <sdk/macros.h>

namespace sdk
{
class global_vars_base_t
{
public:
	float realtime;
	int framecount;
	float absoluteframetime;
	float absoluteframestarttimestddev;
	float curtime;
	float frametime;
	int max_clients;
	int tickcount;
	float interval_per_tick;
	float interpolation_amount;
	int sim_ticks_this_frame;
	int network_protocol;
	void *save_data;
	bool client;
	bool remote_client;
};
} // namespace sdk

#endif // SDK_GLOBAL_VARS_BASE_H
