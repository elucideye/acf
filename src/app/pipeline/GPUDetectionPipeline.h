/*! -*-c++-*-
  @file   GPUDetectionPipeline.h
  @author David Hirvonen
  @brief  Pipeline for efficient GPU feature computation

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_GPUDetectionPipeline_h__
#define __acf_GPUDetectionPipeline_h__

#include <acf/ACF.h>
#include <acf/acf_common.h>
#include <acf/GPUACF.h>

#include <ogles_gpgpu/common/proc/video.h>
#include <ogles_gpgpu/platform/opengl/gl_includes.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>

#include <cstdint>
#include <stddef.h>
#include <chrono>
#include <functional>
#include <memory>
#include <utility>

ACF_NAMESPACE_BEGIN

struct Detections
{
    Detections() = default;
    Detections(std::uint64_t frameIndex)
        : frameIndex(frameIndex)
    {
    }

    std::uint64_t frameIndex{};
    cv::Mat image;
    std::vector<cv::Rect> roi;
    std::shared_ptr<acf::Detector::Pyramid> P;
};

class GPUDetectionPipeline
{
public:
    using HighResolutionClock = std::chrono::high_resolution_clock;
    using TimePoint = HighResolutionClock::time_point; // <std::chrono::system_clock>;
    using DetectionPtr = std::shared_ptr<acf::Detector>;
    using DetectionTex = std::pair<GLuint, Detections>;
    using DetectionCallback = std::function<void(GLuint texture, const Detections& detections)>;
    using FrameInput = ogles_gpgpu::FrameInput;

    GPUDetectionPipeline
    (
        DetectionPtr& detector,
        const cv::Size& inputSize,
        std::size_t n,
        int rotation,
        int minObjectWidth,
        bool useLatency = true,
        bool doCpuACF = false,
        void *glContext = nullptr
    );
    virtual ~GPUDetectionPipeline();

    GLuint getInputTexture();

    // This method receives an input frame descriptor (pixel buffer or texture ID) on which to run
    // ACF object detection.  The doDetection parameter is provided in order to allow the user to
    // control the duty cycle of the detector (perhaps adaptively).  The detection pipeline introduces
    // two frames of latency so that the GPU->CPU overhead can be hidden.  For input frame N, the results
    // are returned for frame N-2 (along with the corresponding texture ID).
    DetectionTex operator()(const ogles_gpgpu::FrameInput& frame, bool doDetection = true);

    void operator+=(const DetectionCallback& callback);

    std::map<std::string, double> summary();

    void setDoGlobalNMS(bool flag);

protected:
    DetectionTex run(const FrameInput& frame2, bool doDetection);
    DetectionTex runSimple(const ogles_gpgpu::FrameInput& frame, bool doDetection = true);
    DetectionTex runFast(const ogles_gpgpu::FrameInput& frame, bool doDetection = true);

    void preprocess(const ogles_gpgpu::FrameInput& frame, Detections& scene, bool doDetection);

    std::shared_ptr<acf::Detector::Pyramid> createAcfGpu(const FrameInput& frame, bool doDetection);
    std::shared_ptr<acf::Detector::Pyramid> createAcfCpu(const FrameInput& frame, bool doDetection);

    // Allow user defined object detection drawing via inheritance.
    virtual GLuint paint(const Detections& scene, GLuint inputTexture);

    int detectOnly(Detections& scene, bool doDetection);
    int detect(const ogles_gpgpu::FrameInput& frame, Detections& scene, bool doDetection);
    void fill(acf::Detector::Pyramid& P);
    void init(const cv::Size& inputSize);
    void initACF(const cv::Size& inputSizeUp);
    void initFIFO(const cv::Size& inputSize, std::size_t n);
    void computeAcf(const ogles_gpgpu::FrameInput& frame, bool doLuv, bool doDetection);
    int computeDetectionWidth(const cv::Size& inputSizeUp) const;

    struct Impl;

    std::unique_ptr<Impl> impl;
};

ACF_NAMESPACE_END

#endif // __acf_GPUDetectionPipeline_h__
