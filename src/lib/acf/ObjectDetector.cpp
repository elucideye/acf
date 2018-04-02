/*! -*-c++-*-
  @file   ObjectDetector.cpp
  @author David Hirvonen
  @brief  Internal ObjectDetector abstract API declaration file.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/ObjectDetector.h>

ACF_NAMESPACE_BEGIN

ObjectDetector::~ObjectDetector() = default;

void ObjectDetector::setMaxDetectionCount(size_t maxCount)
{
    m_maxDetectionCount = maxCount;
}

void ObjectDetector::setDetectionScorePruneRatio(double ratio)
{
    m_detectionScorePruneRatio = ratio;
}

void ObjectDetector::prune(std::vector<cv::Rect>& objects, std::vector<double>& scores)
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

void ObjectDetector::setDoNonMaximaSuppression(bool flag)
{
    m_doNms = flag;
}

bool ObjectDetector::getDoNonMaximaSuppression() const
{
    return m_doNms;
}

ACF_NAMESPACE_END
