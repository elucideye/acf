/*! -*-c++-*-
  @file   chnsPyramid.cpp
  @author David Hirvonen
  @author P. Dollár (original matlab code)
  @brief  Computation of an Aggregated Channel Features pyramid (no necessarily powers of two).

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

// Compute channel feature pyramid given an input image.
//
// While chnsCompute() computes channel features at a single scale,
// chnsPyramid() calls chnsCompute() multiple times on different scale
// images to create a scale-space pyramid of channel features.
//
// In its simplest form, chnsPyramid() first creates an image pyramid, then
// calls chnsCompute() with the specified "pChns" on each scale of the image
// pyramid. The parameter "nPerOct" determines the number of scales per
// octave in the image pyramid (an octave is the set of scales up to half of
// the initial scale), a typical value is nPerOct=8 in which case each scale
// in the pyramid is 2^(-1/8)~=.917 times the size of the previous. The
// smallest scale of the pyramid is determined by "minDs", once either image
// dimension in the resized image falls below minDs, pyramid creation stops.
// The largest scale in the pyramid is determined by "nOctUp" which
// determines the number of octaves to compute above the original scale.
//
// While calling chnsCompute() on each image scale works, it is unnecessary.
// For a broad family of features, including gradient histograms and all
// channel types tested, the feature responses computed at a single scale
// can be used to approximate feature responses at nearby scales. The
// approximation is accurate at least within an entire scale octave. For
// details and to understand why this unexpected result holds, please see:
//   P. Dollár, R. Appel, S. Belongie and P. Perona
//   "Fast Feature Pyramids for Object Detection", PAMI 2014.
//
// The parameter "nApprox" determines how many intermediate scales are
// approximated using the techniques described in the above paper. Roughly
// speaking, channels at approximated scales are computed by taking the
// corresponding channel at the nearest true scale (computed w chnsCompute)
// and resampling and re-normalizing it appropriately. For example, if
// nPerOct=8 and nApprox=7, then the 7 intermediate scales are approximated
// and only power of two scales are actually computed (using chnsCompute).
// The parameter "lambdas" determines how the channels are normalized (see
// the above paper). lambdas for a given set of channels can be computed
// using chnsScaling.m, alternatively, if no lambdas are specified, the
// lambdas are automatically approximated using two true image scales.
//
// Typically approximating all scales within an octave (by setting
// nApprox=nPerOct-1 or nApprox=-1) works well, and results in large speed
// gains (~4x). See example below for a visualization of the pyramid
// computed with and without the approximation. While there is a slight
// difference in the channels, during detection the approximated channels
// have been shown to be essentially as effective as the original channels.
//
// While every effort is made to space the image scales evenly, this is not
// always possible. For example, given a 101x100 image, it is impossible to
// downsample it by exactly 1/2 along the first dimension, moreover, the
// exact scaling along the two dimensions will differ. Instead, the scales
// are tweaked slightly (e.g. for a 101x101 image the scale would go from
// 1/2 to something like 50/101), and the output contains the exact scaling
// factors used for both the heights and the widths ("scaleshw") and also
// the approximate scale for both dimensions ("scales"). If "shrink">1 the
// scales are further tweaked so that the resized image has dimensions that
// are exactly divisible by shrink (for details please see the code).
//
// If chnsPyramid() is called with no inputs, the output is the complete
// default parameters (pPyramid). Finally, we describe the remaining
// parameters: "pad" controls the amount the channels are padded after being
// created (useful for detecting objects near boundaries); "smooth" controls
// the amount of smoothing after the channels are created (and controls the
// integration scale of the channels); finally "concat" determines whether
// all channels at a single scale are concatenated in the output.
//
// An emphasis has been placed on speed, with the code undergoing heavy
// optimization. Computing the full set of (approximated) *multi-scale*
// channels on a 480x640 image runs over *30 fps* on a single core of a
// machine from 2011 (although runtime depends on input parameters).
//
// USAGE
//  pPyramid = chnsPyramid()
//  pyramid = chnsPyramid( I, pPyramid )
//
// INPUTS
//  I            - [hxwx3] input image (uint8 or single/double in [0,1])
//  pPyramid     - parameters (struct or name/value pairs)
//   .pChns        - parameters for creating channels (see chnsCompute.m)
//   .nPerOct      - [8] number of scales per octave
//   .nOctUp       - [0] number of upsampled octaves to compute
//   .nApprox      - [-1] number of approx. scales (if -1 nApprox=nPerOct-1)
//   .lambdas      - [] coefficients for power law scaling (see BMVC10)
//   .pad          - [0 0] amount to pad channels (along T/B and L/R)
//   .minDs        - [16 16] minimum image size for channel computation
//   .smooth       - [1] radius for channel smoothing (using convTri)
//   .concat       - [1] if true concatenate channels
//   .complete     - [] if true does not check/set default vals in pPyramid
//
// OUTPUTS
//  pyramid      - output struct
//   .pPyramid     - exact input parameters used (may change from input)
//   .nTypes       - number of channel types
//   .nScales      - number of scales computed
//   .data         - [nScales x nTypes] cell array of computed channels
//   .info         - [nTypes x 1] struct array (mirrored from chnsCompute)
//   .lambdas      - [nTypes x 1] scaling coefficients actually used
//   .scales       - [nScales x 1] relative scales (approximate)
//   .scaleshw     - [nScales x 2] exact scales for resampling h and w
//
// EXAMPLE
//  I=imResample(imread('peppers.png'),[480 640]);
//  pPyramid=chnsPyramid(); pPyramid.minDs=[128 128];
//  pPyramid.nApprox=0; tic, P1=chnsPyramid(I,pPyramid); toc
//  pPyramid.nApprox=7; tic, P2=chnsPyramid(I,pPyramid); toc
//  figure(1); montage2(P1.data{2}); figure(2); montage2(P2.data{2});
//  figure(3); montage2(abs(P1.data{2}-P2.data{2})); colorbar;
//
// See also chnsCompute, chnsScaling, convTri, imPad
//
// Piotr's Image&Video Toolbox      Version 3.25
// Copyright 2013 Piotr Dollar & Ron Appel.  [pdollar-at-caltech.edu]
// Please email me if you find bugs, or have suggestions or questions!
// Licensed under the Simplified BSD License [see external/bsd.txt]

#include <acf/ACF.h>

#include <util/Parallel.h>
#include <util/acf_math.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <random>

ACF_NAMESPACE_BEGIN

template <typename T>
cv::Size round(const cv::Size_<T>& size)
{
    return cv::Size_<T>(util::round(size.width), util::round(size.height));
}

/*
 * chnsPyramid() aims to adhere to the above specification as much as possible
 * given a strongly typed API.  The toolbox Matlab code uses an empty parameter
 * list as a cue to populate a default parameter list for the Pyramid options
 * which is passed on as the functions return type.  This API includes an output
 * Pyramid reference type, so we populate that object's Pyramid::pPyramid field 
 * with the default options in this case.
 *
 * We use the following logic to indicate "empty" input parameters
 *   
 *  (const MatP&) IIn.empty()
 *  (const Options::Pyramid*) p == nullptr
 */

