/*! -*-c++-*-
  @file   GLDetector.h
  @author David Hirvonen
  @brief  Utility class to run ACF object detection with OpenGL ACF pyramid computation.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/ACF.h>

#include <memory>

#ifndef __acf_GLDetector_h__
#define __acf_GLDetector_h__

ACF_NAMESPACE_BEGIN

/*
 * A utility class intended to exercise the OpenGL ES mobile pyramid computation
 * (in a fairly naive way that should be good enough for testing, but might be slow).sy
 */

class GLDetector : public acf::Detector
{
public:
    using RectVec = std::vector<cv::Rect>;
    using DoubleVec = std::vector<double>;
    using DetectorPtr = std::shared_ptr<acf::Detector>;

    GLDetector(const std::string& filename, int maxSize = 2048);
    ~GLDetector();

    // Virtual API:
    virtual int operator()(const cv::Mat& input, RectVec& objects, DoubleVec* scores = 0);

    cv::Mat draw(bool gpu); // debug routine
    void clear();
    
protected:
    void init(const cv::Mat& I);
    void initContext();
    const acf::Detector::Pyramid& getPyramid(const cv::Mat& input, const cv::Mat& rgb = {});

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

ACF_NAMESPACE_END

#endif // __acf_GLDetector_h__
