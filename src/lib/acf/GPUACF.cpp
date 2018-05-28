/*! -*-c++-*-
  @file   GPUACF.cpp
  @author David Hirvonen
  @brief  Declaration of OpenGL shader optimized Aggregated Channel Feature computation.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <acf/GPUACF.h>

#include <ogles_gpgpu/common/proc/grayscale.h>
#include <ogles_gpgpu/common/proc/pyramid.h>
#include <ogles_gpgpu/common/proc/grad.h>
#include <ogles_gpgpu/common/proc/gauss.h>
#include <ogles_gpgpu/common/proc/gauss_opt.h>
#include <ogles_gpgpu/common/proc/transform.h>
#include <ogles_gpgpu/common/proc/gain.h>
#include <ogles_gpgpu/common/proc/tensor.h>
#include <ogles_gpgpu/common/proc/nms.h>

// generic shaders
#include <ogles_gpgpu/common/proc/gain.h>
#include <ogles_gpgpu/common/proc/swizzle.h>
#include <ogles_gpgpu/common/proc/rgb2luv.h>

// acf specific shader
#include <acf/gpu/swizzle2.h>
#include <acf/gpu/gradhist.h>
#include <acf/gpu/triangle_opt.h>

#include <util/convert.h>
#include <util/timing.h>
#include <util/Parallel.h>
#include <util/make_unique.h>
#include <ogles_gpgpu/common/gl/memtransfer_optimized.h>

#include <iostream>
#include <chrono>
#include <array>

// clang-format off
#ifdef ANDROID
#  define TEXTURE_FORMAT GL_RGBA
#  define TEXTURE_FORMAT_IS_RGBA 1
#else
#  define TEXTURE_FORMAT GL_BGRA
#  define TEXTURE_FORMAT_IS_RGBA 0
#endif
// clang-format on

BEGIN_OGLES_GPGPU

struct ACF::Impl
{
    using PlaneInfoVec = std::vector<util::PlaneInfo>;
    using ChannelSpecification = std::vector<std::pair<PlaneInfoVec, ProcInterface*>>;
    using SmoothProc = ogles_gpgpu::GaussOptProc;

    Impl(void* glContext, const Size2d& size, const SizeVec& scales, FeatureKind kind, int grayWidth, int shrink)
        : m_featureKind(kind)
        , m_size(size)
        , m_doGray(grayWidth > 0)
        , m_grayscaleScale(float(grayWidth) / float(size.width))
        , m_shrink(shrink)
    {
        initACF(scales, kind);

        if (m_doGray)
        {
            Size2d graySize(grayWidth, int(m_grayscaleScale * size.height + 0.5f));
            reduceForGrayProc = util::make_unique<ogles_gpgpu::GainProc>();
            reduceForGrayProc->setOutputSize(graySize.width, graySize.height);
            rotationProc->add(reduceForGrayProc.get()); // ### OUTPUT ###
        }

        if (m_doLuvTransfer)
        {
            // Add transposed Luv output for CPU processing (optional)
            luvTransposeOut->setOutputRenderOrientation(RenderOrientationDiagonal);
            rgb2luvProc->add(luvTransposeOut.get());
        }
    }

    void initACF(const SizeVec& scales, FeatureKind kind)
    {
        // Rotation + rescale and ACF pipeline:
        rotationProc = util::make_unique<ogles_gpgpu::GainProc>();
        rgb2luvProc = util::make_unique<ogles_gpgpu::Rgb2LuvProc>();
        pyramidProc = util::make_unique<ogles_gpgpu::PyramidProc>(scales);
        gradProc = util::make_unique<ogles_gpgpu::GradProc>(1.0f);
        normProc = util::make_unique<ogles_gpgpu::TriangleOptProc>(5, true, 0.005f);
        gradHistProcA = util::make_unique<ogles_gpgpu::GradHistProc>(6, 0, 1.0f);
        gradHistProcB = util::make_unique<ogles_gpgpu::GradHistProc>(6, 4, 1.0f);

        // Strategic smoothing:
        smoothProc = util::make_unique<SmoothProc>(1.0f);
        smoothNormGradProc = util::make_unique<SmoothProc>(2.0f);
        smoothGradHistProcA = util::make_unique<SmoothProc>(2.0f);
        smoothGradHistProcB = util::make_unique<SmoothProc>(2.0f);

        // Reduction:
        reduceRgbProc = util::make_unique<ogles_gpgpu::GainProc>();
        reduceLuvProc = util::make_unique<ogles_gpgpu::GainProc>();
        reduceNormGradProc = util::make_unique<ogles_gpgpu::GainProc>(1.2f);
        reduceGradHistProcA = util::make_unique<ogles_gpgpu::GainProc>(1.2f);
        reduceGradHistProcB = util::make_unique<ogles_gpgpu::GainProc>(1.2f);

        // Reduce base image to highest resolution used in pyramid:
        reduceRgbProc->setOutputSize(scales[0].width, scales[0].height);

        const float scale = (1.f / static_cast<float>(m_shrink));
        reduceNormGradProc->setOutputSize(scale);
        reduceLuvProc->setOutputSize(scale);
        reduceGradHistProcA->setOutputSize(scale);
        reduceGradHistProcB->setOutputSize(scale);

#if GPU_ACF_TRANSPOSE
        reduceNormGradProc->setOutputRenderOrientation(RenderOrientationDiagonal);
        reduceLuvProc->setOutputRenderOrientation(RenderOrientationDiagonal);
        reduceGradHistProcA->setOutputRenderOrientation(RenderOrientationDiagonal);
        reduceGradHistProcB->setOutputRenderOrientation(RenderOrientationDiagonal);
#endif
        pyramidProc->setInterpolation(ogles_gpgpu::TransformProc::BICUBIC);

        rotationProc->add(smoothProc.get());
        smoothProc->add(reduceRgbProc.get());
        reduceRgbProc->add(rgb2luvProc.get());

        // ((( luv -> pyramid(luv) )))
        rgb2luvProc->add(pyramidProc.get());

        // ((( pyramid(luv) -> {luv_out, MOXY} )))
        pyramidProc->add(reduceLuvProc.get());
        pyramidProc->add(gradProc.get());

        // ((( MOXY -> norm(M) ))
        gradProc->add(normProc.get()); // norm(M)OX.

        // ((( norm(M) -> {histA, histB} )))
        normProc->add(smoothNormGradProc.get());
        smoothNormGradProc->add(reduceNormGradProc.get());
        normProc->add(gradHistProcA.get());
        normProc->add(gradHistProcB.get());

        // ((( histA -> smooth(histA) )))
        gradHistProcA->add(smoothGradHistProcA.get());
        smoothGradHistProcA->add(reduceGradHistProcA.get());

        // ((( histB -> smooth(histB) )))
        gradHistProcB->add(smoothGradHistProcB.get());
        smoothGradHistProcB->add(reduceGradHistProcB.get());

        switch (kind)
        {
            case kM012345:

                // This uses two swizzle steps to creaet LG56 output
                // Adding a 3 input texture swizzler might be slightly more efficient.

                // ((( MERGE(luv, grad) )))
                mergeProcLUVG = util::make_unique<MergeProc>(MergeProc::kSwizzleABC1);
                reduceLuvProc->add(mergeProcLUVG.get(), 0);
                reduceNormGradProc->add(mergeProcLUVG.get(), 1);

                // ((( MERGE(lg, 56) )))
                mergeProcLG56 = util::make_unique<MergeProc>(MergeProc::kSwizzleAD12);
                mergeProcLUVG->add(mergeProcLG56.get(), 0);
                reduceGradHistProcB->add(mergeProcLG56.get(), 1);
                break;

            case kLUVM012345:
                // ((( MERGE(luv, grad) )))
                mergeProcLUVG = util::make_unique<MergeProc>(MergeProc::kSwizzleABC1);
                reduceLuvProc->add(mergeProcLUVG.get(), 0);
                reduceNormGradProc->add(mergeProcLUVG.get(), 1);
                break;
            default:
                CV_Assert(false);
        }
    }

    // This provides a map for unpacking/swizzling OpenGL textures (i.e., RGBA or BGRA) to user
    // memory using NEON optimized instructions.
    ChannelSpecification getACFChannelSpecification(MatP& acf) const
    {
        // clang-format off
        const auto& rgba = m_rgba;
        switch (m_featureKind)
        {
            case kLUVM012345:
                // 10 : { LUVMp; H0123p; H4567p } requires 3 textures
                return ChannelSpecification
                {
                    {
                        {
                            { acf[0], rgba[0] },
                            { acf[1], rgba[1] },
                            { acf[2], rgba[2] },
                            { acf[3], rgba[3] }
                        },
                        mergeProcLUVG.get()
                    },
                    {
                        {
                            { acf[4], rgba[0] },
                            { acf[5], rgba[1] },
                            { acf[6], rgba[2] },
                            { acf[7], rgba[3] }
                        },
                        reduceGradHistProcA.get()
                    },
                    {
                        {
                            { acf[8], rgba[0] },
                            { acf[9], rgba[1] }
                        },
                        reduceGradHistProcB.get()
                    }
                };

            case kM012345:
                // 7: { Mp; H0123p; H4567p } requires only 2 textures
                return ChannelSpecification
                {
                    {
                        {
                            { acf[0], rgba[1] },
                            { acf[5], rgba[2] },
                            { acf[6], rgba[3] }
                        },
                        mergeProcLG56.get()
                    },
                    {
                        {
                            { acf[1], rgba[0] },
                            { acf[2], rgba[1] },
                            { acf[3], rgba[2] },
                            { acf[4], rgba[3] }
                        },
                        reduceGradHistProcA.get()
                    }
                };
            default:
                CV_Assert(false);
        }
        return ChannelSpecification();
        // clang-format on
    }

    bool needsTextures() const
    {
        bool status = false;
        status |= m_doAcfTransfer && !m_hasChannelOutput;
        status |= m_doGray && !m_hasGrayscaleOutput;
        status |= m_doLuvTransfer && !m_hasLuvOutput;
        return status;
    }

    // ::: MEMBER VARIABLES :::
    FeatureKind m_featureKind = kLUVM012345;

    std::array<int, 4> m_rgba = { { 0, 1, 2, 3 } };
    Size2d m_size;

    // Retriev input image:
    bool m_doAcfTransfer = true;

    cv::Mat m_luv;
    MatP m_luvPlanar;
    bool m_doLuvTransfer = false;
    bool m_hasLuvOutput = false;

    // Grayscale stuff:
    bool m_doGray = false;
    float m_grayscaleScale = 1.0f;
    bool m_hasGrayscaleOutput = false;
    cv::Mat m_grayscale;

    int m_shrink = 4;

    std::unique_ptr<ogles_gpgpu::GainProc> rotationProc; // make sure we have an unmodified upright image
    std::unique_ptr<ogles_gpgpu::Rgb2LuvProc> rgb2luvProc;
    std::unique_ptr<ogles_gpgpu::PyramidProc> pyramidProc;
    std::unique_ptr<ogles_gpgpu::GradProc> gradProc;          // (1.0);
    std::unique_ptr<ogles_gpgpu::TriangleOptProc> normProc;   // (5, true, 0.005);
    std::unique_ptr<ogles_gpgpu::GradHistProc> gradHistProcA; // (6, 0, 1.f);
    std::unique_ptr<ogles_gpgpu::GradHistProc> gradHistProcB; // (6, 4, 1.f);

    // Reduction:
    std::unique_ptr<ogles_gpgpu::GainProc> reduceRgbProc; // initial reduction
    std::unique_ptr<ogles_gpgpu::GainProc> reduceLuvProc; // LUV rescale:
    std::unique_ptr<ogles_gpgpu::GainProc> reduceNormGradProc;
    std::unique_ptr<ogles_gpgpu::GainProc> reduceGradHistProcA; // (1);
    std::unique_ptr<ogles_gpgpu::GainProc> reduceGradHistProcB; // (1);
    std::unique_ptr<ogles_gpgpu::GainProc> reduceForGrayProc;   // (optional) reduce for grayscale output

    // Strategic smoothing (hand tuned to match ACF output)
    std::unique_ptr<SmoothProc> smoothProc;
    std::unique_ptr<SmoothProc> smoothNormGradProc;
    std::unique_ptr<SmoothProc> smoothGradHistProcA;
    std::unique_ptr<SmoothProc> smoothGradHistProcB;

    // #### OUTPUT ###
    std::unique_ptr<ogles_gpgpu::GainProc> luvTransposeOut; //  transposed LUV output

    // Multi-texture swizzle (one or the other for 7 vs 10 channels)
    std::unique_ptr<ogles_gpgpu::MergeProc> mergeProcLUVG;
    std::unique_ptr<ogles_gpgpu::MergeProc> mergeProcLG56;

    uint64_t frameIndex = 0;
    std::vector<Rect2d> m_crops;

    // Channel stuff:
    cv::Mat m_channels;
    bool m_hasChannelOutput = false;
    bool m_usePBO = false;

    std::shared_ptr<spdlog::logger> m_logger;
};

// ::::::::::: PUBLIC ::::::::::::::
void ACF::updateGL()
{
#if defined(_WIN32) || defined(_WIN64)
    // A windows DLL must call glewInit() from within the library (not the application layer)
    CV_Assert(wglGetCurrentContext() != NULL && GLEW_OK == glewInit());
#endif
}

// { 1280 x 960 } x 0.25 => 320x240
ACF::ACF(void* glContext, const Size2d& size, const SizeVec& scales, FeatureKind kind, int grayWidth, int shrink)
    : VideoSource(glContext)
{
    impl = util::make_unique<Impl>(glContext, size, scales, kind, grayWidth, shrink);

    // ((( video -> smooth(luv) )))
    set(impl->rotationProc.get());
}

ACF::~ACF() = default;

void ACF::tryEnablePlatformOptimizations()
{
    Core::tryEnablePlatformOptimizations();
}

void ACF::setLogger(std::shared_ptr<spdlog::logger>& logger)
{
    impl->m_logger = logger;
}

bool ACF::getChannelStatus()
{
    return impl->m_hasChannelOutput;
}

void ACF::setDoLuvTransfer(bool flag)
{
    impl->m_doLuvTransfer = flag;
}

void ACF::setDoAcfTrasfer(bool flag)
{
    impl->m_doAcfTransfer = flag;
}

void ACF::setUsePBO(bool flag)
{
    impl->m_usePBO = flag;
}

bool ACF::getUsePBO() const
{
    return impl->m_usePBO;
}

// ACF base resolution to Grayscale image
float ACF::getGrayscaleScale() const
{
    return impl->m_grayscaleScale;
}

const std::array<int, 4>& ACF::getChannelOrder()
{
    return impl->m_rgba;
}

ProcInterface* ACF::first()
{
    return impl->rotationProc.get();
}

ProcInterface* ACF::getRgb()
{
    return dynamic_cast<ProcInterface*>(impl->rotationProc.get());
}

void ACF::connect(std::shared_ptr<spdlog::logger>& logger)
{
    impl->m_logger = logger;
}

void ACF::setRotation(int degrees)
{
    first()->setOutputRenderOrientation(ogles_gpgpu::degreesToOrientation(degrees));
}

const cv::Mat& ACF::getGrayscale()
{
    assert(impl->m_doGray);
    return impl->m_grayscale;
}

/*
 * Texture swizzling will be used to ensure BGRA format on texture read.
 */

