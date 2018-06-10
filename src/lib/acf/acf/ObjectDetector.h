/*! -*-c++-*-
  @file   ObjectDetector.h
  @author David Hirvonen
  @brief  Internal ObjectDetector abstract API declaration file.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_ObjectDetector_h__
#define __acf_ObjectDetector_h__

#include <acf/MatP.h>
#include <acf/acf_common.h>
#include <acf/acf_export.h>

#include <opencv2/core/core.hpp>
#include <opencv2/core/types.hpp>

#include <stddef.h>
#include <vector>

namespace cv {
class Mat;
}  // namespace cv

ACF_NAMESPACE_BEGIN

// Specify API
class ACF_EXPORT ObjectDetector
{
public:
    virtual ~ObjectDetector();
    virtual int operator()(const cv::Mat& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = nullptr) = 0;
    virtual int operator()(const MatP& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = nullptr) = 0;
    virtual void setDoNonMaximaSuppression(bool flag);
    virtual bool getDoNonMaximaSuppression() const;
    virtual void setMaxDetectionCount(size_t maxCount);
    virtual void setDetectionScorePruneRatio(double ratio);
    virtual void prune(std::vector<cv::Rect>& objects, std::vector<double>& scores);
    virtual cv::Size getWindowSize() const = 0;

protected:
    bool m_doNms = false;
    double m_detectionScorePruneRatio = 0.0;
    size_t m_maxDetectionCount = 10;
};

ACF_NAMESPACE_END

#endif
