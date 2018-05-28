/*! -*-c++-*-
  @file   triangle_opt.h
  @author David Hirvonen (C++ implementation)
  @brief Declaration of an optimized 2D separable ogles_gpgpu triangle filter shader.

  This separable filter makes use of OpengL HW texel interpolation to replace two adjacent 
  integer offset kernel coefficients with a single coefficient for a single (interpolated)
  texel lookup.  The code is adapted from:

  https://github.com/BradLarson/GPUImage/blob/master/framework/Source/GPUImageSingleComponentGaussianBlurFilter.m#L5-L186

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

/**
 * GPGPU gaussian smoothing processor (two-pass).
 */
#ifndef __acf_gpu_triangle_opt_h__
#define __acf_gpu_triangle_opt_h__

#include <acf/acf_common.h>
#include <acf/gpu/multipass/triangle_opt_pass.h>

#include <ogles_gpgpu/common/common_includes.h>
#include <ogles_gpgpu/common/proc/base/multipassproc.h>

BEGIN_OGLES_GPGPU

class TriangleOptProc : public MultiPassProc
{
public:
    TriangleOptProc(int radius, bool doNorm = false, float normConst = 0.005f);
    /**
     * Return the processors name.
     */
    virtual const char* getProcName()
    {
        return "TriangleOptProc";
    }
};

END_OGLES_GPGPU

#endif // __acf_gpu_triangle_opt_h__