void ACF::initLuvTransposeOutput()
{
    // Add transposed Luv output for CPU processing (optional)
    impl->luvTransposeOut = util::make_unique<ogles_gpgpu::GainProc>();
    impl->luvTransposeOut->setOutputRenderOrientation(RenderOrientationDiagonal);
    impl->rgb2luvProc->add(impl->luvTransposeOut.get());
}

// Implement virtual API to toggle detection + tracking:
void ACF::operator()(const FrameInput& frame)
{
    // Initial pipeline filters:
    // this -> rotationProc -> rgb2luvProc -> pyramidProc
    bool needsPyramid = (impl->m_doAcfTransfer);
    bool needsLuv = (needsPyramid | impl->m_doLuvTransfer);

    // Initial LUV transpose operation (upright image):
    if (impl->m_doLuvTransfer & !impl->luvTransposeOut.get())
    {
        initLuvTransposeOutput();
    }

    if (impl->rgb2luvProc.get())
    {
        impl->rgb2luvProc->setActive(needsLuv);
    }

    if (impl->pyramidProc.get())
    {
        impl->pyramidProc->setActive(needsPyramid);
    }

    // pyramidProc is the highest level unique ACF processing filter:
    if (impl->pyramidProc.get())
    {
        impl->pyramidProc->setActive(impl->m_doAcfTransfer);
    }

    impl->frameIndex++;

    VideoSource::operator()(frame); // call main method

    // Start the transfer asynchronously here:
    if (impl->m_usePBO)
    {
        beginTransfer();
    }
}

