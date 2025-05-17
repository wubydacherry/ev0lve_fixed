
#ifndef SDK_RENDER_VIEW_H
#define SDK_RENDER_VIEW_H

#include <sdk/color.h>
#include <sdk/macros.h>
#include <sdk/math.h>

namespace sdk
{
struct v_plane
{
	vec3 normal;
	float dist;
};

using frustum_t = v_plane[6];

enum clear_flags
{
	view_clear_color = 1,
	view_clear_depth = 2,
	view_clear_full_target = 4,
	view_no_draw = 8,
	view_clear_obey_stencil = 16,
	view_clear_stencil = 32
};

class render_view_t
{
	VIRTUAL(6, set_color_modulation_internal(float const *col), void(__thiscall *)(void *, float const *))(col);

public:
	VIRTUAL(4, set_blend(const float blend), void(__thiscall *)(void *, float))(blend);
	VIRTUAL(5, get_blend(), float(__thiscall *)(void *))();
	VIRTUAL(44, push_3d_view(void *ren_ctx, const view_setup &setup, int flags, texture *target, frustum_t planes),
			void(__thiscall *)(void *, void *, const view_setup &, int, texture *, frustum_t))
	(ren_ctx, setup, flags, target, planes);
	VIRTUAL(46, pop_view(void *ren_ctx, frustum_t planes), void(__thiscall *)(void *, void *, frustum_t))
	(ren_ctx, planes);
	VIRTUAL(56,
			get_matrices_for_view(const view_setup &setup, vmatrix *w2v, vmatrix *v2p, vmatrix *w2proj, vmatrix *w2pix),
			void(__thiscall *)(void *, const view_setup &, vmatrix *, vmatrix *, vmatrix *, vmatrix *))
	(setup, w2v, v2p, w2proj, w2pix);

	void set_color_modulation(const color color)
	{
		float col[4] = {color.red() / 255.f, color.green() / 255.f, color.blue() / 255.f, color.alpha() / 255.f};
		set_color_modulation_internal(col);
	}
};
} // namespace sdk

#endif // SDK_RENDER_VIEW_H
