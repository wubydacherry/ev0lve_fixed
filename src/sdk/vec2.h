
#ifndef SDK_VEC2_H
#define SDK_VEC2_H

#include <sdk/intrinsics.h>

namespace sdk
{
class vec2
{
public:
	float x, y;

	vec2() : vec2(0.f, 0.f) {}
	vec2(const float x, const float y) : x(x), y(y) {}

	vec2 operator+(const vec2 &v) const { return vec2(x + v.x, y + v.y); }

	vec2 operator-(const vec2 &v) const { return vec2(x - v.x, y - v.y); }

	vec2 operator*(const vec2 &v) const { return vec2(x * v.x, y * v.y); }

	vec2 operator/(const vec2 &v) const { return vec2(x / v.x, y / v.y); }

	vec2 operator*(const float v) const { return vec2(x * v, y * v); }

	vec2 operator/(const float v) const { return vec2(x / v, y / v); }

	vec2 operator+=(const vec2 &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	vec2 operator-=(const vec2 &other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	vec2 operator*=(const vec2 &other)
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}

	vec2 operator/=(const vec2 &other)
	{
		x /= other.x;
		y /= other.y;
		return *this;
	}

	vec2 operator*=(const float other)
	{
		x *= other;
		y *= other;
		return *this;
	}

	vec2 operator/=(const float other)
	{
		x /= other;
		y /= other;
		return *this;
	}

	vec2 floor()
	{
		x = std::floor(x);
		y = std::floor(y);
		return *this;
	}

	inline float length() const { return sqrt_ps(x * x + y * y); }
};
} // namespace sdk

#endif // SDK_VEC2_H