// Use this to queue asynchronous transfers w/ PBO (OpenGL ES 3.0)
void ACF::beginTransfer()
{
    if (impl->m_doAcfTransfer)
    {
        switch (impl->m_featureKind)
        {
            case kLUVM012345:
            {
                impl->mergeProcLUVG->getResultData(nullptr);
                impl->reduceGradHistProcA->getResultData(nullptr);
                impl->reduceGradHistProcB->getResultData(nullptr);
            }
            break;

            case kM012345:
            {
                impl->mergeProcLG56->getResultData(nullptr);
                impl->reduceGradHistProcA->getResultData(nullptr);
            }
            break;

            case kUnknown:
            {
                throw std::runtime_error("ACF::beginTransfer() unknown feature type");
            }
        }
    }

    if (impl->m_doGray)
    {
        impl->reduceForGrayProc->getResultData(nullptr);
    }

    if (impl->m_doLuvTransfer)
    {
        impl->luvTransposeOut->getResultData(nullptr);
    }
}

void ACF::preConfig()
{
    impl->m_hasLuvOutput = false;
    impl->m_hasChannelOutput = false;
    impl->m_hasGrayscaleOutput = false;
}

void ACF::postConfig()
{
    // Obtain the scaled image rois:
    const int& s = impl->m_shrink;
    impl->m_crops.clear();
    const auto& rois = impl->pyramidProc->getLevelCrops();
    for (auto& r : rois)
    {
        // TODO: check rounding error (add clipping)?
        impl->m_crops.emplace_back(r.x / s, r.y / s, r.width / s, r.height / s);
    }
}

