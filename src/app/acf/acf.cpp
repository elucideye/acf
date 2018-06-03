/*! -*-c++-*-
  @file   acf.cpp
  @author David Hirvonen
  @brief  Aggregated channel feature computation and object detection.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

// Local includes:

// clang-format off
#if defined(ACF_ADD_TO_STRING)
#  include "io/stdlib_string.h" // android workaround
#endif
// clang-format on

#include <acf/ACF.h>

// clang-format off
#if defined(ACF_DO_GPU)
#  include <acf/GLDetector.h>
#endif
// clang-format on

// Private/internal headers:
#include <util/LazyParallelResource.h>
#include <util/Line.h>
#include <util/Logger.h>
#include <util/Parallel.h>
#include <util/make_unique.h>
#include <util/string_utils.h>
#include <util/cli.h>
#include <io/cv_cereal.h>
#include <io/cereal_pba.h> // optional

// Package includes:
#include <cxxopts.hpp>

#include <opencv2/highgui.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/json.hpp>

#include <type_traits>

using ObjectDetectorPtr = std::shared_ptr<acf::ObjectDetector>;
using AcfPtr = std::shared_ptr<acf::Detector>;
using RectVec = std::vector<cv::Rect>;

static void randomShapes(cv::Mat& image, int n);

struct VideoSource
{
    struct Frame
    {
        std::string name;
        cv::Mat image;
    };

    VideoSource(const std::string& filename)
    {
        filenames = util::cli::expand(filename);
    }

    VideoSource(int n)
        : m_n(n) // random frames
    {
    }

    virtual Frame operator()(int i)
    {
        Frame frame;
        if (filenames.size())
        {
            frame.name = filenames[i];
            frame.image = cv::imread(filenames[i], cv::IMREAD_COLOR);
        }
        else
        {
            frame.name = std::to_string(i);
            frame.image = cv::Mat::zeros(640, 480, CV_8UC3);
            randomShapes(frame.image, rand() % 32);
        }

        return frame;
    }

    virtual bool isRandomAccess() const
    {
        return true;
    }

    virtual std::size_t size()
    {
        return filenames.size() ? filenames.size() : m_n;
    }

    int m_n = 0;
    std::vector<std::string> filenames;
};

static bool writeAsJson(const std::string& filename, const std::vector<cv::Rect>& objects);
static bool writeAsText(const std::string& filename, const std::vector<cv::Rect>& objects);
static void drawObjects(cv::Mat& canvas, const std::vector<cv::Rect>& objects);
static cv::Rect2f operator*(const cv::Rect2f& roi, float scale);
static void chooseBest(std::vector<cv::Rect>& objects, std::vector<double>& scores);

// Resize input image to detection objects of minimum width
// given an object detection window size. i.e.,
//
// ACF windowSize = {48x48};
//
// To restrict object detection search to find objects of
// minWidth == 100 pixels, we would need to *downsample*
// the image by a factor of 48/100 (nearly 0.5x)

class Resizer
{
public:
    Resizer(cv::Mat& image, const cv::Size& winSize, int width = -1)
    {
        if ((width >= 0) && !image.empty())
        {
            scale = static_cast<float>(winSize.width) / static_cast<float>(width);
            const int interpolation = (scale < 1.f) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(image, reduced, {}, scale, scale, interpolation);
        }
        else
        {
            reduced = image;
        }
    }

    void operator()(std::vector<cv::Rect>& objects) const
    {
        if (scale != 1.f)
        {
            for (auto& o : objects)
            {
                o = cv::Rect2f(o) * (1.f / scale);
            }
        }
    }
    operator cv::Mat() { return reduced; }

    float scale = 1.f;
    cv::Mat reduced;
};

int gauze_main(int argc, char** argv)
{
    const auto argumentCount = argc;

    // Instantiate line logger:
    auto logger = util::Logger::create("acf-detect");

    // ############################
    // ### Command line parsing ###
    // ############################

    std::string sInput, sOutput, sModel;
    int threads = -1;
    bool doScoreLog = false;
    bool doAnnotation = false;
    bool doPositiveOnly = false;
    bool doSingleDetection = false;
    bool doWindow = false;
    bool doBox = false;
    bool doNms = false;
    bool doGpu = false;
    bool doPyramids = false;
    bool doRandom = false;
    double cascCal = 0.0;
    int minWidth = -1; // minimum object width
    //int maxWidth = -1; /* maximum object width TODO */

    cxxopts::Options options("acf-detect", "Command line interface for ACF object detection (see Piotr's toolbox)");

    // clang-format off
    options.add_options()
        ("i,input", "Input file", cxxopts::value<std::string>(sInput))
        ("o,output", "Output directory", cxxopts::value<std::string>(sOutput))
        ("m,model", "Input file", cxxopts::value<std::string>(sModel))
        ("n,nms", "Non maximum suppression", cxxopts::value<bool>(doNms))
        ("l,min", "Minimum object width (lower bound)", cxxopts::value<int>(minWidth))
        ("c,calibration", "Cascade calibration", cxxopts::value<double>(cascCal))
        ("a,annotate", "Create annotated images", cxxopts::value<bool>(doAnnotation))
        ("p,positive", "Do positive only", cxxopts::value<bool>(doPositiveOnly))
        ("b,box", "Box output files", cxxopts::value<bool>(doBox))
        ("t,threads", "Thread count", cxxopts::value<int>(threads))
        ("s,scores", "Log the filenames + max score", cxxopts::value<bool>(doScoreLog))
        ("1,single", "Single detection (max)", cxxopts::value<bool>(doSingleDetection))
        ("w,window", "Use window preview", cxxopts::value<bool>(doWindow))
        ("g,gpu", "Use gpu pyramid processing", cxxopts::value<bool>(doGpu))
        ("pyramids", "Dump pyramids", cxxopts::value<bool>(doPyramids))
        ("random", "Random frames", cxxopts::value<bool>(doRandom))
        ("h,help", "Print help message");
    // clang-format on

    options.parse(argc, argv);

    if ((argumentCount <= 1) || options.count("help"))
    {
        std::cout << options.help({ "" }) << std::endl;
        return 0;
    }

    // ############################################
    // ### Command line argument error checking ###
    // ############################################

    // ### Directory
    if (sOutput.empty())
    {
        logger->error("Must specify output directory");
        return 1;
    }

    if (util::cli::directory::exists(sOutput, ".acf-detect"))
    {
        std::string filename = sOutput + "/.acf-detect";
        remove(filename.c_str());
    }
    else
    {
        logger->error("Specified directory {} does not exist or is not writeable", sOutput);
        return 1;
    }

    // ### Model
    if (sModel.empty())
    {
        logger->error("Must specify model file");
        return 1;
    }
    if (!util::cli::file::exists(sModel))
    {
        logger->error("Specified model file does not exist or is not readable");
        return 1;
    }

    std::shared_ptr<VideoSource> video;
    if (doRandom)
    {
        video = std::make_shared<VideoSource>(1000);
    }
    else
    {
        video = std::make_shared<VideoSource>(sInput); // list of files
    }

    // Allocate resource manager:
    util::LazyParallelResource<std::thread::id, AcfPtr> manager = [&]() {

        // Declare teh core acf::Detector module, which runs on the CPU
        // although we may wrap this with an OpenGL ACF pyramid computation
        // class if we are testing the GPU functionality.  Note that this
        // is intended primarily as an API test, and since we aren't doing
        // anything clever to utilize multi-threading that hides the
        // CPU -> GPU -> CPU transfer it is very likely to actually run
        // slower than the pure CPU approach.  A real application should
        // "hide" the transfer through appropriately multi-threading.
        AcfPtr acf;

#if defined(ACF_DO_GPU)
        if (doGpu)
        {
            acf = std::make_shared<acf::GLDetector>(sModel);
        }
#endif

        if (!acf)
        {
            acf = std::make_shared<acf::Detector>(sModel);
        }

        if (acf.get() && acf->good())
        {
            // Cofigure parameters:
            acf->setDoNonMaximaSuppression(doNms);

            // Cascade threhsold adjustment:
            if (cascCal != 0.f)
            {
                acf::Detector::Modify dflt;
                dflt.cascThr = { "cascThr", -1.0 };
                dflt.cascCal = { "cascCal", cascCal };
                acf->acfModify(dflt);
            }
        }
        else
        {
            acf.reset();
        }

        return acf;
    };

    std::size_t total = 0;
    std::vector<std::pair<std::string, float>> scores;

    // Parallel loop:
    util::ParallelHomogeneousLambda harness = [&](int i) {
        // Get thread specific segmenter lazily:
        auto& detector = manager[std::this_thread::get_id()];
        assert(detector);

        auto winSize = detector->getWindowSize();
        if (!detector->getIsRowMajor())
        {
            std::swap(winSize.width, winSize.height);
        }

        // Load current image
        auto frame = (*video)(i);
        auto& image = frame.image;

        if (!image.empty())
        {
            cv::Mat imageRGB;
            switch (image.channels())
            {
                case 1:
                    cv::cvtColor(image, imageRGB, cv::COLOR_GRAY2RGB);
                    break;
                case 3:
                    cv::cvtColor(image, imageRGB, cv::COLOR_BGR2RGB);
                    break;
                case 4:
                    cv::cvtColor(image, imageRGB, cv::COLOR_BGRA2RGB);
                    break;
            }

            std::vector<double> scores;
            std::vector<cv::Rect> objects;
            if (image.size() == winSize)
            {
                const float score = detector->evaluate(imageRGB);
                scores.push_back(score);
                objects.push_back(cv::Rect({ 0, 0 }, image.size()));
            }
            else
            {
                Resizer resizer(imageRGB, detector->getWindowSize(), minWidth);
                (*detector)(resizer, objects, &scores);
                resizer(objects);

                if (doSingleDetection)
                {
                    chooseBest(objects, scores);
                }
            }

            if (!doPositiveOnly || (objects.size() > 0))
            {
                // Construct valid filename with no extension:
                std::string base = util::basename(frame.name);
                std::string filename = sOutput + "/" + base;

                float maxScore = -1e6f;
                auto iter = std::max_element(scores.begin(), scores.end());
                if (iter != scores.end())
                {
                    maxScore = *iter;
                }

                if (doPyramids)
                {
                    // The "--pyramid" command line option can be used to visualize the
                    // CPU and GPU computed ACF pyramids.  This was added to acf-detect
                    // in order to help reduce differences between the shader and SSE/CPU
                    // computed features.  If we are in a GPU model (using acf::GLDetector)
                    // then we dump both the CPU and the GPU pyramids and we use a reset()
                    // method in order to ensure the CPU pyramid will be computed for each
                    // frame.
#if defined(ACF_DO_GPU)
                    if (auto* handle = dynamic_cast<acf::GLDetector*>(detector.get()))
                    {
                        cv::Mat Pcpu = handle->draw(false);
                        cv::Mat Pgpu = handle->draw(true);
                        cv::imwrite(filename + "_Pcpu.png", Pcpu);
                        cv::imwrite(filename + "_Pgpu.png", Pgpu);
                        handle->clear();
                    }
#endif
                }

                if (doScoreLog)
                {
                    logger->info("SCORE: {} = {}", filename, maxScore);
                }
                else
                {
                    logger->info("{}/{} {} = {}; score = {}", ++total, video->size(), frame.name, objects.size(), maxScore);
                }

                // Save detection results in JSON:
                if (!writeAsJson(filename + ".json", objects))
                {
                    logger->error("Failed to write: {}.json", filename);
                }

                if (doBox && !writeAsText(filename + ".roi", objects))
                {
                    logger->error("Failed to write: {}.box", filename);
                }

                if (doAnnotation || doWindow)
                {
                    cv::Mat canvas = image.clone();
                    drawObjects(canvas, objects);

                    if (doAnnotation)
                    {
                        cv::imwrite(filename + "_objects.jpg", canvas);
                    }

                    if (doWindow)
                    {
                        cv::imshow("acf", canvas);
                        cv::waitKey(1);
                    }
                }
            }
        }
    };

    auto count = static_cast<int>(video->size());
    if (doGpu || threads == 1 || threads == 0 || doWindow || !video->isRandomAccess())
    {
        for (int i = 0; (i < count); i++)
        {
            // Increment manually, to check for end of file:
            harness({ i, i + 1 });
        }
    }
    else
    {
        cv::parallel_for_({ 0, count }, harness, std::max(threads, -1));
    }

    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        std::string home;
