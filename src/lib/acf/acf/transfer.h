/*! -*-c++-*-
  @file   transfer.h
  @author David Hirvonen
  @brief  Declaration of gpu->cpu transfer routines.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_transfer_h__
#define __acf_transfer_h__

#include <opencv2/core.hpp>
#include <ogles_gpgpu/common/proc/base/procinterface.h>

BEGIN_OGLES_GPGPU

cv::Mat getImage(ProcInterface& proc);
cv::Mat getImage(ProcInterface& proc, cv::Mat& frame);

END_OGLES_GPGPU

#endif // __acf_transfer_h__

