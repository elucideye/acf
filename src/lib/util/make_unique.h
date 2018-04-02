/*! -*-c++-*-
  @file   make_unique.h
  @author David Hirvonen
  @brief  Simple macro to add make_unique functionality currently lacking in C++11

  http://clean-cpp.org/underprivileged-unique-pointers-make_unique/#more-54

*/

#ifndef __acf_make_unique_h__
#define __acf_make_unique_h__ 1

#include <util/acf_util.h>

#include <memory>

UTIL_NAMESPACE_BEGIN

template <typename Value, typename... Arguments>
std::unique_ptr<Value> make_unique(Arguments&&... arguments_for_constructor)
{
    return std::unique_ptr<Value>(new Value(std::forward<Arguments>(arguments_for_constructor)...));
}

UTIL_NAMESPACE_END

#endif // __acf_make_unique_h__
