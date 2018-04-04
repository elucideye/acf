/*! -*-c++-*-
  @file   test-acf.cpp
  @author David Hirvonen
  @brief  Google test for the GPU ACF code.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include <gtest/gtest.h>
#include <opencv2/core.hpp>

const char* imageFilename;
const char* truthFilename;
const char* modelFilename;
const char* outputDirectory;

#if defined(ACF_SERIALIZE_WITH_CVMATIO)
const char* acfInriaDetectorFilename;
const char* acfCaltechDetectorFilename;
const char* acfPedestrianImageFilename;
#endif

int gauze_main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    outputDirectory = ".";

    CV_Assert(argc >= 4);
    imageFilename = argv[1];
    truthFilename = argv[2];
    modelFilename = argv[3];


#if defined(ACF_SERIALIZE_WITH_CVMATIO)
    if (argc >= 7)
    {
        acfInriaDetectorFilename = argv[4];
        acfCaltechDetectorFilename = argv[5];
        acfPedestrianImageFilename = argv[6];
    }
#endif

    return RUN_ALL_TESTS();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// clang-format off
#if defined(ACF_ADD_TO_STRING)
#  include <io/stdlib_string.h>
#endif
// clang-format on

// clang-format off
#if defined(ACF_DO_GPU)
#  include <acf/GPUACF.h>
#  include <aglet/GLContext.h>
#endif
// clang-format on

#include <acf/ACF.h>
#include <acf/MatP.h>
#include <util/Logger.h>
#include <io/cereal_pba.h>
#include <util/ScopeTimeLogger.h>

// http://uscilab.github.io/cereal/serialization_archives.html
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>

#include <gtest/gtest.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <fstream>
#include <memory>
#include <iostream>
#include <chrono>

// clang-format off
#ifdef ANDROID
#  define DFLT_TEXTURE_FORMAT GL_RGBA
#else
#  define DFLT_TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

// clang-format off
#define BEGIN_EMPTY_NAMESPACE namespace {
#define END_EMPTY_NAMESPACE }
// clang-format on

BEGIN_EMPTY_NAMESPACE

// http://stackoverflow.com/a/32647694
static bool isEqual(const cv::Mat& a, const cv::Mat& b);
static bool isEqual(const acf::Detector& a, const acf::Detector& b);
//static cv::Mat draw(acf::Detector::Pyramid& pyramid);

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

    void testPedestrianDetector(const char* detectorFilename, const char* inputFilename)
    {
        if (detectorFilename && inputFilename)
        {
            auto detector = create(detectorFilename);
            ASSERT_NE(detector, nullptr);
            if (detector)
            {
                cv::Mat image = cv::imread(inputFilename);
                ASSERT_NE(image.empty(), true);
                if (!image.empty())
                {
                    std::vector<double> scores;
                    std::vector<cv::Rect> objects;
                    detector->setIsTranspose(false);
                    detector->setDoNonMaximaSuppression(true);
                    (*detector)(image, objects, &scores);

                    // TODO: explicit ground truth with Matlab
                    // CAVEAT: There will likely be some cross-platform variation in
                    // actual detection results even within matlab.  We can do some
                    // reasonable comparison for detections where the confidence
                    // is high using the PASCAL critieria: (a & b) / (a | b)
                    // For now, we just make sure we have at least 5 detections
                    // which ensures that the MAT models are loading fine and
                    // produces reasonable detections.
                    ASSERT_GE(objects.size(), 5);

#if ACF_TEST_DISPLAY_OUTPUT
                    WaitKey waitKey;
                    for (auto& d : objects)
                    {
                        cv::rectangle(image, d, { 0, 255, 0 }, 4, 8);
                    }
                    cv::imshow("image", image);
                    cv::waitKey(0);
#endif
                }
            }
        }
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
        const int shrink = m_detector->opts.pPyramid->pChns->shrink.get();        
        auto sizes = getPyramidSizes(Pcpu);
        static const bool doGray = false;
        ogles_gpgpu::Size2d inputSize(image.cols, image.rows);

        m_acf = std::make_shared<ogles_gpgpu::ACF>(nullptr, inputSize, sizes, ogles_gpgpu::ACF::kM012345, doGray, shrink);
        m_acf->setRotation(0);
        m_acf->setDoLuvTransfer(false);

        cv::Mat input = image;

        {
            // Fill in the pyramid:
            (*m_acf)({{ input.cols, input.rows }, input.ptr(), true, 0, DFLT_TEXTURE_FORMAT});
            glFlush();
            m_acf->fill(Pgpu, Pcpu);
        }
    }

    std::shared_ptr<aglet::GLContext> m_context;
    std::shared_ptr<ogles_gpgpu::ACF> m_acf;
#endif // defined(ACF_DO_GPU)

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

/*
`>> result = chnsCompute`

```
result =
shrink: 4
pColor: [1x1 struct]
pGradMag: [1x1 struct]
pGradHist: [1x1 struct]
pCustom: [0x0 struct]
complete: 1
```

`>> result.pColor`
```
ans =
enabled: 1
smooth: 1
colorSpace: 'luv'
```

`>> result.pGradMag`
```
ans =
enabled: 1
colorChn: 0
normRad: 5
normConst: 0.0050
full: 0
```

`>> result.pGradHist`
```
ans =
enabled: 1
binSize: []
nOrients: 6
softBin: 0
useHog: 0
clipHog: 0.2000
```

`>> result.complete`
```
ans =
1
```
 */