#if !(defined(_WIN32) || defined(_WIN64))
        home = getenv("HOME");
#endif
        std::vector<char*> args(argc);
        args[0] = argv[0];

        std::vector<std::string> storage(argc);
        for (int i = 0; i < argc; i++)
        {
            storage[i] = std::regex_replace(std::string(argv[i]), std::regex("HOME"), home);
            args[i] = const_cast<char*>(storage[i].c_str());
        }

        return gauze_main(argc, &args.front());
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception";
    }

    return 0;
}

// utility

static void chooseBest(std::vector<cv::Rect>& objects, std::vector<double>& scores)
{
    if (objects.size() > 1)
    {
        int best = 0;
        for (int i = 1; i < objects.size(); i++)
        {
            if (scores[i] > scores[best])
            {
                best = i;
            }
        }
        objects = { objects[best] };
        scores = { scores[best] };
    }
}

static bool writeAsJson(const std::string& filename, const std::vector<cv::Rect>& objects)
{
    std::ofstream ofs(filename);
    if (ofs)
    {
        cereal::JSONOutputArchive oa(ofs);
        using Archive = decltype(oa); // needed by macro
        oa << GENERIC_NVP("objects", objects);
    }
    return ofs.good();
}

static bool writeAsText(const std::string& filename, const std::vector<cv::Rect>& objects)
{
    std::ofstream ofs(filename);
    if (ofs)
    {
        ofs << objects.size() << " ";
        for (auto& o : objects)
        {
            ofs << o.x << " " << o.y << " " << o.width << " " << o.height << " ";
        }
    }
    return ofs.good();
}

