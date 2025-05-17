
#ifndef SDK_MATH_H
#define SDK_MATH_H

#include <sdk/color.h>
#include <sdk/engine_client.h>
#include <sdk/global_vars_base.h>
#include <sdk/intrinsics.h>
#include <sdk/mat.h>
#include <sdk/model_info_client.h>
#include <sdk/vec3.h>
#include <sdk/vertex.h>

#define RAD2DEG(x) ((float)(x) * (float)(180.f / sdk::pi))
#define DEG2RAD(x) ((float)(x) * (float)(sdk::pi / 180.f))
#define TIME_TO_TICKS(dt) ((int)(0.5f + (float)(dt) / game->globals->interval_per_tick))
#define TICKS_TO_TIME(t) (game->globals->interval_per_tick * (t))

namespace sdk
{
constexpr auto pi = 3.14159265358979323846;
constexpr auto pi_2 = 6.283186f;
constexpr auto rad_pi = 57.295779513082f;
constexpr auto sqrt2 = 1.41421356237309504880f;

constexpr auto pitch_bounds = 89.f;
constexpr auto yaw_bounds = 360.f;
constexpr auto roll_bounds = 50.f;

constexpr auto forward_bounds = 450.f;
constexpr auto side_bounds = 450.f;
constexpr auto up_bounds = 320.f;

constexpr auto max_bones = 128;

using vmatrix = float[4][4];

__forceinline void normalize(angle &angle)
{
	if (!angle.is_valid())
	{
		angle = {0.f, 0.f, 0.f};
		return;
	}

	angle.x = std::clamp(std::remainderf(angle.x, yaw_bounds), -pitch_bounds, pitch_bounds);
	angle.y = std::remainderf(angle.y, yaw_bounds);
	angle.z = std::clamp(angle.z, -roll_bounds, roll_bounds);
}

__forceinline float vector_normalize(vec3 &vec)
{
	const auto sqr_len = vec.length_sqr() + 1.0e-10f;
	float invlen;
	const auto xx = _mm_load_ss(&sqr_len);
	auto xr = _mm_rsqrt_ss(xx);
	auto xt = _mm_mul_ss(xr, xr);
	xt = _mm_mul_ss(xt, xx);
	xt = _mm_sub_ss(_mm_set_ss(3.f), xt);
	xt = _mm_mul_ss(xt, _mm_set_ss(.5f));
	xr = _mm_mul_ss(xr, xt);
	_mm_store_ss(&invlen, xr);
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
	return sqr_len * invlen;
}

__forceinline void normalize_move(float &forward, float &side, float &up)
{
	auto dir = vec3(forward, side, up);

	if (dir.x > forward_bounds || dir.x < -forward_bounds)
	{
		const auto old = dir.x;
		dir.x = std::clamp(dir.x, -forward_bounds, forward_bounds);
		dir.y *= dir.x / old;
		dir.z *= dir.x / old;
	}

	if (dir.y > side_bounds || dir.y < -side_bounds)
	{
		const auto old = dir.y;
		dir.y = std::clamp(dir.y, -side_bounds, side_bounds);
		dir.x *= dir.y / old;
		dir.z *= dir.y / old;
	}

	if (dir.z > up_bounds || dir.z < -up_bounds)
	{
		const auto old = dir.z;
		dir.z = std::clamp(dir.z, -up_bounds, up_bounds);
		dir.x *= dir.z / old;
		dir.y *= dir.z / old;
	}

	forward = std::clamp(dir.x, -forward_bounds, forward_bounds);
	side = std::clamp(dir.y, -side_bounds, side_bounds);
	up = std::clamp(dir.z, -up_bounds, up_bounds);
}

__forceinline int32_t random_int(int32_t min, int32_t max)
{
	return ((int32_t(__cdecl *)(int32_t, int32_t))game->vstdlib.at(functions::random_int))(min, max);
}

__forceinline float random_float(float min, float max)
{
	return ((float(__cdecl *)(float, float))game->vstdlib.at(functions::random_float))(min, max);
}

__forceinline void random_seed(uint32_t seed)
{
	((void(__cdecl *)(uint32_t))game->vstdlib.at(functions::random_seed))(seed);
}

__forceinline bool screen_transform(const vec3& in, vec3& out)
{
	static std::uintptr_t view_matrix;
	if (!view_matrix)
	{
		view_matrix = game->client.at(globals::world_to_screen);
		view_matrix += 3;
		view_matrix = *reinterpret_cast<std::uintptr_t*>(view_matrix);
		view_matrix += 176;
	}

	using vmatrix = float[4][4];
	const auto& w2s = *(vmatrix*)view_matrix; //*(vmatrix*) game->client.at(globals::world_to_screen);

   //    using vmatrix = float[4][4];
    //    const auto& w2s = *(vmatrix*) game->client.at(globals::world_to_screen);
        out.x = w2s[0][0] * in.x + w2s[0][1] * in.y + w2s[0][2] * in.z + w2s[0][3];
        out.y = w2s[1][0] * in.x + w2s[1][1] * in.y + w2s[1][2] * in.z + w2s[1][3];
        out.z = 0.0f;

        const auto w = w2s[3][0] * in.x + w2s[3][1] * in.y + w2s[3][2] * in.z + w2s[3][3];
        if (w < .001f)
        {
            out.x *= 100000.f;
            out.y *= 100000.f;
            return false;
        }

        const auto invw = 1.f / w;
        out.x *= invw;
        out.y *= invw;
        return true;
    }

__forceinline bool world_to_screen(const vec3 &in, vec3 &out)
{
	const auto result = screen_transform(in, out);
	const auto size = game->engine_client->get_screen_size();
	out.x = (size.x / 2.f) + (out.x * size.x) / 2.f;
	out.y = (size.y / 2.f) - (out.y * size.y) / 2.f;
	return result;
}

__forceinline angle calc_angle(const vec3 &from, const vec3 &to)
{
	static const auto ang_zero = angle(0.0f, 0.0f, 0.0f);

	const auto delta = from - to;
	if (delta.length() <= 0.0f)
		return ang_zero;

	if (delta.z == 0.0f && delta.length() == 0.0f)
		return ang_zero;

	if (delta.y == 0.0f && delta.x == 0.0f)
		return ang_zero;

	angle angles;
	angles.x = asinf(delta.z / delta.length()) * rad_pi;
	angles.y = atanf(delta.y / delta.x) * rad_pi;
	angles.z = 0.0f;

	if (delta.x >= 0.0f)
		angles.y += 180.f;

	normalize(angles);
	return angles;
}

__forceinline float angle_diff(const float end, const float start)
{
	auto delta = fmodf(end - start, 360.0f);

	if (end > start)
	{
		if (delta >= 180.f)
			delta -= 360.f;
	}
	else if (delta <= -180.f)
		delta += 360.f;

	return delta;
}

__forceinline float anglemod(float a) { return (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535); }

__forceinline float approach_angle(float target, float value, float speed)
{
	target = anglemod(target);
	value = anglemod(value);

	auto delta = target - value;

	if (speed < 0.f)
		speed = -speed;

	if (delta < -180.f)
		delta += 360.f;
	else if (delta > 180.f)
		delta -= 360.f;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

__forceinline void sincos(float x, float *s, float *c)
{
	*s = std::sin(x);
	*c = std::cos(x);
}

__forceinline mat3x4 angle_matrix(const angle &ang)
{
	mat3x4 result{};

	float sr, sp, sy, cr, cp, cy;
	sdk::sincos(DEG2RAD(ang.x), &sp, &cp);
	sdk::sincos(DEG2RAD(ang.y), &sy, &cy);
	sdk::sincos(DEG2RAD(ang.z), &sr, &cr);

	result[0][0] = cp * cy;
	result[1][0] = cp * sy;
	result[2][0] = -sp;

	const auto crcy = cr * cy;
	const auto crsy = cr * sy;
	const auto srcy = sr * cy;
	const auto srsy = sr * sy;
	result[0][1] = sp * srcy - crsy;
	result[1][1] = sp * srsy + crcy;
	result[2][1] = sr * cp;

	result[0][2] = (sp * crcy + srsy);
	result[1][2] = (sp * crsy - srcy);
	result[2][2] = cr * cp;

	return result;
}

__forceinline mat3x4 angle_matrix(const angle &ang, const vec3 &pos)
{
	auto result = angle_matrix(ang);
	result[0][3] = pos.x;
	result[1][3] = pos.y;
	result[2][3] = pos.z;
	return result;
}

__forceinline quaternion angle_quaternion(const angle &ang)
{
	float sr, sp, sy, cr, cp, cy;
	sdk::sincos(.5f * DEG2RAD(ang.x), &sp, &cp);
	sdk::sincos(.5f * DEG2RAD(ang.y), &sy, &cy);
	sdk::sincos(.5f * DEG2RAD(ang.z), &sr, &cr);

	const auto srXcp = sr * cp, crXsp = cr * sp;
	const auto crXcp = cr * cp, srXsp = sr * sp;
	return {srXcp * cy - crXsp * sy, crXsp * cy + srXcp * sy, crXcp * sy - srXsp * cy, crXcp * cy + srXsp * sy};
}

__forceinline mat3x4 matrix_invert(const mat3x4 &in)
{
	mat3x4 out;
	out[0][0] = in[0][0];
	out[0][1] = in[1][0];
	out[0][2] = in[2][0];

	out[1][0] = in[0][1];
	out[1][1] = in[1][1];
	out[1][2] = in[2][1];

	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
	out[2][2] = in[2][2];

	vec3 tmp(in[0][3], in[1][3], in[2][3]);

	out[0][3] = -tmp.dot(out[0]);
	out[1][3] = -tmp.dot(out[1]);
	out[2][3] = -tmp.dot(out[2]);

	return out;
}

__forceinline vertex_t rotate_vertex(const vec2 &o, const vertex_t &v, const float angle)
{
	float t = DEG2RAD(angle), c = cos(t), s = sin(t);

	return vertex_t{{o.x + (v.pos.x - o.x) * c - (v.pos.y - o.y) * s, o.y + (v.pos.x - o.x) * s + (v.pos.y - o.y) * c}};
}

__forceinline float dot_product(const float *v1, const float *v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

__forceinline void cross_product(const float *v1, const float *v2, float *cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

__forceinline void vector_transform(const vec3 &in1, const mat3x4 &in2, vec3 &out)
{
	out.x = dot_product(&in1.x, in2[0]) + in2[0][3];
	out.y = dot_product(&in1.x, in2[1]) + in2[1][3];
	out.z = dot_product(&in1.x, in2[2]) + in2[2][3];
}

__forceinline vec3 vector_rotate(const vec3 &in1, const mat3x4 &in2)
{
	vec3 out;
	out.x = dot_product(&in1.x, in2[0]);
	out.y = dot_product(&in1.x, in2[1]);
	out.z = dot_product(&in1.x, in2[2]);
	return out;
}

__forceinline void vector_vectors(const vec3 &forward, vec3 &right, vec3 &up)
{
	if (forward.x == 0.f && forward.y == 0.f)
	{
		right.x = 0.f;
		right.y = -1.f;
		right.z = 0.f;
		up.x = -forward.z;
		up.y = 0.f;
		up.z = 0.f;
	}
	else
	{
		float tmp[] = {0.f, 0.f, 1.f};
		cross_product(&forward.x, tmp, &right.x);
		right.normalize();
		cross_product(&right.x, &forward.x, &up.x);
		up.normalize();
	}
}

__forceinline mat3x4 vector_matrix(const vec3 &forward)
{
	mat3x4 mat{};
	vec3 right, up;
	vector_vectors(forward, right, up);

	mat[0][0] = forward.x;
	mat[1][0] = forward.y;
	mat[2][0] = forward.z;
	mat[0][1] = -right.x;
	mat[1][1] = -right.y;
	mat[2][1] = -right.z;
	mat[0][2] = up.x;
	mat[1][2] = up.y;
	mat[2][2] = up.z;

	return mat;
}

__forceinline void vector_angles(const vec3 &in, vec3 &out)
{
	if (in.z == 0.f && in.x == 0.f)
	{
		out.y = 0.f;

		if (in.z > 0.f)
			out.x = 90.f;
		else
			out.x = 270.f;
	}
	else
	{
		out.x = RAD2DEG(atan2f(-in.z, in.length2d()));
		out.y = RAD2DEG(atan2f(in.y, in.x));

		if (out.x < 0.0f)
			out.x += 360.f;

		if (out.y < 0.0f)
			out.y += 360.f;
	}

	out.x -= floorf(out.x / 360.f + .5f) * 360.f;
	out.y -= floorf(out.y / 360.f + .5f) * 360.f;

	if (out.x > 89.f)
		out.x = 89.f;
	else if (out.x < -89.f)
		out.x = -89.f;
}

__forceinline void angle_vectors(const angle &ang, vec3 &forward)
{
	const auto sp = sin(DEG2RAD(ang.x)), cp = cos(DEG2RAD(ang.x)), sy = sin(DEG2RAD(ang.y)), cy = cos(DEG2RAD(ang.y));

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

__forceinline void angle_vectors(const angle &ang, vec3 &forward, vec3 &right, vec3 &up)
{
	const auto sp = sin(DEG2RAD(ang.x)), cp = cos(DEG2RAD(ang.x)), sy = sin(DEG2RAD(ang.y)), cy = cos(DEG2RAD(ang.y)),
			   sr = sin(DEG2RAD(ang.z)), cr = cos(DEG2RAD(ang.z));

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;

	right.x = -1.f * sr * sp * cy + -1.f * cr * -sy;
	right.y = -1.f * sr * sp * sy + -1.f * cr * cy;
	right.z = -1.f * sr * cp;

	up.x = cr * sp * cy + -sr * -sy;
	up.y = cr * sp * sy + -sr * cy;
	up.z = cr * cp;
}

__forceinline vec3 matrix_position(const mat3x4 &matrix) { return vec3(matrix[0][3], matrix[1][3], matrix[2][3]); }

__forceinline angle matrix_angles(const mat3x4 &matrix)
{
	float forward[3];
	float left[3];

	forward[0] = matrix[0][0];
	forward[1] = matrix[1][0];
	forward[2] = matrix[2][0];

	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];

	const auto up = matrix[2][2];

	const auto xy_dist = sqrt_ps(forward[0] * forward[0] + forward[1] * forward[1]);

	if (xy_dist > 0.001f)
		return angle(RAD2DEG(atan2f(-forward[2], xy_dist)), RAD2DEG(atan2f(forward[1], forward[0])),
					 RAD2DEG(atan2f(left[2], up)));

	return angle(RAD2DEG(atan2f(-forward[2], xy_dist)), RAD2DEG(atan2f(-left[0], left[1])), 0);
}

__forceinline mat3x4 quaternion_matrix(const quaternion_aligned &q, const vec3 &pos)
{
	mat3x4 result{};
	result[0][0] = 1.f - 2.f * q.y * q.y - 2.f * q.z * q.z;
	result[1][0] = 2.f * q.x * q.y + 2.f * q.w * q.z;
	result[2][0] = 2.f * q.x * q.z - 2.f * q.w * q.y;

	result[0][1] = 2.f * q.x * q.y - 2.f * q.w * q.z;
	result[1][1] = 1.f - 2.f * q.x * q.x - 2.f * q.z * q.z;
	result[2][1] = 2.f * q.y * q.z + 2.f * q.w * q.x;

	result[0][2] = 2.f * q.x * q.z + 2.f * q.w * q.y;
	result[1][2] = 2.f * q.y * q.z - 2.f * q.w * q.x;
	result[2][2] = 1.f - 2.f * q.x * q.x - 2.f * q.y * q.y;

	result[0][3] = pos.x;
	result[1][3] = pos.y;
	result[2][3] = pos.z;
	return result;
}

__forceinline mat3x4 concat_transforms(const mat3x4 &in1, const mat3x4 &in2)
{
	auto last_mask = *(m128 *)(&component_mask[3]);
	auto row_a0 = load_simd(in1[0]);
	auto row_a1 = load_simd(in1[1]);
	auto row_a2 = load_simd(in1[2]);
	auto row_b0 = load_simd(in2[0]);
	auto row_b1 = load_simd(in2[1]);
	auto row_b2 = load_simd(in2[2]);

	auto a0 = splat_x(row_a0);
	auto a1 = splat_y(row_a0);
	auto a2 = splat_z(row_a0);
	auto mul_00 = mul_simd(a0, row_b0);
	auto mul_01 = mul_simd(a1, row_b1);
	auto mul_02 = mul_simd(a2, row_b2);

	auto temp = add_simd(mul_01, mul_02);
	auto out0 = add_simd(mul_00, temp);

	a0 = splat_x(row_a1);
	a1 = splat_y(row_a1);
	a2 = splat_z(row_a1);

	auto mul10 = mul_simd(a0, row_b0);
	auto mul11 = mul_simd(a1, row_b1);
	auto mul12 = mul_simd(a2, row_b2);
	temp = add_simd(mul11, mul12);
	auto out1 = add_simd(mul10, temp);

	a0 = splat_x(row_a2);
	a1 = splat_y(row_a2);
	a2 = splat_z(row_a2);
	auto mul_20 = mul_simd(a0, row_b0);
	auto mul_21 = mul_simd(a1, row_b1);
	auto mul_22 = mul_simd(a2, row_b2);
	temp = add_simd(mul_21, mul_22);
	auto out2 = add_simd(mul_20, temp);

	a0 = v_and(row_a0, last_mask);
	a1 = v_and(row_a1, last_mask);
	a2 = v_and(row_a2, last_mask);
	out0 = add_simd(out0, a0);
	out1 = add_simd(out1, a1);
	out2 = add_simd(out2, a2);

	mat3x4 res{};
	store_simd(res[0], out0);
	store_simd(res[1], out1);
	store_simd(res[2], out2);
	return res;
}

inline vec3 approach(vec3 target, vec3 value, float speed)
{
	vec3 diff = target - value;
	float delta = diff.length();

	if (delta > speed)
		value += diff.normalize() * speed;
	else if (delta < -speed)
		value -= diff.normalize() * speed;
	else
		value = target;

	return value;
}

template <typename T> __forceinline int signum(T val) { return (T(0) < val) - (val < T(0)); }

__forceinline float get_fov(const angle &current, const vec3 &start, const vec3 &end)
{
	vec3 v;
	angle_vectors(current, v);
	if ((end - start).normalize().dot(v) < .3f)
		return FLT_MAX;

	const auto target = calc_angle(start, end);
	const auto dist = (end - start).length();
	const auto pitch = sin(DEG2RAD(fabsf(current.x - target.x))) * dist;
	const auto yaw = sin(DEG2RAD(fabsf(current.y - target.y))) * dist;
	return vec2(pitch, yaw).length();
}

__forceinline float get_fov_simple(const angle &current, const vec3 &start, const vec3 &end)
{
	vec3 current_aim, target = (end - start).normalize();
	angle_vectors(current, current_aim);
	return RAD2DEG(acos(target.dot(current_aim)));
}

__forceinline void matrix_set_origin(const vec3 &pos, mat3x4 &matrix)
{
	matrix[0][3] = pos.x;
	matrix[1][3] = pos.y;
	matrix[2][3] = pos.z;
}

__forceinline vec3 matrix_get_origin(const mat3x4 &src) { return {src[0][3], src[1][3], src[2][3]}; }

__forceinline void move_matrix(mat3x4 *const mat, vec3 &from, const vec3 &to)
{
	for (auto i = 0; i < max_bones; ++i)
	{
		const auto delta = matrix_get_origin(mat[i]) - from;
		matrix_set_origin(delta + to, mat[i]);
	}

	from = to;
}

__forceinline vec3 rotate_2d(const vec3 &start, const float rotation, const float length_remaining)
{
	return vec3(start.x + std::cos(DEG2RAD(rotation)) * length_remaining,
				start.y + std::sin(DEG2RAD(rotation)) * length_remaining, start.z);
}

template <typename T>
__forceinline void linear_fade(T &current, const T min, const T max, const float frequency, const bool direction)
{
	if (current < max && direction)
		current += frequency * game->globals->frametime;
	else if (current > min && !direction)
		current -= frequency * game->globals->frametime;

	current = std::clamp(current, min, max);
};

__forceinline color hsv_to_rgb(float h, float s, float v)
{
	if (s <= 0.0)
	{
		const auto x = static_cast<uint8_t>(v * 255);
		return {x, x, x};
	}

	if (h >= 360.f)
		h = 0.f;

	h /= 60.f;

	const auto i = static_cast<long>(h);
	const auto f = h - i;
	const auto p = v * (1.0 - s);
	const auto q = v * (1.0 - (s * f));
	const auto t = v * (1.0 - (s * (1.0 - f)));

	switch (i)
	{
	case 0:
		return {static_cast<uint8_t>(v * 255), static_cast<uint8_t>(t * 255), static_cast<uint8_t>(p * 255)};
	case 1:
		return {static_cast<uint8_t>(q * 255), static_cast<uint8_t>(v * 255), static_cast<uint8_t>(p * 255)};
	case 2:
		return {static_cast<uint8_t>(p * 255), static_cast<uint8_t>(v * 255), static_cast<uint8_t>(t * 255)};
	case 3:
		return {static_cast<uint8_t>(p * 255), static_cast<uint8_t>(q * 255), static_cast<uint8_t>(v * 255)};
	case 4:
		return {static_cast<uint8_t>(t * 255), static_cast<uint8_t>(p * 255), static_cast<uint8_t>(v * 255)};
	default:
		return {static_cast<uint8_t>(v * 255), static_cast<uint8_t>(p * 255), static_cast<uint8_t>(q * 255)};
	}
}

__forceinline void rgb_to_hsv(color in, float *h, float *s, float *v)
{
	const auto r = static_cast<float>(in.red() / 255.f);
	const auto g = static_cast<float>(in.green() / 255.f);
	const auto b = static_cast<float>(in.blue() / 255.f);

	const auto m = min(min(r, g), b);
	*v = max(max(r, g), b);
	const auto delta = *v - m;

	if (*v == 0.f)
		*s = 0.f;
	else
		*s = delta / *v;

	if (*s == 0.f)
		*h = 0.f;
	else
	{
		if (r == *v)
			*h = (g - b) / delta;
		else if (g == *v)
			*h = 2.f + (b - r) / delta;
		else if (b == *v)
			*h = 4.f + (r - g) / delta;

		*h *= 60.f;

		if (*h < 0.)
			*h += 360.f;
	}
}

__forceinline float interpolate(const float from, const float to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

__forceinline vec3 interpolate(const vec3 &from, const vec3 &to, const float percent)
{
	return to * percent + from * (1.f - percent);
}

__forceinline color interpolate(const color &from, const color &to, const float percent, const auto sat)
{
	float h1, h2, s1, s2, v1, v2;
	rgb_to_hsv(from, &h1, &s1, &v1);
	rgb_to_hsv(to, &h2, &s2, &v2);
	s1 *= sat;
	s2 *= sat;
	return hsv_to_rgb(interpolate(h1, h2, percent), interpolate(s1, s2, percent), interpolate(v1, v2, percent));
}

__forceinline color saturate(const color &from, const auto saturation)
{
	float h, s, v;
	rgb_to_hsv(from, &h, &s, &v);
	return hsv_to_rgb(h, s * saturation, v);
}

__forceinline float distance_point_to_line(const vec3 &a, const vec3 &b, const vec3 &c)
{
	const auto l = c.length();
	const auto t = (a - b).dot(c) / (l * l);

	if (t <= FLT_EPSILON)
		return FLT_MAX;

	const auto x = b + c * t;
	return (a - x).length();
}

__forceinline void clamp_vector(vec3 &v, const float cl)
{
	const auto length = v.length();
	if (length > .001f)
	{
		v /= length;
		v *= min(length, cl);
	}
}

__forceinline float approach(float target, float value, float speed)
{
	float delta = target - value;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}

__forceinline float smoothstep_bounds(float edge0, float edge1, float x)
{
	x = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
	return x * x * (3 - 2 * x);
}

__forceinline float distance_to_ray(const vec3 &pos, const vec3 &start, const vec3 &end)
{
	const auto to = pos - start;
	auto dir = end - start;
	const auto length = dir.length();
	dir.normalize();
	const auto range_along = dir.dot(to);

	float range;

	if (range_along < 0.0f)
		range = -(pos - start).length();
	else if (range_along > length)
		range = -(pos - end).length();
	else
		range = (pos - (start + (dir * range_along))).length();

	return range;
}

__forceinline float segment_to_segment(const vec3 &s1, const vec3 &s2, const vec3 &k1, const vec3 &k2)
{
	constexpr auto epsilon = .00000001;

	auto u = s2 - s1;
	auto v = k2 - k1;
	const auto w = s1 - k1;

	const auto a = u.dot(u);
	const auto b = u.dot(v);
	const auto c = v.dot(v);
	const auto d = u.dot(w);
	const auto e = v.dot(w);
	const auto D = a * c - b * b;
	float sN, sD = D;
	float tN, tD = D;

	if (D < epsilon)
	{
		sN = 0.0;
		sD = 1.0;
		tN = e;
		tD = c;
	}
	else
	{
		sN = b * e - c * d;
		tN = a * e - b * d;

		if (sN < 0.0)
		{
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD)
		{
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0)
	{
		tN = 0.0;

		if (-d < 0.0)
			sN = 0.0;
		else if (-d > a)
			sN = sD;
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD)
	{
		tN = tD;

		if (-d + b < 0.0)
			sN = 0;
		else if (-d + b > a)
			sN = sD;
		else
		{
			sN = -d + b;
			sD = a;
		}
	}

	const float sc = abs(sN) < epsilon ? 0.0 : sN / sD;
	const float tc = abs(tN) < epsilon ? 0.0 : tN / tD;

	auto dP = w + u * sc - v * tc;
	return sqrt(dP.dot(dP));
}

__forceinline bool intersect_line_with_bb(const ray &r, vec3 &min, vec3 &max)
{
	auto tmin = (min.x - r.start.x) / r.delta.x;
	auto tmax = (max.x - r.start.x) / r.delta.x;

	if (tmin > tmax)
		std::swap(tmin, tmax);

	auto tymin = (min.y - r.start.y) / r.delta.y;
	auto tymax = (max.y - r.start.y) / r.delta.y;

	if (tymin > tymax)
		std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	auto tzmin = (min.z - r.start.z) / r.delta.z;
	auto tzmax = (max.z - r.start.z) / r.delta.z;

	if (tzmin > tzmax)
		std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

__forceinline bool intersect_line_with_capsule(const ray r, const vec3 vmin, const vec3 vmax, const float radius)
{
	return segment_to_segment(vec3(r.start), r.start + r.delta, vmin, vmax) < radius;
}
} // namespace sdk

#endif // SDK_MATH_H
