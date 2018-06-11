/*! -*-c++-*-
  @file   string_hash.h
  @author Yakk
  @brief  String hashing utility.

  See: http://stackoverflow.com/a/23684632

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This file is released under the Creative Commons License.}

*/

#ifndef __acf_string_hash_h__
#define __acf_string_hash_h__

#include <util/acf_util.h>

#include <string>

UTIL_BEGIN_NAMESPACE(string_hash)

template <class>
struct hasher;
template <>
struct hasher<std::string>
{
    std::size_t constexpr operator()(char const* input) const
    {
        return *input ? static_cast<unsigned int>(*input) + 33 * (*this)(input + 1) : 5381;
    }
    std::size_t operator()(const std::string& str) const
    {
        return (*this)(str.c_str());
    }
};
template <typename T>
std::size_t constexpr hash(T&& t)
{
    return hasher<typename std::decay<T>::type>()(std::forward<T>(t));
}

// clang-format off
inline /* inline namespace */
UTIL_BEGIN_NAMESPACE(literals)
    std::size_t constexpr
    operator"" _hash(const char* s, size_t)
{
    return hasher<std::string>()(s);
}
UTIL_END_NAMESPACE(literals)
// clang-format on

UTIL_END_NAMESPACE(string_hash)

#endif