static void drawObjects(cv::Mat& canvas, const std::vector<cv::Rect>& objects)
{
    for (const auto& o : objects)
    {
        cv::rectangle(canvas, o, { 0, 255, 0 }, 2, 8);
    }
}

static cv::Rect2f operator*(const cv::Rect2f& roi, float scale)
{
    return { roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale };
}

static void randomEllipse(cv::Mat& image, int n)
{
    for (int i = 0; i < n; i++)
    {
        const cv::Point2f center(rand() % image.cols, rand() % image.rows);
        const cv::Size2f size(rand() % image.cols, rand() % image.rows);
        const cv::RotatedRect ellipse(center, size, static_cast<float>(rand() % 1000) / 1000.f * M_PI);
        const cv::Scalar bgr(rand() % 255, rand() % 255, rand() % 255);
        cv::ellipse(image, ellipse, bgr, -1);
    }
}

static void randomRectangle(cv::Mat& image, int n)
{
    for (int i = 0; i < n; i++)
    {
        const cv::Point p1(rand() % image.cols, rand() % image.rows);
        const cv::Point p2(rand() % image.cols, rand() % image.rows);

        if ((rand() % 8) > 4)
        {
            cv::randu(image(cv::Rect(p1, p2)), cv::Scalar::all(0), cv::Scalar::all(255));
        }
        else
        {
            const cv::Scalar bgr(rand() % 255, rand() % 255, rand() % 255);
            cv::rectangle(image, p1, p2, bgr, -1);
        }
    }
}

static void randomLines(cv::Mat& image, int n)
{
    for (int i = 0; i < n; i++)
    {
        const cv::Point u1(rand() % image.cols, rand() % image.rows);
        const cv::Point u2(rand() % image.cols, rand() % image.rows);
        const cv::Scalar bgr(rand() % 255, rand() % 255, rand() % 255);
        cv::line(image, u1, u2, bgr, (rand() % 16) + 1, 8);
    }
}

// Provide a simple mechanism for testing the ACF pyramids (GPU and CPU)
// without the need for reading actual images.  This was added initially
// to aid testing on mobile devices.
static void randomShapes(cv::Mat& image, int n)
{
    for (int i = 0; i < n; i++)
    {
        switch (rand() % 3)
        {
            case 0:
                randomLines(image, 1);
            case 1:
                randomRectangle(image, 1);
            case 2:
                randomEllipse(image, 1);
        }
    }
}
