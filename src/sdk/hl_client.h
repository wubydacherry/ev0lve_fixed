
#ifndef SDK_HL_CLIENT_H
#define SDK_HL_CLIENT_H

#include <sdk/client_entity.h>

namespace sdk
{
enum framestage
{
	frame_undefined = -1,
	frame_start,
	frame_net_update_start,
	frame_net_update_postdataupdate_start,
	frame_net_update_postdataupdate_end,
	frame_net_update_end,
	frame_render_start,
	frame_render_end
};

class hl_client_t
{
public:
	virtual int connect(void *factory, void *globals) = 0;
	virtual int disconnect() = 0;
	virtual int init(void *factory, void *globals) = 0;
	virtual void post_init() = 0;
	virtual void shutdown() = 0;
	virtual void level_init_pre_entity(char const *map_name) = 0;
	virtual void level_init_post_entity() = 0;
	virtual void level_shutdown() = 0;
	virtual client_class *get_all_classes() = 0;
};
} // namespace sdk

#endif // SDK_HL_CLIENT_H
