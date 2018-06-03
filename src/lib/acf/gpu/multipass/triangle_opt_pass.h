/*! -*-c++-*-
  @file   triangle_opt_pass.h
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
 * GPGPU gaussian smoothing processor.
 */
#ifndef __acf_gpu_multipass_triangle_pass_h__
#define __acf_gpu_multipass_triangle_pass_h__

#include <acf/acf_common.h>

#include <ogles_gpgpu/common/common_includes.h>
#include <ogles_gpgpu/common/proc/base/filterprocbase.h>

BEGIN_OGLES_GPGPU

/**
 * This filter applies gaussian smoothing to an input image.
 */
class TriangleOptProcPass : public FilterProcBase
{
public:
    /**
     * Construct as render pass <pass> (1 or 2).
     */
    TriangleOptProcPass(int pass, int radius, bool doNorm = false, float normConst = 0.005f)
        : FilterProcBase()
        , doNorm(doNorm)
        , renderPass(pass)
        , pxDx(0.0f)
        , pxDy(0.0f)
        , normConst(normConst)
    {
        setRadius(radius);
        assert(renderPass == 1 || renderPass == 2);
    }

    void setRadius(int newValue);

    /**
     * Return the processors name.
     */
    const char* getProcName() override
    {
        return "TriangleOptProcPass";
    }

    void filterShaderSetup(const char* vShaderSrc, const char* fShaderSrc, GLenum target) override;
    void setUniforms() override;
    void getUniforms() override;
    const char* getFragmentShaderSource() override;
    const char* getVertexShaderSource() override;

private:
    bool doNorm = false;
    int renderPass; // render pass number. must be 1 or 2

    float pxDx; // pixel delta value for texture access
    float pxDy; // pixel delta value for texture access

    float normConst = 0.005;

    int _blurRadiusInPixels = 0.0; // start 0 (uninitialized)

    GLint shParamUTexelWidthOffset{};
    GLint shParamUTexelHeightOffset{};

    std::string vshaderTriangleSrc;
    std::string fshaderTriangleSrc;
};

END_OGLES_GPGPU
#endif