int Detector::chnsPyramid
(
    const MatP& Iin,
    const Options::Pyramid* pIn,
    Pyramid& pyramid,
    bool isInit,
    const MatLoggerType& pLogger
)
{
    // % get default parameters pPyramid
    //if(nargin==2), p=varargin{1}; else p=[]; end
    Options::Pyramid p;
    if (isInit && pIn)
    {
        p = *pIn;
    }

    if (!p.complete.has || (p.complete != 1) || Iin.empty())
    {
        // 'pChns',{},,'nOctUp',0,'nApprox',-1,'lambdas',[],'pad',[0 0], ...
        // 'minDs',[16 16],'smooth',1,'concat',1,'complete',1};
        Options::Pyramid dfs;
        dfs.nPerOct = { "nPerOct", 8 };
        dfs.nOctUp = { "nOctUp", 0 };
        dfs.nApprox = { "nApprox", -1 };
        dfs.pad = { "pad", cv::Size(0, 0) };
        dfs.minDs = { "minDs", cv::Size(16, 16) };
        dfs.smooth = { "smooth", 1 };
        dfs.concat = { "concat", 1 };
        dfs.complete = { "complete", 1 };
        p.merge(dfs, 1);

        Detector::Channels chns;
        chnsCompute({}, p.pChns.get(), chns, false, pLogger);

        p.pChns = chns.pChns;
        p.pChns.get().complete = 1;
        int shrink = p.pChns->shrink.get();
        cv::Size_<double> pad = p.pad.get(), minDs = p.minDs.get();
        p.pad.get() = round(pad / double(shrink)) * shrink;
        p.minDs.get() = cv::Size(std::max(minDs.width, double(shrink * 4.0)), std::max(minDs.height, double(shrink * 4.0)));
        if (p.nApprox < 0)
        {
            p.nApprox = p.nPerOct - 1;
        }
    }

    if (Iin.empty() && !pIn && isInit)
    {
        pyramid.pPyramid = p;
        return 0;
    }

    // pPyramid=p;
    // vs=struct2cell(p);
    // [pChns,nPerOct,nOctUp,nApprox,lambdas,pad,minDs,smooth,concat,~]=deal(vs{:});
    auto pPyramid = p;
    auto pChns = p.pChns.get();
    auto nPerOct = p.nPerOct.get();
    auto nOctUp = p.nOctUp.get();
    auto nApprox = p.nApprox.get();
    auto lambdas = p.lambdas.get();
    auto pad = p.pad.get();
    auto minDs = p.minDs.get();
    auto smooth = p.smooth.get();
    auto concat = p.concat.get();
    auto shrink = pChns.shrink.get();

    pChns.isLuv = m_isLuv; // propagate LUV special case through to static function

    // Convert I to appropriate color space (or simply normalize):
    const std::string& cs = pChns.pColor->colorSpace;
    cv::Size sz = Iin.size();
    MatP I, pI, MO;
    if (sz.area() && Iin.channels() == 1 && (cs == "gray" || cs == "orig"))
    {
        // NOTE: Here we replicate the original Matlab behavior of repeating
        // the grayscale in 3 channels and then converting that to grayscale
        // later treating it as a color image, even though this will produce
        // a different single channel image in rgbConvert()
        //
        // I=I(:,:,[1 1 1]); warning('Converting image to color');
        pI.create(Iin.size(), Iin.depth(), 3);
        cv::repeat(Iin[0], 3, 1, pI.base());
    }
    else
    {
        pI = Iin; // shallow copy
        if (Iin.channels() > 3)
        {
            std::copy(pI.begin() + 3, pI.end(), std::back_inserter(MO));
            while (pI.channels() > 3)
            {
                pI.pop_back();
            }
        }
    }

    if (pI.channels())
    {
        rgbConvert(pI, I, cs, true, m_isLuv);
    }

    pChns.pColor->colorSpace = std::string("orig");

    auto& info = pyramid.info;
    auto& scales = pyramid.scales;
    auto& scaleshw = pyramid.scaleshw;

    // Get scales at which to compute features and list of real/approx scales:
    getScales(nPerOct, nOctUp, minDs, shrink, sz, scales, scaleshw);

    auto nScales = static_cast<int>(scales.size());
    std::vector<int> isR, isA, isN(nScales, 0), *isRA[2] = { &isR, &isA };
    for (int i = 0; i < nScales; i++)
    {
        isRA[(i % (nApprox + 1)) > 0]->push_back(i + 1);
    }

    std::vector<int> isH((isR.size() + 1), 0);
    isH.back() = nScales;
    for (int i = 0; i < std::max(int(isR.size()) - 1, 0); i++)
    {
        isH[i + 1] = (isR[i] + isR[i + 1]) / 2;
    }

    for (int i = 0; i < isR.size(); i++)
    {
        for (int j = isH[i]; j < isH[i + 1]; j++)
        {
            isN[j] = isR[i];
        }
    }

    // Compute image pyramid [real scales]
    int nTypes = 0;
    auto& data = pyramid.data;
    for (const auto& i : isR)
    {
        double s = scales[i - 1];
        cv::Size sz1 = round((cv::Size2d(sz) * s) / double(shrink)) * shrink;

        MatP I1;
        if (sz == sz1)
        {
            I1 = I;
        }
        else
        {
            // TODO: use imResampleMex to resave remap coefficients
            imResample(I, I1, sz1, 1.0);
        }

        if ((s == 0.5) && ((nApprox > 0) || (nPerOct == 1)))
        {
            I = I1;
        }

        if ((i == isR.front()) && (MO.channels() == 2))
        {
            I1.push_back(MO[0]);
            I1.push_back(MO[1]);
        }

        Detector::Channels chns;
        chnsCompute(I1, pChns, chns, false, pLogger);
        info = chns.info;
        if (i == isR.front()) // on first iteration allocate data
        {
            nTypes = chns.nTypes;
            data.resize(nScales);
            for (auto& s : data)
            {
                s.resize(nTypes);
            }
        }

        std::copy(chns.data.begin(), chns.data.end(), data[i - 1].begin());
    }

    // If lambdas not specified compute image specific lambdas:
    if (nScales > 0 && nApprox > 0 && !lambdas.size())
    {
        std::vector<int> is;
        for (int i = (1 + nOctUp * nPerOct); i <= nScales; i += (nApprox + 1))
        {
            is.push_back(i - 1);
        }

        CV_Assert(is.size() >= 2);

        if (is.size() > 2)
        {
            is = { is[1], is[2] };
        }

        std::vector<double> f0(nTypes, 0.0), f1 = f0;
        for (int j = 0; j < nTypes; j++)
        {
            f0[j] = sum(data[is[0]][j]) / double(numel(data[is[0]][j]));
            CV_Assert(!std::isnan(f0[j]));
        }

        for (int j = 0; j < nTypes; j++)
        {
            f1[j] = sum(data[is[1]][j]) / double(numel(data[is[1]][j]));
            CV_Assert(!std::isnan(f1[j]));
        }

        lambdas.resize(nTypes);
        for (int j = 0; j < nTypes; j++)
        {
            lambdas[j] = -util::log2(f0[j] / f1[j]) / util::log2(scales[is[0]] / scales[is[1]]);
        }
    }

    // The per scale/type operations are easily parallelized, but with a parallel_for approach
    // using simple uniform slicing will tend to starve some threads due to the nature of the
    // pyramid layout.  Randomizing the scale indices should do better.  More optimal strategies
    // may exist with further testing (work stealing, etc).
    const auto scalesIndex = util::create_random_indices(nScales);

    auto isAIndex = isA;
    std::shuffle(isAIndex.begin(), isAIndex.end(), std::mt19937(std::random_device()()));

    cv::parallel_for_({ 0, int(isAIndex.size()) }, [&](const cv::Range& r) {
        for (int k = r.start; k < r.end; k++)
        {
            const int i = isAIndex[k];
            const int iR = isN[i - 1];
            const cv::Size sz1 = round(cv::Size2d(sz) * scales[i - 1] / double(shrink));
            for (int j = 0; j < nTypes; j++)
            {
                double ratio = std::pow(scales[i - 1] / scales[iR - 1], -lambdas[j]);
                imResample(data[iR - 1][j], data[i - 1][j], sz1, ratio);
            }
        }
    });

    cv::parallel_for_({ 0, int(scales.size()) }, [&](const cv::Range& r) {
        for (int i = r.start; i < r.end; i++)
        {
            for (int j = 0; j < nTypes; j++)
            {
                convTri(data[scalesIndex[i]][j], data[scalesIndex[i]][j], smooth, 1);
            }
        }
    });

    // TODO: test imPad
    if (pad.width || pad.height)
    {
        cv::parallel_for_({ 0, int(scales.size()) }, [&](const cv::Range& r) {
            for (int i = r.start; i < r.end; i++)
            {
                for (int j = 0; j < nTypes; j++)
                {
                    int y = pad.height / shrink;
                    int x = pad.width / shrink;
                    auto& I = data[scalesIndex[i]][j];
                    copyMakeBorder(I, I, y, y, x, x, cv::BORDER_REFLECT);
                }
            }
        });
    }

    if (concat && nTypes)
    {
        auto data0 = data;
        data.resize(nScales);
        for (int i = 0; i < nScales; i++)
        {
            data[i].resize(1);
            fuseChannels(data0[i].begin(), data0[i].end(), data[i][0]);
        }
    }

    pyramid.pPyramid = pPyramid;
    pyramid.nTypes = nTypes;
    pyramid.nScales = nScales;
    pyramid.lambdas = lambdas;

#define DO_DEBUG_CONCATENATED_FEATURES 0
#if DO_DEBUG_CONCATENATED_FEATURES
    for (int i = 0; i < nScales; i++)
    {
        for (auto& p : data[i][0])
        {
            cv::Mat tmp;
            cv::normalize(p, tmp, 0, 1, cv::NORM_MINMAX, CV_32FC1);
            cv::imshow("tmp", tmp), cv::waitKey(0);
        }
    }
#endif

    return 0;
}

