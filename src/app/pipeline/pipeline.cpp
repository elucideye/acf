/*! -*-c++-*-
  @file   pipeline.cpp
  @author David Hirvonen
  @brief GPU accelerated ACF object detection application. 

  This application performs GPU accelerated ACF object detection.
  In particular, it makes use of the acf::GPUACF class (ogles_gpgpu shaders)
  to compute ACF pyramids on the GPU in pure OpenGL, which are retrieved
  (with latency) and used for "sliding window" multi-scale object detection 
  on the CPU.

  Since the object detection (gradient boosting w/ lots of random memory access)
  doesn't map well to low end mobile GPU's, the multi-scale detection runs on
  the CPU, but makes use of multi-scale ACF features from the GPU that don't
  compute with CPU resources.  In most cases the GPU->CPU transfer will be a
  significant bottleneck, and a naive implementation (src/app/acf/GLDetector.{h,cpp})
  may very well run slower than the end-to-end CPU based implementation.

  In this sample application, some latency is introduced and the GPU->CPU 
  transfer overhead associated with the ACF pyramids is hidden in that time.  
  This assumes the target application can tolerate a little latency to improve
  overall throughput.  Even if the full CPU processing achieves the desired 
  frame rate, it can be beneficial to offload the feature computation to the GPU
  in order to reduce CPU load and power consumption.

  In order to run this efficiently, you will also want to make sure that you
  avoid searching for faces at unnecessary scales.  For example, if you have
  high resolution input images, then it is very unlikely you will need to
  search for faces down at the scales matching the object detection training
  samples.  In ACF face detection publications, 80x80 has been shown to work
  well in practice (good accuracy vs computation balance).  In most selfie
  type face detection applications, at least, there is no need to search for 
  faces of size 80x80, and an enormous amount of computation can be saved
  by resizing the input images such that the smallest faces one wants to find
  end up at matching the detector window size (i.e., 80x80 or thereabouts).

  If we are using the drishti_face_gray_80x80.cpb model, then we may want to
  call this application with the following parameters:

  acf-pipeline \
      --input=0 \
      --model=${SOME_PATH_VAR}/drishti-assets/drishti_face_gray_80x80.cpb
      --minimum=200 \
      --calibration=0.01 \
      --window

  In the above command, "minimum=200" means we are ignoring all faces
  less than 200 pixels wide.  You should set this to the largest value
  that makes sense for your application. 

  Note that the "calibration" term may also be required to achieve the
  desired recall for a given detector.  See the ACF publications referenced
  on the main README.rst page for additional details on this parameter.

  \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#if defined(ACF_ADD_TO_STRING)
#  include <io/stdlib_string.h> // first
#endif

#include <util/Logger.h>

#include "GPUDetectionPipeline.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <aglet/GLContext.h>

#include <ogles_gpgpu/common/proc/disp.h>

#include <cxxopts.hpp>

// clang-format off
#ifdef ANDROID
#  define TEXTURE_FORMAT GL_RGBA
#else
#  define TEXTURE_FORMAT GL_BGRA
#endif
// clang-format on

template <typename T>
void* void_ptr(const T* ptr)
{
    return static_cast<void*>(const_cast<T*>(ptr));
}

static std::shared_ptr<cv::VideoCapture> create(const std::string& filename);
static cv::Size getSize(cv::VideoCapture& video);

struct Application
{
    // clang-format off
    Application
    (
        const std::string &input,
        const std::string &model,
        float acfCalibration,
        int minWidth,
        bool window,
        float resolution
    ) : resolution(resolution)
    // clang-format on
    {
        // Create a video source:
        // 1) integar == index to device camera
        // 2) filename == supported video formats
        // 3) "/fullpath/Image_%03d.png" == list of stills
        // http://answers.opencv.org/answers/761/revisions/
        video = create(input);

        // Create an OpenGL context:
        const auto size = getSize(*video);
        context = aglet::GLContext::create(aglet::GLContext::kAuto, window ? "acf" : "", size.width, size.height);

        // Create an object detector:
        detector = std::make_shared<acf::Detector>(model);
        detector->setDoNonMaximaSuppression(true);
        if (acfCalibration != 0.f)
        {
            acf::Detector::Modify dflt;
            dflt.cascThr = { "cascThr", -1.0 };
            dflt.cascCal = { "cascCal", acfCalibration };
            detector->acfModify(dflt);
        }

        // Create the asynchronous scheduler:
        pipeline = std::make_shared<acf::GPUDetectionPipeline>(detector, size, 5, 0, minWidth);

        // Instantiate an ogles_gpgpu display class that will draw to the
        // default texture (0) which will be managed by aglet (typically glfw)
        if (window && context->hasDisplay())
        {
            display = std::make_shared<ogles_gpgpu::Disp>();
            display->init(size.width, size.height, TEXTURE_FORMAT);
            display->setOutputRenderOrientation(ogles_gpgpu::RenderOrientationFlipped);
        }
    }

    void setLogger(std::shared_ptr<spdlog::logger>& logger)
    {
        this->logger = logger;
    }

    bool update()
    {
        cv::Mat frame;
        (*video) >> frame;

        if (frame.empty())
        {
            return false; // indicate failure, exit loop
        }

        if (frame.channels() == 3)
        {
// ogles_gpgpu supports both {BGR,RGB}A and NV{21,12} inputs, and
// cv::VideoCapture support {RGB,BGR} output, so we need to add an
// alpha plane.  Doing this on the CPU is wasteful, and it would be
// better to access the camera directly for NV{21,12} processing
#if ANDROID
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA); // android need GL_RGBA
#else
            cv::cvtColor(frame, frame, cv::COLOR_BGR2BGRA); // assume all others are GL_BGRA
#endif
        }

        auto result = (*pipeline)({ { frame.cols, frame.rows }, void_ptr(frame.data), true, false, TEXTURE_FORMAT }, true);

        if (logger)
        {
            logger->info("OBJECTS : {}", result.second.roi.size());
        }

        if (display)
        {
            show(result.first);
        }

        return true; // continue sequence
    }

    void show(GLuint texture)
    {
        auto& geometry = context->getGeometry();
        display->setOffset(geometry.tx, geometry.ty);
        display->setDisplayResolution(geometry.sx * resolution, geometry.sy * resolution);
        display->useTexture(texture);
        display->render(0);
    }

    float resolution = 1.f;

    std::shared_ptr<spdlog::logger> logger;

    std::shared_ptr<aglet::GLContext> context;
    std::shared_ptr<ogles_gpgpu::Disp> display;

    std::shared_ptr<cv::VideoCapture> video;
    std::shared_ptr<acf::Detector> detector;
    std::shared_ptr<acf::GPUDetectionPipeline> pipeline;
};

int main(int argc, char** argv)
{
    auto logger = util::Logger::create("acf-pipeline");

    bool help = false, doWindow = false;
    float resolution = 1.f, acfCalibration = 0.f;
    std::string sInput, sOutput, sModel;
    int minWidth = 0;

    const int argumentCount = argc;
    cxxopts::Options options("acf-pipeline", "GPU accelerated ACF object detection (see Piotr's toolbox)");

    // clang-format off
    options.add_options()
        ("i,input", "Input file", cxxopts::value<std::string>(sInput))
        ("o,output", "Output directory", cxxopts::value<std::string>(sOutput))
        ("m,model", "Model file", cxxopts::value<std::string>(sModel))
        ("c,calibration", "ACF calibration", cxxopts::value<float>(acfCalibration))
        ("r,resolution", "Resolution", cxxopts::value<float>(resolution))
        ("w,window", "Window", cxxopts::value<bool>(doWindow))
        ("M,minimum", "Minimum object width", cxxopts::value<int>(minWidth))
        ("h,help", "Help message", cxxopts::value<bool>(help))
        ;
    // clang-format on

    options.parse(argc, argv);
    if ((argumentCount <= 1) || options.count("help"))
    {
        std::cout << options.help({ "" }) << std::endl;
        return 0;
    }

    if (sModel.empty())
    {
        logger->error("Must specify a valid model");
        return 1;
    }

    if (sInput.empty())
    {
        logger->error("Must specify input image");
        return 1;
    }

    Application app(sInput, sModel, acfCalibration, minWidth, doWindow, resolution);
    app.setLogger(logger);

    aglet::GLContext::RenderDelegate delegate = [&]() {
        return app.update();
    };

    (*app.context)(delegate);

    logger->info("DONE");

    return 0;
}

// :::: utility ::::

static std::shared_ptr<cv::VideoCapture> create(const std::string& filename)
{
    if (filename.find_first_not_of("0123456789") == string::npos)
    {
        return std::make_shared<cv::VideoCapture>(std::stoi(filename));
    }
    else
    {
        return std::make_shared<cv::VideoCapture>(filename);
    }
}

static cv::Size getSize(cv::VideoCapture& video)
{
    return {
        static_cast<int>(video.get(CV_CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(video.get(CV_CAP_PROP_FRAME_HEIGHT))
    };
}
