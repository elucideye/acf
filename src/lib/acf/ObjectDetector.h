/*! -*-c++-*-
  @file   ObjectDetector.h
  @author David Hirvonen
  @brief  Internal ObjectDetector abstract API declaration file.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_ObjectDetector_h__
#define __acf_ObjectDetector_h__

#include <acf/acf_export.h>
#include <acf/acf_common.h>
#include <acf/MatP.h>

#include <opencv2/core/core.hpp>

#include <vector>

ACF_NAMESPACE_BEGIN

// Specify API
class ACF_EXPORT ObjectDetector
{
public:
    // TODO: enforce a public non virtual API that calls a virtual detect method
    // and applies pruning criteria as needed.
    virtual int operator()(const cv::Mat& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = 0) = 0;
    virtual int operator()(const MatP& image, std::vector<cv::Rect>& objects, std::vector<double>* scores = 0) = 0;
    virtual void setMaxDetectionCount(size_t maxCount)
    {
        m_maxDetectionCount = maxCount;
    }
    virtual void setDetectionScorePruneRatio(double ratio)
    {
        m_detectionScorePruneRatio = ratio;
    }
    virtual void prune(std::vector<cv::Rect>& objects, std::vector<double>& scores)
    {
        if (objects.size() > 1)
        {
            int cutoff = 1;
            for (int i = 1; i < std::min(m_maxDetectionCount, objects.size()); i++)
            {
                cutoff = i + 1;
                if (scores[i] < (scores[0] * m_detectionScorePruneRatio))
                {
                    break;
                }
            }
            objects.erase(objects.begin() + cutoff, objects.end());
            scores.erase(scores.begin() + cutoff, scores.end());
        }
    }
    virtual void setDoNonMaximaSuppression(bool flag)
    {
        m_doNms = flag;
    }
    virtual bool getDoNonMaximaSuppression() const
    {
        return m_doNms;
    }

    virtual cv::Size getWindowSize() const = 0;

protected:
    bool m_doNms = false;
    double m_detectionScorePruneRatio = 0.0;
    size_t m_maxDetectionCount = 10;
};

ACF_NAMESPACE_END

#endif
