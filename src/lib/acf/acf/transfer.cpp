/*! -*-c++-*-
  @file   transfer.cpp
  @author David Hirvonen
  @brief  Implementation of gpu->cpu transfer routines.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/transfer.h>
#include <ogles_gpgpu/common/gl/memtransfer_optimized.h>
#include <ogles_gpgpu/common/proc/base/procinterface.h>

BEGIN_OGLES_GPGPU

cv::Mat getImage(ProcInterface& proc, cv::Mat& frame)
{
    if (dynamic_cast<MemTransferOptimized*>(proc.getMemTransferObj()))
    {
        MemTransfer::FrameDelegate delegate = [&](const Size2d& size, const void* pixels, size_t bytesPerRow) {
            frame = cv::Mat(size.height, size.width, CV_8UC4, const_cast<void*>(pixels), bytesPerRow).clone();
        };
        proc.getResultData(delegate);
    }
    else
    {
        frame.create(proc.getOutFrameH(), proc.getOutFrameW(), CV_8UC4); // noop if preallocated
        proc.getResultData(frame.ptr());
    }
    return frame;
}

cv::Mat getImage(ProcInterface& proc)
{
    cv::Mat frame;
    return getImage(proc, frame);
}

END_OGLES_GPGPU

