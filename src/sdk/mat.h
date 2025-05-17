
#ifndef SDK_MAT_H
#define SDK_MAT_H

#include <sdk/intrinsics.h>

namespace sdk
{
struct vrect_t
{
	int x, y, width, height;
	vrect_t *pnext;
};

struct mat3x4
{
	float mat[3][4];

	__forceinline float *operator[](const int i) { return mat[i]; }

	__forceinline const float *operator[](const int i) const { return mat[i]; }
};

struct viewmat
{
	float matrix[4][4];

	__forceinline float *operator[](const int i) { return matrix[i]; }

	__forceinline const float *operator[](const int i) const { return matrix[i]; }
};
} // namespace sdk

#endif // SDK_MAT_H
