/*! -*-c++-*-
  @file   acf.cpp
  @author David Hirvonen
  @brief  Aggregated channel feature computation and object detection.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

// Local includes:
#include "io/stdlib_string.h" // android workaround

#include "acf/ACF.h"

#include "util/LazyParallelResource.h"
#include "util/Line.h"
#include "util/Logger.h"
#include "util/Parallel.h"
#include "util/make_unique.h"
#include "util/string_utils.h"
#include "util/cli.h"
#include "io/cv_cereal.h"
#include "io/cereal_pba.h" // optional

// Package includes:
#include "cxxopts.hpp"

#include <opencv2/highgui.hpp>

#include <cereal/types/map.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/json.hpp>

#include <type_traits>

using AcfPtr = std::unique_ptr<acf::Detector>;
using RectVec = std::vector<cv::Rect>;

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

    virtual Frame operator()(int i)
    {
        Frame frame;
        frame.name = filenames[i];
        frame.image = cv::imread(filenames[i], cv::IMREAD_COLOR);
        return frame;
    }

    virtual bool isRandomAccess() const
    {
        return true;
    }

    virtual std::size_t size()
    {
        return filenames.size();
    }

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
    auto logger = util::Logger::create("drishti-acf");

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
    double cascCal = 0.0;
    int minWidth = -1; // minimum object width
    int maxWidth = -1; // maximum object width TODO

    cxxopts::Options options("drishti-acf", "Command line interface for ACF object detection (see Piotr's toolbox)");

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

    if (util::cli::directory::exists(sOutput, ".drishti-acf"))
    {
        std::string filename = sOutput + "/.drishti-acf";
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

    std::shared_ptr<VideoSource> video = std::make_shared<VideoSource>(sInput); // list of files

    // Allocate resource manager:
    util::LazyParallelResource<std::thread::id, AcfPtr> manager = [&]() {
        AcfPtr acf = util::make_unique<acf::Detector>(sModel);
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
            acf.release();
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
        const auto winSize = detector->getWindowSize();

        // Load current image
        auto frame = (*video)(i);
        auto& image = frame.image;

        //cv::imwrite("/tmp/i.png", image);

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
    if (threads == 1 || threads == 0 || doWindow || !video->isRandomAccess())
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
        return gauze_main(argc, argv);
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
        typedef decltype(oa) Archive; // needed by macro
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
