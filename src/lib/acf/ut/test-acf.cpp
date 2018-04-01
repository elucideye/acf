/*! -*-c++-*-
  @file   test-acf.cpp
  @author David Hirvonen
  @brief  Google test for the GPU ACF code.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest.h>
#include <opencv2/core.hpp>

extern const char* imageFilename;
extern const char* truthFilename;
extern const char* modelFilename;
extern const char* outputDirectory;

int gauze_main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    CV_Assert(argc == 5);

    imageFilename = argv[1];
    truthFilename = argv[2];
    modelFilename = argv[3];
    outputDirectory = argv[4];

    return RUN_ALL_TESTS();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// clang-format off
#if defined(ACF_DO_GPU)
#  include "acf/GPUACF.h"
#  include "aglet/GLContext.h"
#endif
// clang-format on

// clang-format off
#if defined(ACF_ADD_TO_STRING)
#  include "io/stdlib_string.h"
#endif
// clang-format on
#include "io/cereal_pba.h"

#include "util/ScopeTimeLogger.h"

#define ACF_LOG_GPU_TIME 0

// http://uscilab.github.io/cereal/serialization_archives.html
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>

// #!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!
// #!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!
// #!#!#!#!#!#!#!#!#!#!#!#!#!#!# Work in progress !#!#!#!#!#!#!#!#!#!#!#!#!
// #!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!
// #!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!#!

#include <gtest/gtest.h>

#include "acf/ACF.h"
#include "acf/MatP.h"
#include "util/Logger.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <memory>

const char* imageFilename;
const char* truthFilename;
const char* modelFilename;
const char* outputDirectory;

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

#include <iostream>
#include <chrono>

// clang-format off
#define BEGIN_EMPTY_NAMESPACE namespace {
#define END_EMPTY_NAMESPACE }
// clang-format on

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

BEGIN_EMPTY_NAMESPACE

// http://stackoverflow.com/a/32647694
static bool isEqual(const cv::Mat& a, const cv::Mat& b);
static bool isEqual(const acf::Detector& a, const acf::Detector& b);
static cv::Mat draw(acf::Detector::Pyramid& pyramid);

class ACFTest : public ::testing::Test
{
protected:
    bool m_hasTranspose = false;

    // Setup
    ACFTest()
    {
        m_logger = util::Logger::create("test-acf");
        m_logger->set_level(spdlog::level::off); // by default...

        // Load the ground truth data:
        image = loadImage(imageFilename);
        if (m_hasTranspose)
        {
            image = image.t();
        }

        // Load input suitable for ACF processing
        // * single precision floatin point
        // * RGB channel order
        loadACFInput(imageFilename);

// TODO: we need to load ground truth output for each shader
// (some combinations could be tested, but that is probably excessive!)
// truth = loadImage(truthFilename);

#if defined(ACF_DO_GPU)
        m_context = aglet::GLContext::create(aglet::GLContext::kAuto);
        CV_Assert(m_context && (*m_context));
#endif
    }

    // Cleanup
    virtual ~ACFTest()
    {
        util::Logger::drop("test-acf");
    }

    std::shared_ptr<acf::Detector> create(const std::string& filename, bool do10Channel = true)
    {
        return std::make_shared<acf::Detector>(filename);
    }

    // Called after constructor for each test
    virtual void SetUp() {}

    // Called after destructor for each test
    virtual void TearDown() {}

    std::shared_ptr<acf::Detector> getDetector()
    {
        if (!m_detector)
        {
            m_detector = create(modelFilename);
        }
        return m_detector;
    }

    static cv::Mat loadImage(const std::string& filename)
    {
        assert(!filename.empty());
        cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);

        assert(!image.empty() && image.type() == CV_8UC3);
        cv::Mat tmp;
        cv::cvtColor(image, tmp, cv::COLOR_BGR2BGRA);
        cv::swap(image, tmp);
        return image;
    }

    // Load and format a single image for reuse in multiple tests:
    void loadACFInput(const std::string& filename)
    {
        cv::Mat I = loadImage(filename);
        cv::cvtColor(I, m_I, cv::COLOR_BGR2RGB);
        m_I.convertTo(m_I, CV_32FC3, (1.0 / 255.0));
        m_IpT = MatP(m_I.t());
        m_hasTranspose = false;
    }

#if defined(ACF_DO_GPU)

    static std::vector<ogles_gpgpu::Size2d> getPyramidSizes(acf::Detector::Pyramid& Pcpu)
    {
        std::vector<ogles_gpgpu::Size2d> sizes;
        for (int i = 0; i < Pcpu.nScales; i++)
        {
            const auto size = Pcpu.data[i][0][0].size();
            sizes.emplace_back(size.height * 4, size.width * 4); // transpose
        }
        return sizes;
    }

    // Utility method for code reuse in common GPU tests:
    //
    // Output:
    // 1) acf::Detector::Pyramid from GPU
    //
    // State:
    // 1) Allocates acf::Detector
    // 2) Allocates ogles_gpgpu::ACF

    void initGPUAndCreatePyramid(acf::Detector::Pyramid& Pgpu)
    {
        m_detector = create(modelFilename);

        // Compute a reference pyramid on the CPU:
        acf::Detector::Pyramid Pcpu;
        m_detector->computePyramid(m_IpT, Pcpu);

        ASSERT_NE(m_detector, nullptr);

        m_detector->setIsTranspose(true);
        m_detector->computePyramid(m_IpT, Pcpu);
        auto sizes = getPyramidSizes(Pcpu);
        static const bool doGrayscale = false;
        ogles_gpgpu::Size2d inputSize(image.cols, image.rows);

        m_acf = std::make_shared<ogles_gpgpu::ACF>(nullptr, inputSize, sizes, ogles_gpgpu::ACF::kM012345, doGrayscale, false);
        m_acf->setRotation(0);
        m_acf->setDoLuvTransfer(false);

        cv::Mat input = image;
        
        {
            // Fill in the pyramid:
            (*m_acf)({ input.cols, input.rows }, input.ptr(), true, 0, DFLT_TEXTURE_FORMAT);
            glFlush();
            m_acf->fill(Pgpu, Pcpu);
        }
    }

    std::shared_ptr<aglet::GLContext> m_context;
    std::shared_ptr<ogles_gpgpu::ACF> m_acf;
#endif

    std::shared_ptr<spdlog::logger> m_logger;

    std::shared_ptr<acf::Detector> m_detector;

    // Test images:
    cv::Mat image, truth;

    // ACF inputs: RGB single precision floating point
    cv::Mat m_I;
    MatP m_IpT;
};

// This block demonstrates how to retreive output
//#if defined(ACF_DO_GPU)
//static cv::Mat getImage(ogles_gpgpu::ProcInterface& proc)
//{
//    cv::Mat result(proc.getOutFrameH(), proc.getOutFrameW(), CV_8UC4);
//    proc.getResultData(result.ptr());
//    return result;
//}
//#endif // defined(ACF_DO_GPU)

// This is a WIP, currently we test the basic CPU detection functionality
// with a sample image.  Given the complexity of the GPU implementation,
// more tests will need to be added for lower level channel computation,
// such as CPU vs GPU error bounds.  For now the simple detection success
// test and a simple placeholder assert(true) test wil be added at the
// end of the test.

TEST_F(ACFTest, ACFSerializeCereal)
{
    // Load from operative format:
    auto detector = create(modelFilename);
    ASSERT_NE(detector, nullptr);

    // Test *.cpb serialization (write and load)
    acf::Detector detector2;
    std::string filename = outputDirectory;
    filename += "/acf.cpb";
    save_cpb(filename, *detector);
    load_cpb(filename, detector2);
    ASSERT_TRUE(isEqual(*detector, detector2));
}

//static void draw(cv::Mat& canvas, const std::vector<cv::Rect>& objects)
//{
//    for (const auto& r : objects)
//    {
//        cv::rectangle(canvas, r, { 0, 255, 0 }, 1, 8);
//    }
//}

TEST_F(ACFTest, ACFDetectionCPUMat)
{
    auto detector = getDetector();
    ASSERT_NE(detector, nullptr);

    std::vector<double> scores;
    std::vector<cv::Rect> objects;
    detector->setIsTranspose(false);
    (*detector)(m_I, objects, &scores);

    ASSERT_GT(objects.size(), 0); // Very weak test!!!
}

TEST_F(ACFTest, ACFDetectionCPUMatP)
{
    auto detector = getDetector();
    ASSERT_NE(detector, nullptr);

    std::vector<double> scores;
    std::vector<cv::Rect> objects;

    // Input is transposed, but objects will be returned in upright coordinate system
    detector->setIsTranspose(true);
    (*detector)(m_IpT, objects, &scores);

    ASSERT_GT(objects.size(), 0); // Very weak test!!!
}

// Pull out the ACF intermediate results from the logger:
//
//using ChannelLogger = int(const cv::Mat &, const std::string &);
//cv::Mat M, Mnorm, O, L, U, V, H;
//std::function<ChannelLogger> logger = [&](const cv::Mat &I, const std::string &tag) -> int
//{
//    if(tag.find("L:") != std::string::npos) { L = I.t(); }
//    else if(tag.find("U:") != std::string::npos) { U = I.t(); }
//    else if(tag.find("V:") != std::string::npos) { V = I.t(); }
//    else if(tag.find("Mnorm:") != std::string::npos) { Mnorm = I.t(); }
//    else if(tag.find("M:") != std::string::npos) { M = I.t(); }
//    else if(tag.find("O:") != std::string::npos) { O = I.t(); }
//    else if(tag.find("H:") != std::string::npos) { H = I.t(); }
//    return 0;
//};
//detector->setLogger(logger);

TEST_F(ACFTest, ACFChannelsCPU)
{
    auto detector = getDetector();
    ASSERT_NE(detector, nullptr);

    MatP Ich;
    detector->setIsTranspose(true);
    detector->computeChannels(m_IpT, Ich);

    // TODO: comparison for channels:
    // load cereal pba cv::Mat, compare precision, etc
    ASSERT_EQ(Ich.base().empty(), false);
}

TEST_F(ACFTest, ACFPyramidCPU)
{
    auto detector = getDetector();
    ASSERT_NE(detector, nullptr);

    auto pyramid = std::make_shared<acf::Detector::Pyramid>();
    detector->setIsTranspose(true);
    detector->computePyramid(m_IpT, *pyramid);

    // TODO: comparision for pyramid:
    // load cereal pba cv::Mat, compare precision, etc
    ASSERT_GT(pyramid->data.max_size(), 0);
}

#if defined(ACF_DO_GPU)
TEST_F(ACFTest, ACFPyramidGPU10)
{
    acf::Detector::Pyramid Pgpu;
    initGPUAndCreatePyramid(Pgpu);
    ASSERT_NE(m_detector, nullptr);
    ASSERT_NE(m_acf, nullptr);

    // TODO: comparision for pyramid:
    // load cereal pba cv::Mat, compare precision, etc
    ASSERT_GT(Pgpu.data.max_size(), 0);

    // Compare precision with CPU implementation (very loose)
}

TEST_F(ACFTest, ACFDetectionGPU10)
{
    acf::Detector::Pyramid Pgpu;
    initGPUAndCreatePyramid(Pgpu);
    ASSERT_NE(m_detector, nullptr);
    ASSERT_NE(m_acf, nullptr);

    std::vector<double> scores;
    std::vector<cv::Rect> objects;
    {
#if ACF_LOG_GPU_TIME
        util::ScopeTimeLogger logger = [](double elapsed)
        {
            std::cout << "acf::Detector::operator():" << elapsed << std::endl;
        };
#endif
        (*m_detector)(Pgpu, objects);
    }

    ASSERT_GT(objects.size(), 0); // Very weak test!!!
}
#endif // defined(ACF_DO_GPU)

// ### utility ###

// http://stackoverflow.com/a/32647694
static bool isEqual(const cv::Mat& a, const cv::Mat& b)
{
    cv::Mat temp;
    cv::bitwise_xor(a, b, temp); //It vectorizes well with SSE/NEON
    return !(cv::countNonZero(temp));
}

static bool isEqual(const acf::Detector& a, const acf::Detector& b)
{
    if (!isEqual(a.clf.fids, b.clf.fids))
    {
        std::cout << cv::Mat1b(a.clf.fids == b.clf.fids) << std::endl;

        cv::Mat tmp;
        cv::hconcat(a.clf.fids, b.clf.fids, tmp);
        std::cout << tmp << std::endl;
        return false;
    }

    if (!isEqual(a.clf.child, b.clf.child))
    {
        std::cout << cv::Mat1b(a.clf.child == b.clf.child) << std::endl;

        cv::Mat tmp;
        cv::hconcat(a.clf.fids, b.clf.fids, tmp);
        std::cout << tmp << std::endl;
        return false;
    }

    if (!isEqual(a.clf.depth, b.clf.depth))
    {
        std::cout << cv::Mat1b(a.clf.depth == b.clf.depth) << std::endl;

        cv::Mat tmp;
        cv::hconcat(a.clf.fids, b.clf.fids, tmp);
        std::cout << tmp << std::endl;
        return false;
    }

    return true;

    // The float -> uint16_t -> float will not be an exact match
    //isEqual(a.clf.thrs, b.clf.thrs) &&
    //isEqual(a.clf.hs, b.clf.hs) &&
    //isEqual(a.clf.weights, b.clf.weights) &&
}

// This block demonstrates how to visualize a pyramid structure:
//static cv::Mat draw(acf::Detector::Pyramid& pyramid)
//{
//    cv::Mat canvas;
//    std::vector<cv::Mat> levels;
//    for (int i = 0; i < pyramid.nScales; i++)
//    {
//        // Concatenate the transposed faces, so they are compatible with the GPU layout
//        cv::Mat Ccpu;
//        std::vector<cv::Mat> images;
//        for (const auto& image : pyramid.data[i][0].get())
//        {
//            images.push_back(image.t());
//        }
//        cv::vconcat(images, Ccpu);
//
//        // Instead of upright:
//        //cv::vconcat(pyramid.data[i][0].get(), Ccpu);
//
//        if (levels.size())
//        {
//            cv::copyMakeBorder(Ccpu, Ccpu, 0, levels.front().rows - Ccpu.rows, 0, 0, cv::BORDER_CONSTANT);
//        }
//
//        levels.push_back(Ccpu);
//    }
//    cv::hconcat(levels, canvas);
//    return canvas;
//}

END_EMPTY_NAMESPACE
