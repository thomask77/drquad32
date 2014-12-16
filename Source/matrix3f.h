/**
 * \file
 * A collection of static inlined floating point 3x3 matrix functions.
 *
 * \author  Copyright (c)2012 Thomas Kindler <mail@t-kindler.de>
 *
 * This program is free software;  you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License,  or (at your option) any later version.  Read the
 * full License at http://www.gnu.org/copyleft for more details.
 */
#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "util.h"

/**
 * TODO: Really nail down the math in here!
 *
 * http://fgiesen.wordpress.com/2012/06/03/linear-algebra-toolbox-1/
 * http://fgiesen.wordpress.com/2012/02/12/row-major-vs-column-major-row-vectors-vs-column-vectors/
 * http://fgiesen.wordpress.com/2013/06/02/modified-gram-schmidt-orthogonalization/
 */

#define VEC2_ZERO       { 0, 0 }
#define VEC2_UNIT_X     { 1, 0 }
#define VEC2_UNIT_Y     { 0, 1 }

#define VEC3_ZERO       { 0, 0, 0 }
#define VEC3_UNIT_X     { 1, 0, 0 }
#define VEC3_UNIT_Y     { 0, 1, 0 }
#define VEC3_UNIT_Z     { 0, 0, 1 }

#define MAT2_ZERO       { 0, 0,   0, 0 }
#define MAT2_IDENTITY   { 1, 0,   0, 1 }

#define MAT3_ZERO       { 0, 0, 0,   0, 0, 0,   0, 0, 0 }
#define MAT3_IDENTITY   { 1, 0, 0,   0, 1, 0,   0, 0, 1 }


typedef struct {
    float  x, y;
} vec2f;

typedef struct {
    float  m00, m01;
    float  m10, m11;
} mat2f;

typedef struct {
    float  x, y, z;
} vec3f;

typedef struct {
    float  m00, m01, m02;
    float  m10, m11, m12;
    float  m20, m21, m22;
} mat3f;


static const vec2f vec2f_zero     = VEC2_ZERO;
static const vec2f vec2f_unit_x   = VEC2_UNIT_X;
static const vec2f vec2f_unit_y   = VEC2_UNIT_Y;

static const mat2f mat2f_zero     = MAT2_ZERO;
static const mat2f mat2f_identity = MAT2_IDENTITY;

static const vec3f vec3f_zero     = VEC3_ZERO;
static const vec3f vec3f_unit_x   = VEC3_UNIT_X;
static const vec3f vec3f_unit_y   = VEC3_UNIT_Y;
static const vec3f vec3f_unit_z   = VEC3_UNIT_Z;

static const mat3f mat3f_zero     = MAT3_ZERO;
static const mat3f mat3f_identity = MAT3_IDENTITY;


