
#ifndef SDK_VEC4_H
#define SDK_VEC4_H

namespace sdk
{
class vec4
{
public:
	vec4() = default;
	vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float x, y, z, w;
};
} // namespace sdk

#endif // SDK_VEC4_H