static void testChnsDefault(const acf::Detector::Options::Pyramid::Chns& pChns)
{
    ASSERT_EQ(pChns.shrink.get(), 4);
    ASSERT_EQ(pChns.pColor->enabled.get(), 1);
    ASSERT_EQ(pChns.pColor->smooth.get(), 1);
    ASSERT_EQ(pChns.pColor->colorSpace.get(), "luv");
    ASSERT_EQ(pChns.pGradMag->enabled.get(), 1);
    ASSERT_EQ(pChns.pGradMag->colorChn.get(), 0);
    ASSERT_EQ(pChns.pGradMag->normRad.get(), 5);
    ASSERT_EQ(pChns.pGradMag->full.get(), 0);
    ASSERT_EQ(pChns.pGradHist->enabled.get(), 1);
    ASSERT_EQ(pChns.pGradHist->binSize.has, false);
    ASSERT_EQ(pChns.pGradHist->nOrients.get(), 6);
    ASSERT_EQ(pChns.pGradHist->softBin.get(), 0);
    ASSERT_EQ(pChns.pGradHist->useHog.get(), 0);
    ASSERT_EQ(pChns.pGradHist->clipHog.get(), 0.2);
    ASSERT_EQ(pChns.complete.get(), 1);
}

TEST_F(ACFTest, ACFchnsComputeDefault)
{
    acf::Detector::Channels channels;
    acf::Detector::chnsCompute({}, {}, channels, true, {});
    testChnsDefault(channels.pChns);
}

static void rgbToX(const char* filename, const std::string& color)
{
    acf::Detector::Channels dflt, channels;
    acf::Detector::chnsCompute({}, {}, dflt, true, {});
    dflt.pChns.pColor->colorSpace = color;

    cv::Mat image = cv::imread(filename);
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
    image.convertTo(image, CV_32FC3, 1.0 / 255.0); // convert to float

    ASSERT_NE(image.empty(), true);
    ASSERT_EQ(image.channels(), 3);

    if (!image.empty())
    {
        MatP I(image);
        acf::Detector::chnsCompute(I, dflt.pChns, channels, false, {});

        for (int i = 0; i < channels.data.size(); i++)
        {
            // At the very least we should have
            ASSERT_EQ(channels.info[i].nChns, channels.data[i].channels());
        }
    }
}

TEST_F(ACFTest, ACFchnsComputeRgbToGray)
{
    rgbToX(imageFilename, "gray");
}

TEST_F(ACFTest, ACFchnsComputeRgbToLuv)
{
    rgbToX(imageFilename, "luv");
}

/*
`>> result = chnsPyramid`
```
result =
pChns: [1x1 struct]
nPerOct: 8
nOctUp: 0
nApprox: 7
lambdas: []
pad: [0 0]
minDs: [16 16]
smooth: 1
concat: 1
complete: 1
```
w/ `>> result.pChns` same as above
*/

TEST_F(ACFTest, ACFchnsPyramidDefault)
{
    acf::Detector detector;

    acf::Detector::Pyramid pyramid;
    detector.chnsPyramid({}, nullptr, pyramid, true, {});

    testChnsDefault(pyramid.pPyramid.pChns);
    const auto& pPyramid = pyramid.pPyramid;

    ASSERT_EQ(pPyramid.nPerOct.get(), 8);
    ASSERT_EQ(pPyramid.nOctUp.get(), 0);
    ASSERT_EQ(pPyramid.nApprox.get(), 7);
    ASSERT_EQ(pPyramid.lambdas.has, false);
    ASSERT_EQ(pPyramid.pad.get(), cv::Size(0, 0));
    ASSERT_EQ(pPyramid.minDs.get(), cv::Size(16, 16));
    ASSERT_EQ(pPyramid.smooth.get(), 1);
    ASSERT_EQ(pPyramid.concat.get(), 1);
    ASSERT_EQ(pPyramid.complete.get(), 1);

    const auto& pChns = pPyramid.pChns;
    testChnsDefault(pChns);
}

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

    // load cereal pba cv::Mat, compare precision, etc
    ASSERT_GT(pyramid->data.max_size(), 0);
}

#if defined(ACF_SERIALIZE_WITH_CVMATIO)
TEST_F(ACFTest, ACFInriaDetector)
{
    testPedestrianDetector(acfInriaDetectorFilename, acfPedestrianImageFilename);
}

TEST_F(ACFTest, ACFCaltechDetector)
{
    testPedestrianDetector(acfCaltechDetectorFilename, acfPedestrianImageFilename);
}
#endif // defined(ACF_SERIALIZE_WITH_CVMATIO)

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
    (*m_detector)(Pgpu, objects);

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
