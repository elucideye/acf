/*! -*-c++-*-
  @file   cv_cereal.h
  @author David Hirvonen
  @brief  Serialization of common opencv types.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __common_cv_cereal_h__
#define __common_cv_cereal_h__

#include <common/acf_common.h>

// see: https://github.com/USCiLab/cereal/issues/104

// clang-format off
#ifdef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
#  undef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
#  define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
// clang-format on

#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

#define GENERIC_NVP(name, value) ::cereal::make_nvp<Archive>(name, value)

#include <opencv2/core.hpp>

COMMON_BEGIN_NAMESPACE(cv)

template <class Archive>
void serialize(Archive& ar, cv::Rect& rect, const unsigned int version)
{
    ar& GENERIC_NVP("x", rect.x);
    ar& GENERIC_NVP("y", rect.y);
    ar& GENERIC_NVP("width", rect.width);
    ar& GENERIC_NVP("height", rect.height);
}

COMMON_END_NAMESPACE(cv)

#endif // __common_cv_cereal_h__
