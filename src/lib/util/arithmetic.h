/*! -*-c++-*-
  @file   arithmetic.h
  @author David Hirvonen
  @brief  Declaration of optimized vector arithmetic.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __util_arithmetic_h__
#define __util_arithmetic_h__

#include "util/acf_util.h"

#include <cstdint>

UTIL_NAMESPACE_BEGIN

template <typename T>
T round(T x);

void add16sAnd16s(const int16_t* pa, const int16_t* pb, int16_t* pc, int n);
void add16sAnd32s(const int32_t* pa, const int16_t* pb, int32_t* pc, int n);
void add32f(const float* pa, const float* pb, float* pc, int n);
void convertFixedPoint(const float* pa, int16_t* pb, int n, int fraction);

UTIL_NAMESPACE_END

#endif
