/*! -*-c++-*-
  @file   draw.h
  @author David Hirvonen
  @brief  Declaration of drawing routines related to ACF computation.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_draw_h__
#define __acf_draw_h__

#include <acf/ACF.h>

#include <opencv2/core.hpp>

ACF_NAMESPACE_BEGIN

cv::Mat ACF_EXPORT draw(acf::Detector::Pyramid& pyramid);

ACF_NAMESPACE_END

#endif // __acf_draw_h__
