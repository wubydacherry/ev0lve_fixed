
#ifndef SDK_VEC3_H
#define SDK_VEC3_H

#include <sdk/intrinsics.h>

namespace sdk
{
class vec3
{
public:
	float x, y, z;

	__forceinline vec3() : vec3(0.f, 0.f, 0.f) {}

	__forceinline vec3(const float x, const float y, const float z) : x(x), y(y), z(z) {}

	__forceinline vec3 operator+(const vec3 &v) const { return vec3(x + v.x, y + v.y, z + v.z); }

	__forceinline vec3 operator-(const vec3 &v) const { return vec3(x - v.x, y - v.y, z - v.z); }

	__forceinline vec3 operator*(const vec3 &v) const { return vec3(x * v.x, y * v.y, z * v.z); }

	__forceinline vec3 operator/(const vec3 &v) const { return vec3(x / v.x, y / v.y, z / v.z); }

	__forceinline vec3 operator*(const float v) const { return vec3(x * v, y * v, z * v); }

	__forceinline vec3 operator/(const float v) const { return vec3(x / v, y / v, z / v); }

	__forceinline vec3 operator+=(const vec3 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	__forceinline vec3 operator-=(const vec3 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	__forceinline vec3 operator*=(const vec3 &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	__forceinline vec3 operator/=(const vec3 &other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;
		return *this;
	}

	__forceinline vec3 operator*=(const float other)
	{
		x *= other;
		y *= other;
		z *= other;
		return *this;
	}

	__forceinline vec3 operator/=(const float other)
	{
		x /= other;
		y /= other;
		z /= other;
		return *this;
	}

	__forceinline bool operator==(const vec3 &other) const { return x == other.x && y == other.y && z == other.z; }

	__forceinline bool operator!=(const vec3 &other) const { return x != other.x || y != other.y || z != other.z; }

	__forceinline float operator[](int i) const { return ((float *)this)[i]; }

	__forceinline float &operator[](int i) { return ((float *)this)[i]; }

	__forceinline bool is_zero(float tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance && y > -tolerance && y < tolerance && z > -tolerance && z < tolerance);
	}

	__forceinline float length() const { return sqrt_ps(x * x + y * y + z * z); }

	__forceinline float length_sqr() const { return x * x + y * y + z * z; }

	__forceinline float length2d() const { return sqrt_ps(x * x + y * y); }

	__forceinline float length2d_sqr() const { return x * x + y * y; }

	__forceinline float dot(const vec3 &other) const { return x * other.x + y * other.y + z * other.z; }

	__forceinline float dot(const float *other) const { return x * other[0] + y * other[1] + z * other[2]; }

	__forceinline vec3 cross(const vec3 &other) const
	{
		return vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
	}

	__forceinline vec3 normalize()
	{
		const auto l = length();

		if (l > 0)
		{
			x /= l;
			y /= l;
			z /= l;
		}

		return *this;
	}

	__forceinline bool is_valid() const
	{
		return std::isfinite(this->x) && std::isfinite(this->y) && std::isfinite(this->z);
	}
};

class vec3_aligned : public vec3
{
public:
	__forceinline vec3_aligned() : vec3(0.f, 0.f, 0.f) {}
	__forceinline vec3_aligned(const vec3 &o)
	{
		x = o.x;
		y = o.y;
		z = o.z;
	}

private:
	float w{};
};

using angle = vec3;
} // namespace sdk

#endif // SDK_VEC3_H