using DoubleVec = std::vector<double>;
using Size2dVec = std::vector<cv::Size2d>;

int Detector::getScales
(
    int nPerOct,
    int nOctUp,
    const cv::Size& minDs,
    int shrink,
    const cv::Size& sz,
    DoubleVec& scales,
    Size2dVec& scaleshw
)
{
    // set each scale s such that max(abs(round(sz*s/shrink)*shrink-sz*s)) is
    // minimized without changing the smaller dim of sz (tricky algebra)
    scales = {};
    scaleshw = {};
    if (!sz.area())
    {
        return 0;
    }

    cv::Size2d ratio(double(sz.width) / double(minDs.width), double(sz.height) / double(minDs.height));
    int nScales = std::floor(double(nPerOct) * (double(nOctUp) + util::log2(std::min(ratio.width, ratio.height))) + 1.0);

    double d0 = sz.height, d1 = sz.width;
    if (sz.height >= sz.width)
    {
        std::swap(d0, d1);
    }

    for (int i = 0; i < nScales; i++)
    {
        double s = std::pow(2.0, -double(i) / double(nPerOct) + double(nOctUp));
        double s0 = (util::round(d0 * s / shrink) * shrink - 0.25 * shrink) / d0;
        double s1 = (util::round(d0 * s / shrink) * shrink + 0.25 * shrink) / d0;
        std::pair<double, double> best(0, std::numeric_limits<double>::max());
        for (double j = 0.0; j < 1.0 - std::numeric_limits<double>::epsilon(); j += 0.01)
        {
            double ss = (j * (s1 - s0) + s0);
            double es0 = d0 * ss;
            es0 = std::abs(es0 - util::round(es0 / shrink) * shrink);
            double es1 = d1 * ss;
            es1 = std::abs(es1 - util::round(es1 / shrink) * shrink);
            double es = std::max(es0, es1);
            if (es < best.second)
            {
                best = { ss, es };
            }
        }
        scales.push_back(best.first);
    }

    auto tmp = scales;
    tmp.push_back(0);
    scales.clear();
    for (int i = 1; i < tmp.size(); i++)
    {
        if (tmp[i] != tmp[i - 1])
        {
            double s = tmp[i - 1];
            scales.push_back(s);

            double x = util::round(double(sz.width) * s / shrink) * shrink / sz.width;
            double y = util::round(double(sz.height) * s / shrink) * shrink / sz.height;
            scaleshw.emplace_back(x, y);
        }
    }

    return 0;
}

ACF_NAMESPACE_END
