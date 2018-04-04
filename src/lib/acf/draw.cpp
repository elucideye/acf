/*! -*-c++-*-
  @file   draw.cpp
  @author David Hirvonen
  @brief  Implementation of drawing routines related to ACF computation.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/draw.h>

#include <opencv2/imgproc.hpp>

ACF_NAMESPACE_BEGIN

// This function demonstrates how to visualize a pyramid structure:
cv::Mat draw(acf::Detector::Pyramid& pyramid)
{
   cv::Mat canvas;
   std::vector<cv::Mat> levels;
   for (int i = 0; i < pyramid.nScales; i++)
   {
       // Concatenate the transposed faces, so they are compatible with the GPU layout
       cv::Mat Ccpu;
       std::vector<cv::Mat> images;
       for (const auto& image : pyramid.data[i][0].get())
       {
           images.push_back(image.t());
       }
       cv::vconcat(images, Ccpu);

       // Instead of upright:
       //cv::vconcat(pyramid.data[i][0].get(), Ccpu);

       if (levels.size())
       {
           cv::copyMakeBorder(Ccpu, Ccpu, 0, levels.front().rows - Ccpu.rows, 0, 0, cv::BORDER_CONSTANT);
       }

       levels.push_back(Ccpu);
   }
   cv::hconcat(levels, canvas);
   return canvas;
}

ACF_NAMESPACE_END
