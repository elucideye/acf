/*! -*-c++-*-
  @file   random.h
  @author David Hirvonen
  @brief  Private header for random utilities.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_random_h__
#define __acf_random_h__

#include <acf/acf_common.h>

#include <functional>
#include <numeric>
#include <random>

ACF_NAMESPACE_BEGIN

inline std::vector<int> create_random_indices(int n)
{
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
    return indices;
}

ACF_NAMESPACE_END

#endif // __acf_random_h__
