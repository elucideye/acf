/*! -*-c++-*-
  @file   triangle_opt_pass.cpp
  @author David Hirvonen (C++ implementation)
  @brief Definition of a 1D optimized ogles_gpgpu triangle filter shader.

  This separable filter makes use of OpengL HW texel interpolation to replace two adjacent 
  integer offset kernel coefficients with a single coefficient for a single (interpolated)
  texel lookup.  The code is adapted from the following:

  https://github.com/BradLarson/GPUImage/blob/master/framework/Source/GPUImageSingleComponentGaussianBlurFilter.m#L5-L186

  Which is released under compatible 3-clause BSD license:

  https://github.com/BradLarson/GPUImage/blob/master/License.txt

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/gpu/multipass/triangle_opt_pass.h>

#include <ogles_gpgpu/common/tools.h>
#include <ogles_gpgpu/platform/opengl/gl_includes.h>

using namespace ogles_gpgpu;

static void getOptimizedTriangle(int blurRadius, std::vector<GLfloat>& weights, std::vector<GLfloat>& offsets)
{
    std::vector<GLfloat> standardTriangleWeights(blurRadius + 1);
    const GLfloat sumOfWeights = ((blurRadius + 1) * (blurRadius + 1));
    const GLfloat norm = 1.0f / sumOfWeights;
    const int maxCoeff = (blurRadius + 1);

    for (int currentTriangleWeightIndex = 0; currentTriangleWeightIndex < (blurRadius + 1); currentTriangleWeightIndex++)
    {
        standardTriangleWeights[currentTriangleWeightIndex] = norm * GLfloat(maxCoeff - currentTriangleWeightIndex);
    }

    // From these weights we calculate the offsets to read interpolated values from
    const int numberOfOptimizedOffsets = std::min(blurRadius / 2 + (blurRadius % 2), 7);

    std::vector<GLfloat> optimizedTriangleOffsets(numberOfOptimizedOffsets);
    for (int currentOptimizedOffset = 0; currentOptimizedOffset < numberOfOptimizedOffsets; currentOptimizedOffset++)
    {
        const int firstIndex = currentOptimizedOffset * 2 + 1;
        const int secondIndex = currentOptimizedOffset * 2 + 2;
        const GLfloat firstWeight = standardTriangleWeights[firstIndex];
        const GLfloat secondWeight = standardTriangleWeights[secondIndex];
        const GLfloat optimizedWeight = firstWeight + secondWeight;
        const GLfloat optimizedOffset = (firstWeight * firstIndex + secondWeight * secondIndex) / optimizedWeight;
        optimizedTriangleOffsets[currentOptimizedOffset] = optimizedOffset;
    }

    weights = standardTriangleWeights;
    offsets = optimizedTriangleOffsets;
}

static std::string fragmentShaderForOptimizedTriangle(int blurRadius, bool doNorm = false, int pass = 1, float normConst = 0.005f)
{
    std::vector<GLfloat> standardTriangleWeights;
    std::vector<GLfloat> optimizedTriangleOffsets;
    getOptimizedTriangle(blurRadius, standardTriangleWeights, optimizedTriangleOffsets);

    // From these weights we calculate the offsets to read interpolated values from
    int numberOfOptimizedOffsets = std::min(blurRadius / 2 + (blurRadius % 2), 7);
    int trueNumberOfOptimizedOffsets = blurRadius / 2 + (blurRadius % 2);

    std::stringstream ss;
#if defined(OGLES_GPGPU_OPENGLES)
    ss << "precision highp float;\n";
    ss << "\n";
#endif
    ss << "uniform sampler2D inputImageTexture;\n";
    ss << "uniform float texelWidthOffset;\n";
    ss << "uniform float texelHeightOffset;\n\n";
    ss << "varying vec2 blurCoordinates[" << (1 + (numberOfOptimizedOffsets * 2)) << "];\n\n";
    ss << "void main()\n";
    ss << "{\n";
    ss << "   vec4 sum = vec4(0.0);\n";
    ss << "   vec4 center = texture2D(inputImageTexture, blurCoordinates[0]);\n";
    ss << "   sum += center * " << standardTriangleWeights[0] << ";\n";

    for (int currentBlurCoordinateIndex = 0; currentBlurCoordinateIndex < numberOfOptimizedOffsets; currentBlurCoordinateIndex++)
    {
        GLfloat firstWeight = standardTriangleWeights[currentBlurCoordinateIndex * 2 + 1];
        GLfloat secondWeight = standardTriangleWeights[currentBlurCoordinateIndex * 2 + 2];
        GLfloat optimizedWeight = firstWeight + secondWeight;
        int index1 = static_cast<unsigned long>((currentBlurCoordinateIndex * 2) + 1);
        int index2 = static_cast<unsigned long>((currentBlurCoordinateIndex * 2) + 2);
        ss << "   sum += texture2D(inputImageTexture, blurCoordinates[" << index1 << "]) * " << optimizedWeight << ";\n";
        ss << "   sum += texture2D(inputImageTexture, blurCoordinates[" << index2 << "]) * " << optimizedWeight << ";\n";
    }

    // If the number of required samples exceeds the amount we can pass in via varyings, we have to do dependent texture reads in the fragment shader
    if (trueNumberOfOptimizedOffsets > numberOfOptimizedOffsets)
    {
        ss << "   vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);\n";
        for (int currentOverlowTextureRead = numberOfOptimizedOffsets; currentOverlowTextureRead < trueNumberOfOptimizedOffsets; currentOverlowTextureRead++)
        {
            GLfloat firstWeight = standardTriangleWeights[currentOverlowTextureRead * 2 + 1];
            GLfloat secondWeight = standardTriangleWeights[currentOverlowTextureRead * 2 + 2];

            GLfloat optimizedWeight = firstWeight + secondWeight;
            GLfloat optimizedOffset = (firstWeight * (currentOverlowTextureRead * 2 + 1) + secondWeight * (currentOverlowTextureRead * 2 + 2)) / optimizedWeight;

            ss << "   sum += texture2D(inputImageTexture, blurCoordinates[0] + singleStepOffset * " << optimizedOffset << ") * " << optimizedWeight << ";\n";
            ss << "   sum += texture2D(inputImageTexture, blurCoordinates[0] - singleStepOffset * " << optimizedOffset << ") * " << optimizedWeight << ";\n";
        }
    }

    if (doNorm)
    {
        if (pass == 1)
        {
            ss << "   gl_FragColor = vec4(center.rgb, sum.r);\n";
        }
        else
        {
            ss << "   gl_FragColor = vec4( center.r/(sum.a + " << std::fixed << normConst << "), center.gb, 1.0);\n";
        }
    }
    else
    {
        ss << "   gl_FragColor = sum;\n";
    }

    ss << "}\n";

    return ss.str();
}

std::string vertexShaderForOptimizedTriangle(int blurRadius)
{
    std::vector<GLfloat> standardTriangleWeights;
    std::vector<GLfloat> optimizedTriangleOffsets;
    getOptimizedTriangle(blurRadius, standardTriangleWeights, optimizedTriangleOffsets);

    int numberOfOptimizedOffsets = optimizedTriangleOffsets.size();

    std::stringstream ss;
    ss << "attribute vec4 position;\n";
    ss << "attribute vec4 inputTextureCoordinate;\n";
    ss << "uniform float texelWidthOffset;\n";
    ss << "uniform float texelHeightOffset;\n\n";
    ss << "varying vec2 blurCoordinates[" << static_cast<unsigned long>(1 + (numberOfOptimizedOffsets * 2)) << "];\n\n";
    ss << "void main()\n";
    ss << "{\n";
    ss << "   gl_Position = position;\n";
    ss << "   vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);\n";
    ss << "   blurCoordinates[0] = inputTextureCoordinate.xy;\n";
    for (int currentOptimizedOffset = 0; currentOptimizedOffset < numberOfOptimizedOffsets; currentOptimizedOffset++)
    {
        int x1 = static_cast<unsigned long>((currentOptimizedOffset * 2) + 1);
        int x2 = static_cast<unsigned long>((currentOptimizedOffset * 2) + 2);
        const auto& optOffset = optimizedTriangleOffsets[currentOptimizedOffset];

        ss << "   blurCoordinates[" << x1 << "] = inputTextureCoordinate.xy + singleStepOffset * " << optOffset << ";\n";
        ss << "   blurCoordinates[" << x2 << "] = inputTextureCoordinate.xy - singleStepOffset * " << optOffset << ";\n";
    }
    ss << "}\n";

    return ss.str();
}

// Calculate the number of pixels to sample from by setting a bottom limit for the contribution of the outermost pixel
// The normalized outermost pixel should not be below:
//
//    float minimumWeightToFindEdgeOfSamplingArea = 1.0 / 256.0;
//
// We know the unnormalized edge coefficient will always be 1.0.
// The sum of coefficients will always be: (blurRadius + 1) * (blurRadius + 1)
// So we need to make sure: ((blurRadius + 1) * (blurRadius + 1)) < 256.0
// Solve:
//    x*x + 2x + 2 < 256
//    x < sqrt(255) - 1
//    x < 14.9
//
// The largest kernel radius is 14.9, or 14

void TriangleOptProcPass::setRadius(int newValue)
{
    if (newValue != _blurRadiusInPixels)
    {
        const int blurRadius = newValue + (newValue % 2); // enforce even blur (as with GPUImage)
        _blurRadiusInPixels = std::min(blurRadius, 14);
        vshaderTriangleSrc = vertexShaderForOptimizedTriangle(_blurRadiusInPixels);
        fshaderTriangleSrc = fragmentShaderForOptimizedTriangle(_blurRadiusInPixels, doNorm, renderPass, normConst);
    }
}

void TriangleOptProcPass::filterShaderSetup(const char* vShaderSrc, const char* fShaderSrc, GLenum target)
{
    // create shader object
    ProcBase::createShader(vShaderSrc, fShaderSrc, target);

    // get shader params
    shParamAPos = shader->getParam(ATTR, "position");
    shParamATexCoord = shader->getParam(ATTR, "inputTextureCoordinate");
    Tools::checkGLErr(getProcName(), "filterShaderSetup");
}

void TriangleOptProcPass::setUniforms()
{
    FilterProcBase::setUniforms();

    glUniform1f(shParamUTexelWidthOffset, (renderPass == 1) * pxDx);
    glUniform1f(shParamUTexelHeightOffset, (renderPass == 2) * pxDy);
}

void TriangleOptProcPass::getUniforms()
{
    FilterProcBase::getUniforms();

    // calculate pixel delta values
    pxDx = 1.0f / static_cast<float>(outFrameW);
    pxDy = 1.0f / static_cast<float>(outFrameH);

    shParamUInputTex = shader->getParam(UNIF, "inputImageTexture");
    shParamUTexelWidthOffset = shader->getParam(UNIF, "texelWidthOffset");
    shParamUTexelHeightOffset = shader->getParam(UNIF, "texelHeightOffset");
}

const char* TriangleOptProcPass::getFragmentShaderSource()
{
    return fshaderTriangleSrc.c_str();
}

const char* TriangleOptProcPass::getVertexShaderSource()
{
    return vshaderTriangleSrc.c_str();
}