cv::Mat ACF::getImage(ProcInterface& proc, cv::Mat& frame)
{
    if (dynamic_cast<MemTransferOptimized*>(proc.getMemTransferObj()))
    {
        MemTransfer::FrameDelegate delegate = [&](const Size2d& size, const void* pixels, size_t bytesPerRow) {
            frame = cv::Mat(size.height, size.width, CV_8UC4, (void*)pixels, bytesPerRow).clone();
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

cv::Mat ACF::getImage(ProcInterface& proc)
{
    cv::Mat frame;
    return getImage(proc, frame);
}

bool ACF::processImage(ProcInterface& proc, MemTransfer::FrameDelegate& delegate)
{
    bool status = false;
    MemTransfer* pTransfer = proc.getMemTransferObj();
    if (dynamic_cast<MemTransferOptimized*>(pTransfer))
    {
        proc.getResultData(delegate);
        status = true;
    }
    return status;
}

int ACF::getChannelCount() const
{
    switch (impl->m_featureKind)
    {
        case kM012345:
            return 7;
        case kLUVM012345:
            return 10;
        default:
            return 0;
    }
}

std::vector<std::vector<Rect2d>> ACF::getCropRegions() const
{
    // CReate array of channel rois for each pyramid level
    size_t levelCount = impl->m_crops.size();
    std::vector<std::vector<Rect2d>> crops(levelCount);
    for (size_t i = 0; i < levelCount; i++)
    {
        crops[i] = getChannelCropRegions(int(i));
    }
    return crops;
}

// Copy the parameters from a reference pyramid
void ACF::fill(acf::Detector::Pyramid& Pout, const acf::Detector::Pyramid& Pin)
{
    Pout.pPyramid = Pin.pPyramid;
    Pout.nTypes = Pin.nTypes;
    Pout.nScales = Pin.nScales;
    Pout.info = Pin.info;
    Pout.lambdas = Pin.lambdas;
    Pout.scales = Pin.scales;
    Pout.scaleshw = Pin.scaleshw;

    auto crops = getCropRegions();
    assert(crops.size() > 1);

    Pout.rois.resize(crops.size());
    for (int i = 0; i < crops.size(); i++)
    {
        Pout.rois[i].resize(crops[i].size());
        for (int j = 0; j < crops[i].size(); j++)
        {
            const auto& r = crops[i][j];
            Pout.rois[i][j] = cv::Rect(r.x, r.y, r.width, r.height);
        }
    }
    fill(Pout);
}

// Channels crops are vertically concatenated in the master image:
std::vector<Rect2d> ACF::getChannelCropRegions(int level) const
{
    assert(level < impl->m_crops.size());

#if GPU_ACF_TRANSPOSE
    int step = impl->m_crops[0].height;
    Rect2d roi = impl->m_crops[level];
    std::swap(roi.x, roi.y);
    std::swap(roi.width, roi.height);
    std::vector<Rect2d> crops(getChannelCount(), roi);
    for (int i = 1; i < getChannelCount(); i++)
    {
        crops[i].x += (step * i);
    }
#else
    std::vector<Rect2d> crops(getChannelCount(), impl->m_crops[level]);
    for (int i = 1; i < getChannelCount(); i++)
    {
        crops[i].y += (impl->m_crops[0].height * i);
    }
#endif
    return crops;
}

void ACF::prepare()
{
}

using array_type = acf::Detector::Pyramid::array_type;

// Fill
void ACF::fill(acf::Detector::Pyramid& pyramid)
{
    auto acf = getChannels();

    // Build ACF input:
    const auto regions = getCropRegions();
    const int levelCount = static_cast<int>(regions.size());
    const int channelCount = static_cast<int>(regions.front().size());

    pyramid.nScales = int(levelCount);

    // Create multiresolution representation:
    auto& data = pyramid.data;
    data.resize(levelCount);

    for (int i = 0; i < levelCount; i++)
    {
        data[i].resize(1);
        auto& channels = data[i][0];
        channels.base() = acf;
        channels.resize(int(channelCount));
        for (int j = 0; j < channelCount; j++)
        {
            const auto& roi = regions[i][j];
            channels[j] = acf({ roi.x, roi.y, roi.width, roi.height });
        }
    }
}

static void unpackImage(const cv::Mat4b& frame, std::vector<util::PlaneInfo>& dst)
{
    switch (dst.front().plane.type())
    {
        case CV_8UC4:
            dst.front().plane = frame.clone(); // deep copy
            break;
        case CV_8UC1:
            util::unpack(frame, dst);
            break;
        case CV_32FC1:
            util::convertU8ToF32(frame, dst);
            break;
        default:
            break;
    }
}

static void unpackImage(ProcInterface& proc, std::vector<util::PlaneInfo>& dst)
{
    MemTransfer::FrameDelegate handler = [&](const Size2d& size, const void* pixels, size_t rowStride) {
        cv::Mat4b frame(size.height, size.width, (cv::Vec4b*)pixels, rowStride);
        switch (dst.front().plane.type())
        {
            case CV_8UC4:
                dst.front().plane = frame.clone(); // deep copy
                break;
            case CV_8UC1:
                util::unpack(frame, dst);
                break;
            case CV_32FC1:
                util::convertU8ToF32(frame, dst);
                break;
            default:
                break;
        }
    };
    proc.getResultData(handler);
}

// NOTE: GPUACF::getLuvPlanar(), provides a direct/optimized alternative to
// the following CV_8UC4 access and conversion:

// const auto &LUVA = impl->m_acf->getLuv(); // BGRA
// cv::Mat LUV, LUVf;
// cv::cvtColor(LUVA, LUV, cv::COLOR_BGRA2RGB);
// LUV.convertTo(LUVf, CV_32FC3, 1.0/255.0);
// MatP LUVp(LUVf.t());

const MatP& ACF::getLuvPlanar()
{
    CV_Assert(impl->m_hasLuvOutput);
    return impl->m_luvPlanar;
}

const cv::Mat& ACF::getLuv()
{
    impl->m_luv = getImage(*impl->rgb2luvProc);
    return impl->m_luv;
}

cv::Mat ACF::getChannels()
{
    // This needs to be done after full pipeline execution, but before
    // the channels are retrieved.
    impl->m_rgba = initChannelOrder();

    cv::Mat result = getChannelsImpl();

    return result;
}

void ACF::release()
{
    impl->m_grayscale.release();
    impl->m_channels.release();
}

std::array<int, 4> ACF::initChannelOrder()
{
    // Default GL_BGRA so we can use GL_RGBA for comparisons
    // since GL_BGRA is undefined on Android
    std::array<int, 4> rgba = { { 2, 1, 0, 3 } };
    if (pipeline->getMemTransferObj()->getOutputPixelFormat() == GL_RGBA) // assume BGRA
    {
        std::swap(rgba[0], rgba[2]);
    }

    return rgba;
}

cv::Mat ACF::getChannelsImpl()
{
    using util::unpack;

    // clang-format off
    std::stringstream ss;

    if (impl->needsTextures())
    {
        { // Create a scope for glFlush() timing
            if (auto pTransfer = dynamic_cast<MemTransferOptimized*>(impl->rgb2luvProc->getMemTransferObj()))
            {
                pTransfer->flush();
            }
            else
            {
                glFlush();
            }
        }

        prepare();

        if (m_timer)
        {
            m_timer("read begin");
        }

        Impl::ChannelSpecification planeIndex;
        const auto& rgba = impl->m_rgba; // alias
        
        cv::Mat flow;
        MatP acf, gray, luv;

        if (impl->m_doAcfTransfer)
        {
            const auto acfSize = impl->reduceGradHistProcA->getOutFrameSize();
            acf.create({ acfSize.width, acfSize.height }, CV_8UC1, getChannelCount(), GPU_ACF_TRANSPOSE);
            planeIndex = impl->getACFChannelSpecification(acf);
        }

        if (impl->m_doGray)
        {
            // Here we use the green channel:
            const auto graySize = impl->reduceForGrayProc->getOutFrameSize();
            gray.create({ graySize.width, graySize.height }, CV_8UC1, 1);
            Impl::PlaneInfoVec grayInfo{ { gray[0], rgba[0] } };
            planeIndex.emplace_back(grayInfo, impl->reduceForGrayProc.get());
        }

        if (impl->m_doLuvTransfer)
        {
            const float alpha = 1.0f / 255.0f;
            const auto luvSize = impl->luvTransposeOut->getOutFrameSize();
            luv.create({ luvSize.width, luvSize.height }, CV_32FC1, 3);
            Impl::PlaneInfoVec luvInfo{ { luv[0], rgba[0], alpha }, { luv[1], rgba[1], alpha }, { luv[2], rgba[2], alpha } };
            planeIndex.emplace_back(luvInfo, impl->luvTransposeOut.get());
        }
        
        // We can use either the direct MemTransferOptimized access, or glReadPixels()
        if (dynamic_cast<MemTransferOptimized*>(impl->rgb2luvProc->getMemTransferObj()))
        {
            // TODO: confirm in documentation that ios texture caches can be queried in parallel
            // Experimentally this seems to be the case.
            // clang-format off
            util::ParallelHomogeneousLambda harness = [&](int i)
            {
                planeIndex[i].second->getMemTransferObj()->setOutputPixelFormat(TEXTURE_FORMAT);
                unpackImage(*planeIndex[i].second, planeIndex[i].first);
            }; // clang-format on

#if OGLES_GPGPU_IOS
            // iOS texture cache can be queried in parallel:
            cv::parallel_for_({ 0, int(planeIndex.size()) }, harness);
#else
            harness({ 0, int(planeIndex.size()) });
#endif
        }
        else
        {
            // clang-format off
            util::ParallelHomogeneousLambda harness = [&](int i)
            {
                planeIndex[i].second->getMemTransferObj()->setOutputPixelFormat(TEXTURE_FORMAT);
                unpackImage(getImage(*planeIndex[i].second), planeIndex[i].first);
            }; // clang-format on
            harness({ 0, int(planeIndex.size()) });
        }

        if (impl->m_doAcfTransfer)
        {
            impl->m_channels = acf.base();
            impl->m_hasChannelOutput = true;
        }

        if (impl->m_doGray)
        {
            impl->m_grayscale = gray[0];
            impl->m_hasGrayscaleOutput = true;
        }

        if (impl->m_doLuvTransfer)
        {
            impl->m_luvPlanar = luv;
            impl->m_hasLuvOutput = true;
        }

        if (m_timer)
        {
            m_timer("read end");
        }
    }

    return impl->m_channels;
}

ACF::FeatureKind getFeatureKind(const acf::Detector::Options::Pyramid::Chns& chns)
{
    const auto& pColor = chns.pColor.get();
    const auto& pGradMag = chns.pGradMag.get();
    const auto& pGradHist = chns.pGradHist.get();

    // Currently all supported GPU ACF channel types have trailing M012345
    if (!(pGradMag.enabled && pGradHist.enabled && (pGradHist.nOrients == 6)))
    {
        return ACF::kUnknown;
    }

    if (pColor.enabled)
    {
        if (pColor.colorSpace.get() == "luv")
        {
            return ACF::kLUVM012345;
        }
        else
        {
            return ACF::kUnknown;
        }
    }
    else
    {
        return ACF::kM012345;
    }

    return ACF::kUnknown; // compiler warning
}

END_OGLES_GPGPU