static inline vec3f vec3f_add(const vec3f a, const vec3f b)
{
    return (vec3f) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

static inline vec3f vec3f_sub(const vec3f a, const vec3f b)
{
    return (vec3f) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

static inline vec3f vec3f_scale(const vec3f a, const float c)
{
    return (vec3f) {
        a.x * c,
        a.y * c,
        a.z * c
    };
}

static inline vec3f vec3f_offset(const vec3f a, const float c)
{
    return (vec3f) {
        a.x + c,
        a.y + c,
        a.z + c
    };
}

static inline vec3f vec3f_div(const vec3f a, const float c)
{
    return (vec3f) {
        a.x / c,
        a.y / c,
        a.z / c
    };
}

static inline vec3f vec3f_fma(const vec3f a, const vec3f b, const vec3f c)
{
    return (vec3f) {
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z
    };
}

static inline vec3f vec3f_matmul(const mat3f A, const vec3f b)
{
    return (vec3f) {
        A.m00 * b.x + A.m01 * b.y + A.m02 * b.z,
        A.m10 * b.x + A.m11 * b.y + A.m12 * b.z,
        A.m20 * b.x + A.m21 * b.y + A.m22 * b.z
    };
}

static inline float vec3f_dot(const vec3f a, const vec3f b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3f vec3f_cross(const vec3f a, const vec3f b)
{
    return (vec3f) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3f_lensq(const vec3f a)
{
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

static inline float vec3f_len(const vec3f a)
{
    return sqrtf(vec3f_lensq(a));
}

static inline vec3f vec3f_norm(const vec3f a)
{
    return vec3f_div(a, vec3f_len(a));
}

static inline vec3f vec3f_clamp(const vec3f a, const vec3f min, const vec3f max)
{
    return (vec3f) {
        clamp(a.x, min.x, max.x),
        clamp(a.y, min.y, max.y),
        clamp(a.z, min.z, max.z),
    };
}

static inline void vec3f_print(const vec3f v)
{
    printf("%8.3f %8.3f %8.3f\n", v.x, v.y, v.z);
}



static inline mat3f mat3f_trans(const mat3f A)
{
    return (mat3f) {
        A.m00, A.m10, A.m20,
        A.m01, A.m11, A.m21,
        A.m02, A.m12, A.m22
    };
}

static inline mat3f mat3f_mul(const mat3f A, const mat3f B)
{
    return (mat3f) {
        A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20,
        A.m00 * B.m01 + A.m01 * B.m11 + A.m02 * B.m21,
        A.m00 * B.m02 + A.m01 * B.m12 + A.m02 * B.m22,
        A.m10 * B.m00 + A.m11 * B.m10 + A.m12 * B.m20,
        A.m10 * B.m01 + A.m11 * B.m11 + A.m12 * B.m21,
        A.m10 * B.m02 + A.m11 * B.m12 + A.m12 * B.m22,
        A.m20 * B.m00 + A.m21 * B.m10 + A.m22 * B.m20,
        A.m20 * B.m01 + A.m21 * B.m11 + A.m22 * B.m21,
        A.m20 * B.m02 + A.m21 * B.m12 + A.m22 * B.m22
    };
}

static inline mat3f mat3f_add(const mat3f A, const mat3f B)
{
    return (mat3f) {
        A.m00 + B.m00, A.m01 + B.m01, A.m02 + B.m02,
        A.m10 + B.m10, A.m11 + B.m11, A.m12 + B.m12,
        A.m20 + B.m20, A.m21 + B.m21, A.m22 + B.m22
    };
}

static inline float mat3f_det(const mat3f A)
{
    float c00 = A.m11 * A.m22 - A.m12 * A.m21;
    float c10 = A.m12 * A.m20 - A.m10 * A.m22;
    float c20 = A.m10 * A.m21 - A.m11 * A.m20;

    return  A.m00 * c00 + A.m01 * c10 + A.m02 * c20;
}

static inline vec3f mat3f_row(const mat3f A, const int row)
{
    assert(row >= 0 && row <= 3);

    switch (row) {
    case 0: return (vec3f){ A.m00, A.m01, A.m02 };
    case 1: return (vec3f){ A.m10, A.m11, A.m12 };
    case 2: return (vec3f){ A.m20, A.m21, A.m22 };
    }
}

static inline vec3f mat3f_col(const mat3f A, const int col)
{
    assert(col >= 0 && col <= 3);

    switch (col) {
    case 0: return (vec3f){ A.m00, A.m10, A.m20 };
    case 1: return (vec3f){ A.m01, A.m11, A.m21 };
    case 2: return (vec3f){ A.m02, A.m12, A.m22 };
    }
}

static inline vec3f mat3f_to_euler(const mat3f A)
{
    return (vec3f) {
        atan2f(A.m21, A.m22),   // roll
        -asinf(A.m20),          // pitch
        atan2f(A.m10, A.m00)    // yaw
    };
}

static inline void mat3f_print(const mat3f m)
{
    printf("%8.3f %8.3f %8.3f\n", m.m00, m.m01, m.m02);
    printf("%8.3f %8.3f %8.3f\n", m.m10, m.m11, m.m12);
    printf("%8.3f %8.3f %8.3f\n", m.m20, m.m21, m.m22);
}

