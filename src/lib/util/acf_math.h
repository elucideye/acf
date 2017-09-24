/*! -*-c++-*-
  @file   drishti_math.h
  @author David Hirvonen
  @brief  Declaration of common math routines.

  \copyright Copyright 2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_drishti_math_h__
#define __acf_drishti_math_h__ 1

#include "util/acf_util.h"

#include <cmath>

UTIL_NAMESPACE_BEGIN

template <typename T>
T logN(const T& x, const T& n)
{
    return std::log(x) / std::log(n);
}
template <typename T>
T log2(const T& x)
{
    return logN(x, T(2));
}
template <typename T>
T round(T x);

UTIL_NAMESPACE_END

#endif // drishti_math.h
