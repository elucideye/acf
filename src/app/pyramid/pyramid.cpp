/*! -*-c++-*-
 @file   pyramid.cpp
 @author David Hirvonen
 @brief  Create an ACF pyramid from the command line.
 
 \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
 \license{This project is released under the 3 Clause BSD License.}
 
 */

#include <acf/ACF.h>
#include <acf/MatP.h>
#include <common/string_utils.h>
#include <common/Logger.h>

#include <opencv2/core/cvstd.inl.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cxxopts.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <iomanip>

static cv::Mat load_as_float(const std::string& filename)
{
    cv::Mat I8u = cv::imread(filename, cv::IMREAD_ANYCOLOR), I32f;

    if (I8u.empty())
    {
        throw std::runtime_error("load_as_float: empty input");
    }

    switch (I8u.channels())
    {
        case 4:
            cv::cvtColor(I8u, I8u, cv::COLOR_BGRA2RGB);   // toolbox expects RGB
            I8u.convertTo(I32f, CV_32FC3, 1.0f / 255.0f); // ... and CV_32FC3
        case 3:
            cv::cvtColor(I8u, I8u, cv::COLOR_BGR2RGB);    // toolbox expects RGB
            I8u.convertTo(I32f, CV_32FC3, 1.0f / 255.0f); // ... and CV_32FC3
            break;
        case 1:
            I8u.convertTo(I32f, CV_32FC1, 1.0f / 255.0f); // allow grayscale input
            break;
        default:
            throw std::runtime_error("load_as_float: unsupported channels");
    }
    return I32f;
}

template <typename T>
struct FixedNum
{
    FixedNum(const T& value, int width) : value(value), width(width) {}
    T value;
    int width = 4;
};

template <typename T>
std::ostream & operator <<(std::ostream &os, const FixedNum<T> &num)
{
    os << std::setfill('0') << std::setw(num.width) << num.value;
    return os;
}

int gauze_main(int argc, char** argv)
{
    const auto argumentCount = argc;

    // Instantiate line logger:
    auto logger = util::Logger::create("acf-pyramid");

    // ############################
    // ### Command line parsing ###
    // ############################

    std::string sInput, sOutput, colorspace;

    cxxopts::Options options("acf-pyramid", "Create ACF pyramids on the command line");

    // clang-format off
    options.add_options()
        ("i,input", "Input file", cxxopts::value<std::string>(sInput))
        ("o,output", "Output directory", cxxopts::value<std::string>(sOutput))
        ("colorspace", "Specify the colorspace: gray, luv", cxxopts::value<std::string>(colorspace))
        ("h,help", "Print help message");
    // clang-format on

    auto cli = options.parse(argc, argv);

    if ((argumentCount <= 1) || cli.count("help"))
    {
        logger->info("{}", options.help({ "" }));
        return 0;
    }

    if (sInput.empty())
    {
        logger->error("Must specify input image file");
        return 1;
    }

    if (sOutput.empty())
    {
        logger->error("Must specify output directory");
    }

    std::string base = sOutput + "/" + util::basename(sInput);

    acf::Detector acf;
    acf::Detector::Pyramid pyramid;
    acf.chnsPyramid({}, nullptr, pyramid, true, {}); // get defaults, i.e.: pPyramid=chnsPyramid();

    cv::Mat I32f = load_as_float(sInput);

    // Create a TRANSPOSE + planar format image for compatibility with the
    // original MATLAB (column major) SIMD code.
    MatP Ip(I32f.t());

    // Request grayscale colorspace using the following:
    if (!colorspace.empty())
    {
        pyramid.pPyramid.pChns->pColor->colorSpace = { "colorspace", colorspace };
    }

    acf.chnsPyramid(Ip, &pyramid.pPyramid, pyramid, true, {}); // compute the pyramid

    int i = 0, j = 0, k = 0;
    for (auto iter = pyramid.data.begin(); iter != pyramid.data.end(); iter++, i++)
    {
        for (auto jter = iter->begin(); jter != iter->end(); jter++, j++)
        {
            for (auto kter = jter->begin(); kter != jter->end(); kter++, k++)
            {
                cv::Mat I8uc1;
                kter->convertTo(I8uc1, CV_8UC1, 255.0);

                // Undo the TRANPOSE for each channel before writing to disk
                // for visualization in row major format
                I8uc1 = I8uc1.t();

                std::stringstream ss;
                ss << base << "_" << FixedNum<int>(i,4) << "_" << FixedNum<int>(j,4) << "_" << FixedNum<int>(k,4) << ".png";
                cv::imwrite(ss.str(), I8uc1);
            }
        }
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
        std::cerr << e.what() << std::endl;
    }
}
