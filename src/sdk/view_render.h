
#ifndef SDK_VIEW_RENDER_H
#define SDK_VIEW_RENDER_H

#include <sdk/macros.h>
#include <sdk/mat.h>
#include <sdk/vec3.h>

namespace sdk
{
class view_setup
{
public:
	int32_t x;
	int32_t x_old;
	int32_t y;
	int32_t y_old;
	int32_t width;
	int32_t width_old;
	int32_t height;
	int32_t height_old;
	bool ortho;
	float ortho_left;
	float ortho_top;
	float ortho_right;
	float ortho_bottom;
	bool custom_view_matrix;
	mat3x4 view_matrix;

private:
	char pad[0x48];

public:
	float fov;
	float fov_viewmodel;
	vec3 origin;
	angle angles;
	float z_near;
	float z_far;
	float z_near_viewmodel;
	float z_far_viewmodel;
	float aspect_ratio;
	float near_blur_depth;
	float near_focus_depth;
	float far_focus_depth;
	float far_blur_depth;
	float near_blur_radius;
	float far_blur_radius;
	float dof_quality;
	int32_t motion_blur_mode;
	float shutter_time;
	vec3 shutter_open_pos;
	angle shutter_open_angle;
	vec3 shutter_close_pos;
	angle shutter_close_angle;
	float off_center_top;
	float off_center_bottom;
	float off_center_left;
	float off_center_right;
	bool off_center : 1;
	bool render_to_subrect_of_larger_screen : 1;
	bool do_bloom_and_tone_mapping : 1;
	bool do_depth_of_field : 1;
	bool hdr_target : 1;
	bool draw_world_normal : 1;
	bool cull_front_faces : 1;
	bool cache_full_scene_state : 1;
	bool csm_view : 1;
};

struct bob_state_t
{
	float bob_time;
	float last_bob_time;
	float last_speed;
	float vertical_bob;
	float lateral_bob;
	float raw_vertical_bob;
	float raw_lateral_bob;
};

class view_render_t
{
public:
	VIRTUAL(13, get_view_setup(), view_setup *const(__thiscall *)(void *))();
};
} // namespace sdk

#endif // SDK_VIEW_RENDER_H
