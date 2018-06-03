/*! -*-c++-*-
  @file   lines.cpp
  @author David Hirvonen
  @brief Implementation of ogles_gpgpu shader for drawing lines.

  \copyright Copyright 2017-2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include "lines.h"

BEGIN_OGLES_GPGPU

// clang-format off
const char * LineShader::vshaderColorSrc = OG_TO_STR
(
 attribute vec4 position;
 uniform mat4 modelViewProjMatrix;
 
 void main()
 {
     gl_Position = modelViewProjMatrix * position;
 });
// clang-format on

// clang-format off
const char * LineShader::fshaderColorSrc = 
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision highp float;)
#endif
OG_TO_STR(
 uniform vec3 lineColor;
 void main()
 {
     gl_FragColor = vec4(lineColor, 1.0);
 });
// clang-format on

LineShader::LineShader()
{
    // Compile utility line shader:
    shader = std::make_shared<Shader>();
    if (!shader->buildFromSrc(vshaderColorSrc, fshaderColorSrc))
    {
        throw std::runtime_error("LineShader: shader error");
    }
    shParamUColor = shader->getParam(UNIF, "lineColor");
    shParamUMVP = shader->getParam(UNIF, "modelViewProjMatrix");
    shParamAPosition = shader->getParam(ATTR, "position");
}

const char* LineShader::getProcName()
{
    return "LineShader";
}

void LineShader::setLineSegments(const std::vector<Point2d>& segments)
{
    points = segments;
}

void LineShader::setModlViewTransformation(const Mat44f& mvp)
{
    MVP = mvp;
}

void LineShader::draw(int outFrameW, int outFrameH)
{
    glLineWidth(8);

    if (points.size())
    {
        shader->use();
        glUniform3f(shParamUColor, color[0], color[1], color[2]);
        glUniformMatrix4fv(shParamUMVP, 1, 0, &MVP.data[0][0]);
        glViewport(0, 0, outFrameW, outFrameH);
        glVertexAttribPointer(shParamAPosition, 2, GL_FLOAT, 0, 0, &points.data()[0]);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(points.size()));

        Tools::checkGLErr(getProcName(), "draw()");
    }
};

// : : : : : : : : : : : : : : : : : :

// clang-format off
const char * LineProc::fshaderLineSrc =
#if defined(OGLES_GPGPU_OPENGLES)
OG_TO_STR(precision mediump float;)
#endif
OG_TO_STR(
varying vec2 vTexCoord;
uniform sampler2D uInputTex;
void main()
{
    gl_FragColor = texture2D(uInputTex, vTexCoord);
});


static void imageToTexture(ogles_gpgpu::Mat44f &MVP, int width, int height)
{
    for(auto & y : MVP.data)
    {
        for(int x = 0; x < 4; x++)
        {
            y[x] = 0.f;
        }
    }
    MVP.data[0][0] = 2.f / static_cast<float>(width);
    MVP.data[1][1] = 2.f / static_cast<float>(height);
    MVP.data[2][2] = 1.f;
    MVP.data[3][3] = 1.f;
    
    MVP.data[3][0] = -1.f; // apply x translation transposed
    MVP.data[3][1] = -1.f; // apply y translation transposed
}

void LineProc::filterRenderDraw()
{
    ogles_gpgpu::Mat44f MVP{};
    imageToTexture(MVP, getOutFrameW(), getOutFrameH());
  
    ogles_gpgpu::FilterProcBase::filterRenderDraw();
    lines.setModlViewTransformation(MVP);
    lines.draw(getOutFrameW(), getOutFrameH());
}

void LineProc::setLineSegments(const std::vector<Point2d> &points)
{
    lines.setLineSegments(points);
}

void LineProc::setModlViewTransformation(const Mat44f &mvp)
{
    lines.setModlViewTransformation(mvp);
}

END_OGLES_GPGPU
