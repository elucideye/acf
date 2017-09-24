/*! -*-c++-*-
  @file   acf_common.h
  @author David Hirvonen
  @brief  Internal ACF namespace.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_acf_common_h__
#define __acf_acf_common_h__

// clang-format off
#define ACF_BEGIN_NAMESPACE(XYZ) namespace XYZ {
#define ACF_END_NAMESPACE(XYZ) }
// clang-format on

// clang-format off
#define ACF_NAMESPACE_BEGIN namespace acf { 
#define ACF_NAMESPACE_END }
// clang-format on

// clang-format off
#define BEGIN_OGLES_GPGPU namespace ogles_gpgpu {
#define END_OGLES_GPGPU }
// clang-format on

#endif
