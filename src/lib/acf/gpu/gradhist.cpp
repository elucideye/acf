/*! -*-c++-*-
  @file   gradhist.cpp
  @author David Hirvonen (C++ implementation)
  @brief Implementation of an ogles_gpgpu shader for computing gradient orientation histograms.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/gpu/gradhist.h>

BEGIN_OGLES_GPGPU

// GradProc delivers:
// clamp(mag, 0.0, 1.0), theta/3.14159, clamp(dx, 0.0, 1.0), clamp(dy, 0.0, 1.0))

//
//    [1]
//  [2] [0]
//    [3]
//
// [0][1][2][3]

// clang-format off
const char * GradHistProc::fshaderGradHistSrcN = OG_TO_STR
(
#if defined(OGLES_GPGPU_OPENGLES)
 precision highp float;
#endif
 varying vec2 vTexCoord;
 uniform sampler2D uInputTex;
 uniform float nOrientations;
 uniform ivec4 index;
 uniform float strength;

 void main()
 {
     vec4 val = texture2D(uInputTex, vTexCoord);
     val.y *= (1.0 - step(1.0, val.y));
     float mag = val.x * strength;
     float t = val.y * nOrientations;
     vec2 k = floor(mod(floor(vec2(t, t+1.0)), nOrientations));

     float a = abs(t - k.x);
     float b = abs(1.0 - a);
     vec4 index0 = vec4(equal(ivec4(int(k.x)), index));
     vec4 index1 = vec4(equal(ivec4(int(k.y)), index));
     vec4 result = mag * vec4((index0 * b) + (index1 * a));

     gl_FragColor = result;
 });
// clang-format on
END_OGLES_GPGPU
