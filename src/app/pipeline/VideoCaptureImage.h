/*! -*-c++-*-
  @file   VideoCaptureImage.h
  @author David Hirvonen
  @brief  Present cv::Mat as an cv::VideoCaptureImag

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_VideoCaptureImage_h__
#define __acf_VideoCaptureImage_h__

#include <opencv2/highgui.hpp>

class VideoCaptureImage : public cv::VideoCapture
{
public:
    VideoCaptureImage(const cv::Mat& image, int frames = 100);
    VideoCaptureImage(const std::string& filename, int frames = 100);
    virtual ~VideoCaptureImage();

    void setRepeat(int n);
    virtual bool grab();
    virtual bool isOpened() const;
    virtual void release();
    virtual bool open(const cv::String& filename);
    virtual bool read(cv::OutputArray image);
    double get(int propId) const;

    cv::Mat image;
    int frames = 0;
    int index = -1;
};

#endif // __acf_VideoCaptureImage_h__
