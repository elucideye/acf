/*! -*-c++-*-
  @file   convert.h
  @author David Hirvonen
  @brief  Declaration of optimized unpack and format conversion routines.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_convert_h__
#define __acf_convert_h__ 1

#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <util/acf_util.h>

UTIL_NAMESPACE_BEGIN

struct PlaneInfo
{
    PlaneInfo(cv::Mat& plane, int channel = 0, float alpha = 1.f)
        : plane(plane)
        , channel(channel)
        , alpha(alpha)
    {
    }
    cv::Mat plane;
    int channel = 0;
    float alpha = 1.f;
};

void convertU8ToF32(const cv::Mat4b& input, std::vector<PlaneInfo>& planes);

void unpack(const cv::Mat4b& input, std::vector<PlaneInfo>& planes);

UTIL_NAMESPACE_END

#endif // __acf_convert_h__
