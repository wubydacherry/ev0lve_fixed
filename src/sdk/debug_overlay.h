
#ifndef SDK_DEBUG_OVERLAY_H
#define SDK_DEBUG_OVERLAY_H

#include <sdk/color.h>
#include <sdk/macros.h>
#include <sdk/vec2.h>

namespace sdk
{
class debug_overlay_t
{
public:
	virtual void entity_text_overlay(const int ent_index, const int line_offset, const float duration, const int r,
									 const int g, const int b, const int a, const char *format, ...) = 0;
	virtual void box_overlay(const vec3 &origin, const vec3 &mins, const vec3 &max, angle const &orientation,
							 const int r, const int g, const int b, const int a, const float duration) = 0;
	virtual void sphere_overlay(const vec3 &origin, float radius, int theta, int phi, int r, int g, int b, int a,
								float duration) = 0;
	virtual void triangle_overlay(const vec3 &p1, const vec3 &p2, const vec3 &p3, const int r, const int g, const int b,
								  const int a, const bool no_depth_test, const float duration) = 0;

private:
	virtual void pad0() = 0;

public:
	virtual void line_overlay(const vec3 &origin, const vec3 &dest, const int r, const int g, const int b,
							  const bool no_depth_test, const float duration) = 0;
	VIRTUAL(21,
			box_overlay_2(const vec3 &origin, const vec3 &mins, const vec3 &maxs, const angle &orientation,
						  const color &face, const color &edge, const float duration),
			void(__thiscall *)(void *, const vec3 &, const vec3 &, const vec3 &, const angle &, const color &,
							   const color &, float))
	(origin, mins, maxs, orientation, face, edge, duration);
	VIRTUAL(23,
			capsule_overlay(const vec3 &mins, const vec3 &maxs, float radius, const int r, const int g, const int b,
							const int a, const float duration),
			void(__thiscall *)(void *, const vec3 &, const vec3 &, float &, int, int, int, int, float, bool, bool))
	(mins, maxs, radius, r, g, b, a, duration, false, true);
};
} // namespace sdk

#endif // SDK_DEBUG_OVERLAY_H
