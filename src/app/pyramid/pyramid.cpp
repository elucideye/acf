/*! -*-c++-*-
 @file   pyramid.cpp
 @author David Hirvonen
 @brief  Create an ACF pyramid from the command line.
 
 \copyright Copyright 2018 Elucideye, Inc. All rights reserved.
 \license{This project is released under the 3 Clause BSD License.}
 
 */

#include <acf/ACF.h>
#include "util/Logger.h"
#include "util/string_utils.h"

#include <opencv2/highgui.hpp>

#include <cxxopts.hpp>

#include <iostream>
#include <string>
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

int gauze_main(int argc, char** argv)
{
    const auto argumentCount = argc;

    // Instantiate line logger:
    auto logger = util::Logger::create("acf-pyramid");

    // ############################
    // ### Command line parsing ###
    // ############################

    std::string sInput, sOutput;

    cxxopts::Options options("acf-pyramid", "Create ACF pyramids on the command line");

    // clang-format off
    options.add_options()
        ("i,input", "Input file", cxxopts::value<std::string>(sInput))
        ("o,output", "Output directory", cxxopts::value<std::string>(sOutput))
        ("h,help", "Print help message");
    // clang-format on

    options.parse(argc, argv);

    if ((argumentCount <= 1) || options.count("help"))
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
    //pyramid.pPyramid.pChns->pColor->colorSpace = { "colorspace", "gray" };

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
                ss << base << std::setw(4) << std::setfill('0') << i << "_" << j << "_" << k << ".png";
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
