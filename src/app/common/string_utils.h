/*! -*-c++-*-
  @file   string_utils.h
  @author David Hirvonen
  @brief  Declaration of string manipulation routines.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __common_string_utils_h__
#define __common_string_utils_h__

#include <common/acf_common.h>

#include <string>

COMMON_NAMESPACE_BEGIN

inline std::string basename(const std::string& name, const std::string& ext=".")
{
    size_t pos = name.rfind('/');

    if (pos != std::string::npos)
    {
        pos += 1;
    }
    else
    {
        pos = 0;
    }

    std::string base = name.substr(pos);
    return base.substr(0, std::min(base.size(), base.rfind(ext)));
};

COMMON_NAMESPACE_END

#endif /* __common_string_utils_h___ */
