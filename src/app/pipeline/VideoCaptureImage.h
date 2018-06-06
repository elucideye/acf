/*! -*-c++-*-
  @file   VideoCaptureImage.h
  @author David Hirvonen
  @brief  Present cv::Mat as an cv::VideoCaptureImag

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_VideoCaptureImage_h__
#define __acf_VideoCaptureImage_h__

#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <iosfwd>

class VideoCaptureImage : public cv::VideoCapture
{
public:
    VideoCaptureImage(cv::Mat  image, int frames = 100);
    VideoCaptureImage(const std::string& filename, int frames = 100);
    ~VideoCaptureImage() override;

    void setRepeat(int n);
    bool grab() override;
    bool isOpened() const override;
    void release() override;
    bool open(const cv::String& filename) override;
    bool read(cv::OutputArray image) override;
    double get(int propId) const override;

    cv::Mat image;
    int frames = 0;
    int index = -1;
};

#endif // __acf_VideoCaptureImage_h__
