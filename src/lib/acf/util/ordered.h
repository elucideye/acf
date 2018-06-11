/*! -*-c++-*-
  @file   ordered.h
  @author David Hirvonen
  @brief  Return vector index for sorted array.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __util_ordered_h__
#define __util_ordered_h__ 1

#include <util/acf_util.h>

#include <vector>
#include <numeric>
#include <algorithm>

UTIL_NAMESPACE_BEGIN

template <typename T, typename Comp = std::less<T>>
std::vector<size_t> ordered(const std::vector<T>& values, const Comp& C)
{
    std::vector<size_t> indices(values.size());
    std::iota(std::begin(indices), std::end(indices), static_cast<size_t>(0));
    std::sort(std::begin(indices), std::end(indices), [&](size_t a, size_t b) {
        return C(values[a], values[b]);
    });
    return indices;
}

UTIL_NAMESPACE_END

#endif // __util_ordered_ 1
