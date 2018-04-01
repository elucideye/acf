#include <acf/GLDetector.h>
#include <acf/GPUACF.h>
#include <util/make_unique.h>

#include <aglet/GLContext.h>

#include <opencv2/highgui.hpp>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

ACF_NAMESPACE_BEGIN

static void* void_ptr(const unsigned char* ptr);
static std::vector<ogles_gpgpu::Size2d> getPyramidSizes(acf::Detector::Pyramid& Pcpu, int shrink);
static cv::Mat cvtAnyTo8UC4(const cv::Mat& input);

static cv::Mat draw(const acf::Detector::Pyramid& pyramid);
static void logPyramid(const std::string& filename, const acf::Detector::Pyramid& P);

struct GLDetector::Impl
{
    ogles_gpgpu::ACF::FeatureKind featureKind;
    GLint maxTextureSize;
    cv::Size size;
    acf::Detector::Pyramid Pcpu;
    acf::Detector::Pyramid Pgpu;
    std::shared_ptr<ogles_gpgpu::ACF> acf;
    std::shared_ptr<aglet::GLContext> context;
};

GLDetector::GLDetector(const std::string& filename, int maxSize)
    : acf::Detector(filename)
{
    m_impl = util::make_unique<Impl>();

    // Grab the feature kind to make sure it is supported:
    m_impl->featureKind = ogles_gpgpu::getFeatureKind(opts.pPyramid->pChns.get());
    CV_Assert(m_impl->featureKind != ogles_gpgpu::ACF::FeatureKind::kUnknown);

    // Iniialize the OpenGL context:
    m_impl->context = aglet::GLContext::create(aglet::GLContext::kAuto);
    CV_Assert(m_impl->context && (*m_impl->context));

    // Safeguard for FBO allocation failures here:
    m_impl->maxTextureSize = maxSize;
}

GLDetector::~GLDetector() = default;

void GLDetector::init(const cv::Mat& I)
{
    // Compute a reference pyramid on the CPU to access dimensions:
    m_impl->Pcpu.clear();
    m_impl->Pgpu.clear();

    // As part of the initialization we compute a pyramid at the requested
    // resolution using the CPU pipeline in order to get the desired pyramid
    // level dimensions for the GPU shaders.  The pyramid itself won't typically
    // be used.  If we want the pyramid to match the GPU pyramid then we would
    // have to make sure to pass in RGB format to the CPU functions for the
    // output pyramid to be correct.
    computePyramid(I, m_impl->Pcpu);
    const int shrink = opts.pPyramid->pChns->shrink.get();
    const auto sizes = getPyramidSizes(m_impl->Pcpu, shrink);
    const ogles_gpgpu::Size2d inputSize(I.cols, I.rows);
    m_impl->acf = std::make_shared<ogles_gpgpu::ACF>(nullptr, inputSize, sizes, m_impl->featureKind, false, false, shrink);
    m_impl->acf->setDoLuvTransfer(false);
    m_impl->acf->setRotation(0);
}

const acf::Detector::Pyramid& GLDetector::getPyramid(const cv::Mat& input, const cv::Mat& rgb)
{
    if (input.size() != m_impl->size)
    {
        init(rgb.empty() ? input : rgb);
        m_impl->size = input.size(); // remember the size
    }

    (*m_impl->context)();

    // Fill in the pyramid:
    (*m_impl->acf)({ input.cols, input.rows }, void_ptr(input.ptr()), true, 0, DFLT_TEXTURE_FORMAT);
    glFlush();
    m_impl->acf->fill(m_impl->Pgpu, m_impl->Pcpu);

    return m_impl->Pgpu;
}

// Virtual API:

// input = CV_8UC3 RGB format image (not BGR)
int GLDetector::operator()(const cv::Mat& rgb, RectVec& objects, DoubleVec* scores)
{
    if (std::max(rgb.rows, rgb.cols) < m_impl->maxTextureSize)
    {
        cv::Mat image8uc4 = cvtAnyTo8UC4(rgb);
        if (!image8uc4.empty())
        {
            return acf::Detector::operator()(getPyramid(image8uc4, rgb), objects, scores);
        }
    }

    // Fallback CPU implementation:
    return acf::Detector::operator()(rgb, objects, scores);
}

// utility functions:

static void* void_ptr(const unsigned char* ptr)
{
    return static_cast<void*>(const_cast<unsigned char*>(ptr));
}

std::vector<ogles_gpgpu::Size2d> getPyramidSizes(acf::Detector::Pyramid& Pcpu, int shrink)
{
    std::vector<ogles_gpgpu::Size2d> sizes;
    for (int i = 0; i < Pcpu.nScales; i++)
    {
        const auto size = Pcpu.data[i][0][0].size();
        sizes.emplace_back(size.height * shrink, size.width * shrink); // transpose
    }
    return sizes;
}

static cv::Mat cvtAnyTo8UC4(const cv::Mat& input)
{
    cv::Mat output;
    switch (input.channels())
    {
#if DFLT_TEXTURE_FORMAT == GL_BGRA
        case 1:
            cv::cvtColor(input, output, cv::COLOR_GRAY2BGRA);
            break;
        case 3:
            cv::cvtColor(input, output, cv::COLOR_RGB2BGRA);
            break;
        case 4:
            cv::cvtColor(input, output, cv::COLOR_RGBA2BGRA);
            break;
#else
        case 1:
            cv::cvtColor(input, output, cv::COLOR_GRAY2RGBA);
            break;
        case 3:
            cv::cvtColor(input, output, cv::COLOR_RGB2RGBA);
            break;
        case 4:
            output = input;
            break;
#endif
        default:
            throw std::runtime_error("Unsupported foramt");
    }
    return output;
}

static cv::Mat draw(const acf::Detector::Pyramid& pyramid)
{
    cv::Mat canvas;
    std::vector<cv::Mat> levels;
    for (int i = 0; i < pyramid.nScales; i++)
    {
        // Concatenate the transposed faces, so they are compatible with the GPU layout
        cv::Mat Ccpu;
        std::vector<cv::Mat> images;
        for (const auto& image : pyramid.data[i][0].get())
        {
            images.push_back(image.t());
        }
        cv::vconcat(images, Ccpu);

        // Instead of upright:
        //cv::vconcat(pyramid.data[i][0].get(), Ccpu);

        if (levels.size())
        {
            cv::copyMakeBorder(Ccpu, Ccpu, 0, levels.front().rows - Ccpu.rows, 0, 0, cv::BORDER_CONSTANT);
        }

        levels.push_back(Ccpu);
    }
    cv::hconcat(levels, canvas);
    return canvas;
}

static void logPyramid(const std::string& filename, const acf::Detector::Pyramid& P)
{
    cv::Mat canvas = draw(P);

    if (canvas.depth() != CV_8UC1)
    {
        canvas.convertTo(canvas, CV_8UC1, 255.0);
    }
    cv::imwrite(filename, canvas);
}

ACF_NAMESPACE_END
