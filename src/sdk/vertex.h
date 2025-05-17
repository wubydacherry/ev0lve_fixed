
#ifndef SDK_VERTEX_H
#define SDK_VERTEX_H

#include <sdk/vec2.h>

namespace sdk
{
struct vertex_t
{
	vec2 pos;
	vec2 cord;

	vertex_t() = default;
	vertex_t(const vec2 &pos, const vec2 &coord = vec2(0, 0)) : pos(pos), cord(coord) {}
};
} // namespace sdk

#endif // SDK_VERTEX_H
