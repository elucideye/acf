/*! -*-c++-*-
  @file   GPUACF.h
  @author David Hirvonen
  @brief  Implementation of OpenGL shader optimized Aggregated Channel Feature computation.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_GPUACF_h__
#define __acf_GPUACF_h__

#include <acf/ACF.h>
#include <acf/acf_common.h>
#include <acf/acf_export.h>
#include <ogles_gpgpu/common/gl/memtransfer.h>
#include <ogles_gpgpu/common/common_includes.h>
// ogles_gpgpu shader lib:
#include <ogles_gpgpu/common/proc/video.h>
#include <opencv2/core/mat.hpp>
#include <array>
#include <functional>
#include <memory>

class MatP;
namespace ogles_gpgpu {
class ProcInterface;
struct Size2d;
}  // namespace ogles_gpgpu
// clang-format off
namespace spdlog {
class logger;
}  // namespace spdlog
// clang-format on

BEGIN_OGLES_GPGPU

// #### GPU #####

#define GPU_ACF_TRANSPOSE 1

struct ACF_EXPORT ACF : public ogles_gpgpu::VideoSource
{
public:
    enum FeatureKind
    {
        kM012345,    // 7
        kLUVM012345, // 10
        kUnknown
    };

    using array_type = acf::Detector::Pyramid::array_type;
    using SizeVec = std::vector<Size2d>;

    ACF(void* glContext, const Size2d& size, const SizeVec& scales, FeatureKind kind, int grayWidth = 0, int shrink = 4);
    ~ACF() override;

    // Platform specify OpenGL bindings (e.g., windows DLL + glewInit())
    static void updateGL();
    static void tryEnablePlatformOptimizations();

    void setUsePBO(bool flag);
    bool getUsePBO() const;

    void setLogger(std::shared_ptr<spdlog::logger>& logger);
    bool getChannelStatus();
    bool getFlowStatus();
    void setDoLuvTransfer(bool flag);
    void setDoAcfTransfer(bool flag);

    // ACF base resolution to Grayscale image
    float getGrayscaleScale() const;

    const std::array<int, 4>& getChannelOrder();

    void preConfig() override;
    void postConfig() override;

    virtual void operator()(const FrameInput& frame);

    void release();
    void connect(std::shared_ptr<spdlog::logger>& logger);
    void setRotation(int degrees);

    static bool processImage(ProcInterface& proc, MemTransfer::FrameDelegate& delegate);
    int getChannelCount() const; // LUVM0123456
    std::vector<std::vector<Rect2d>> getCropRegions() const;
    std::vector<Rect2d> getChannelCropRegions(int level = 0) const;
    void prepare();
    void fill(acf::Detector::Pyramid& pyramid);
    void fill(acf::Detector::Pyramid& Pout, const acf::Detector::Pyramid& Pin);

    // GPU => CPU for ACF:
    cv::Mat getChannels();

    // GPU => CPU for grayscale:
    const cv::Mat& getGrayscale();

    ProcInterface* first();
    ProcInterface* getRgb();

    // Retrieve Luv image as planar 3 channel CV_32F
    const MatP& getLuvPlanar();

    // Retrieve Luv image in packed CV_8UC4 (RGBA) format
    const cv::Mat& getLuv();

    void beginTransfer();

protected:
    cv::Mat getChannelsImpl();

    std::array<int, 4> initChannelOrder();
    void initACF(const SizeVec& scales, FeatureKind kind, bool debug);
    void initLuvTransposeOutput();

    struct Impl;

    std::unique_ptr<Impl> impl;
};

ACF_EXPORT ACF::FeatureKind getFeatureKind(const acf::Detector::Options::Pyramid::Chns& chns);

END_OGLES_GPGPU

#endif // __acf_GPUACF_h__
