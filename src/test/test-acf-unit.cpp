/*! -*-c++-*-
  @file   test-acf-unit.cpp
  @author David Hirvonen
  @brief  Google test for the GPU ACF code.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>

#include <opencv2/core.hpp>

const char* imageFilename;

int gauze_main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    CV_Assert(argc >= 2);
    imageFilename = argv[1];
    
    return RUN_ALL_TESTS();
}

#include <acf/convert.h> // private
#include <acf/transfer.h>

#include <opencv2/core/base.hpp>
#include <opencv2/core/cvstd.inl.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/imgcodecs.hpp>

#include <assert.h>

#if defined(ACF_DO_GPU)

#include <aglet/GLContext.h>

#include <ogles_gpgpu/common/proc/video.h>
#include <ogles_gpgpu/common/types.h>
#include <ogles_gpgpu/platform/opengl/gl_includes.h>

#include <acf/gpu/triangle_opt.h> // private

// clang-format off
#if defined(ANDROID) || defined(OGLES_GPGPU_NIX)
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

// clang-format off
#if defined(ACF_DO_GPU)
#  include <acf/GPUACF.h>
#  include <aglet/GLContext.h>
static int gWidth = 640;
static int gHeight = 480;
#  if defined(ACF_OPENGL_ES2)
static const auto gVersion = aglet::GLContext::kGLES20;
#  elif defined(ACF_OPENGL_ES3)
static const auto gVersion = aglet::GLContext::kGLES30;
#  else
static const auto gVersion = aglet::GLContext::kGL;
#  endif // defined(OGLES_GPGPU_OPENGL_ES3)
#endif // defined(ACF_DO_GPU)
// clang-format on

TEST(ACFShaderTest, TriangleOptProcPass)
{
    auto context = aglet::GLContext::create(aglet::GLContext::kAuto, {}, gWidth, gHeight, gVersion);
    (*context)();
    ASSERT_TRUE(context && (*context));
    ASSERT_EQ(glGetError(), GL_NO_ERROR);
    if (context && *context)
    {
        cv::Mat test = cv::imread(imageFilename, cv::IMREAD_COLOR);
        CV_Assert(test.channels() == 3);
        cv::cvtColor(test, test, cv::COLOR_BGR2BGRA); // add alpha

        glActiveTexture(GL_TEXTURE0);
        ogles_gpgpu::VideoSource video;
        ogles_gpgpu::TriangleOptProc triangle(4);

        video.set(&triangle);
        video({ test.cols, test.rows }, test.ptr<void>(), true, 0, DFLT_TEXTURE_FORMAT);

        cv::Mat result_gpu;
        ogles_gpgpu::getImage(triangle, result_gpu);
        ASSERT_FALSE(result_gpu.empty());

        std::vector<float> kernel{ 1.f, 2.f, 3.f, 4.f, 5.f, 4.f, 3.f, 2.f, 1.f };
        auto total = static_cast<float>((4 + 1) * (4 + 1));
        for (auto& k : kernel)
        {
            k /= total;
        }
        cv::Mat result_cpu;
        cv::sepFilter2D(test, result_cpu, CV_8UC1, kernel, kernel);

        cv::cvtColor(result_gpu, result_gpu, cv::COLOR_BGRA2BGR); // drop alpha
        cv::cvtColor(result_cpu, result_cpu, cv::COLOR_BGRA2BGR); // drop alpha
        cv::Mat result_cpu_1d = result_cpu.reshape(1, 1);
        cv::Mat result_gpu_1d = result_gpu.reshape(1, 1);

        double RMSE = cv::norm(result_cpu_1d, result_gpu_1d, cv::NORM_L2);
        RMSE /= std::sqrt(static_cast<double>(result_cpu_1d.total()));

        ASSERT_LE(RMSE, 2.0); // we can tolerate some difference for speed
    }
}
#endif // defined(ACF_DO_GPU)

static const int rgba[] = { 2, 1, 0, 3 };

static std::vector<cv::Mat> unpack_test(const cv::Size& size)
{
    cv::Mat4b src(size, cv::Vec4b(0, 1, 2, 3));

    std::vector<cv::Mat> dst{
        cv::Mat1b(src.size()),
        cv::Mat1b(src.size()),
        cv::Mat1b(src.size()),
        cv::Mat1b(src.size())
    };

    std::vector<acf::PlaneInfo> table{ { dst[0], rgba[0] }, { dst[1], rgba[1] }, { dst[2], rgba[2] }, { dst[3], rgba[3] } };

    acf::unpack(src, table);

    return dst;
}

TEST(ChannelConversion, unpack_mul_16)
{
    auto dst = unpack_test({ 100, 160 });
    for (int i = 0; i < 4; i++)
    {
        int count = cv::countNonZero(dst[i] == rgba[i]);
        ASSERT_EQ(count, dst[i].total());
    }
}

TEST(ChannelConversion, unpack_rem_16)
{
    auto dst = unpack_test({ 100, 161 });
    for (int i = 0; i < 4; i++)
    {
        int count = cv::countNonZero(dst[i] == rgba[i]);
        ASSERT_EQ(count, dst[i].total());
    }
}

static std::vector<cv::Mat> convert_test(const cv::Size& size)
{
    cv::Mat4b src(size, cv::Vec4b(0, 1, 2, 3));

    std::vector<cv::Mat> dst{
        cv::Mat1f(src.size()),
        cv::Mat1f(src.size()),
        cv::Mat1f(src.size()),
        cv::Mat1f(src.size())
    };

    std::vector<acf::PlaneInfo> table{ { dst[0], rgba[0] }, { dst[1], rgba[1] }, { dst[2], rgba[2] }, { dst[3], rgba[3] } };

    acf::convertU8ToF32(src, table);

    return dst;
}

TEST(ChannelConversion, convert_mul_16)
{
    auto dst = convert_test({ 100, 160 });

    for (int i = 0; i < 4; i++)
    {
        int count = cv::countNonZero(dst[i] == float(rgba[i]));
        ASSERT_EQ(count, dst[i].total());
    }
}

TEST(ChannelConversion, convert_rem_16)
{
    auto dst = convert_test({ 100, 161 });

    for (int i = 0; i < 4; i++)
    {
        int count = cv::countNonZero(dst[i] == float(rgba[i]));
        ASSERT_EQ(count, dst[i].total());
    }
}
