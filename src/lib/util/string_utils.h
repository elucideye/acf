/*! -*-c++-*-
  @file   string_utils.h
  @author David Hirvonen
  @brief  Declaration of string manipulation routines.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __util_string_utils_h__
#define __util_string_utils_h__

#include <util/acf_util.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

UTIL_NAMESPACE_BEGIN

void tokenize(const std::string& input, std::vector<std::string>& tokens);
std::string basename(const std::string& name, const std::string& ext = ".");
bool replace(std::string& str, const std::string& from, const std::string& to);

UTIL_NAMESPACE_END

#endif /* __util_string_utils_h___ */
