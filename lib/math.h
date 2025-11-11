/*
 * This file is part of libtw07, a header-only library for teeworlds0.7.
 *
 * Orignal Code by:
 * Copyright (C) 2007-2025 Magnus Auvinen
 *
 * Libtw07 Library:
 * Copyright (C) 2025 TeeworldsArchive
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Note: This is an altered version — a C language port of the original C++ implementation.
 */
#ifndef LIBTW07_MATH_H
#define LIBTW07_MATH_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define libtw07_clamp(val, min, max) ((val < min) ? min : ((val > max) ? max : val))

inline float libtw07_sign(float f)
{
	return f < 0.0f ? -1.0f : 1.0f;
}

inline int libtw07_round_to_int(float f)
{
	if(f > 0.0f)
		return (int)(f + 0.5f);
	return (int)(f - 0.5f);
}

#define libtw07_mix(a, b, amount) (a + (b - a) * amount)
#define libtw07_bezier(p0, p1, p2, p3, amount) libtw07_mix(libtw07_mix(libtw07_mix(p0, p1, amount), libtw07_mix(p1, p2, amount), amount), libtw07_mix(libtw07_mix(p1, p2, amount), libtw07_mix(p2, p3, amount), amount), amount)

inline int libtw07_random_int()
{
	return ((rand() & 0x7fff) << 16) | (rand() & 0xffff);
}

inline float libtw07_random_float()
{
	return rand() / (float)RAND_MAX;
}

const int fxpscale = 1 << 10;

// float to fixed
inline int libtw07_f2fx(float v)
{
	return (int)(v * fxpscale);
}
inline float libtw07_fx2f(int v)
{
	return v / (float)fxpscale;
}

// int to fixed
inline int libtw07_i2fx(int v)
{
	return v * fxpscale;
}
inline int libtw07_fx2i(int v)
{
	return v / fxpscale;
}

inline int libtw07_gcd(int a, int b)
{
	while(b != 0)
	{
		int c = a % b;
		a = b;
		b = c;
	}
	return a;
}

const float pi = 3.1415926535897932384626433f;

#define libtw07_minimum(a, b) (a < b ? a : b)
#define libtw07_maximum(a, b) (a > b ? a : b)

#define libtw07_absolute(a) (a < 0 ? -a : a)

#ifdef __cplusplus
}
#endif

#endif // BASE_MATH_H
