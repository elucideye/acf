/*******************************************************************************
* Piotr's Image&Video Toolbox      Version 3.21
* Copyright 2013 Piotr Dollar.  [pdollar-at-caltech.edu]
* Please email me if you find bugs, or have suggestions or questions!
* Licensed under the Simplified BSD License [see external/bsd.txt]
*******************************************************************************/

#include <acf/ACF.h>
#include <acf/MatP.h>
#include <acf/acf_common.h>

#include <opencv2/core/base.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>

#include <cstdint>
#include <assert.h>

using namespace std;

using uint32 = unsigned int;

ACF_NAMESPACE_BEGIN

/*
 * These are computed in row major order:
 */

#define GPU_ACF_TRANSPOSE 1 // 1 = compatibility with matlab column major training

using RectVec = std::vector<cv::Rect>;
using UInt32Vec = std::vector<uint32_t>;
static UInt32Vec computeChannelIndex(const RectVec& rois, uint32 rowStride, int modelWd, int modelHt, int width, int height);
static UInt32Vec computeChannelIndexColMajor(int nChns, int modelWd, int modelHt, int width, int height);

class DetectionSink
{
public:
    virtual void add(const cv::Point& p, float value)
    {
        hits.emplace_back(p, value);
    }
    std::vector<std::pair<cv::Point, float>> hits;
};

class DetectionParams : public cv::ParallelLoopBody
{
public:
    cv::Size winSize; // possibly transposed
    cv::Size size1;
    cv::Point step1;
    int stride{};
    int shrink{};
    int rowStride{};
    std::vector<uint32_t> cids;
    const uint32* fids{};
    const float* hs = nullptr;
    int nTrees{};
    int nTreeNodes{};
    float cascThr{};
    const uint32_t* child = nullptr;

    MatP I;
    cv::Mat canvas;

    virtual float evaluate(uint32_t row, uint32_t col) const = 0;
};

template <class T, int kDepth>
class ParallelDetectionBody : public DetectionParams
{
public:
    ParallelDetectionBody(const T* chns, const T* thrs, DetectionSink* sink)
        : chns(chns)
        , thrs(thrs)
        , sink(sink)
    {
        CV_Assert(thrs);
    }

    void operator()(const cv::Range& range) const override
    {
        for (int c = 0; c < size1.width; c += step1.x)
        {
            for (int r = 0; r < size1.height; r += step1.y)
            {
                int offset = (r * stride / shrink) + (c * stride / shrink) * rowStride;
                float h = evaluate(chns + offset);
                if (h > cascThr)
                {
                    sink->add({ c, r }, h);
                }
            }
        }
    }

    void getChild(const T* chns1, uint32 offset, uint32& k0, uint32& k) const
    {
        int index = cids[fids[k]];
        float ftr = chns1[index];
        k = (ftr < thrs[k]) ? 1 : 2;
        k0 = k += k0 * 2;
        k += offset;
    }

    void traverse(const T* chns1, uint32_t offset, uint32_t& k0, uint32_t& k) const
    {
        for (int i = 0; i < kDepth; i++)
        {
            getChild(chns1, offset, k0, k);
        }
    }

    float evaluate(uint32_t row, uint32_t col) const override
    {
        int offset = (row * stride / shrink) + (col * stride / shrink) * rowStride;
        return evaluate(chns + offset);
    }

    float evaluate(const T* chns1) const
    {
        float h = 0.f;
        uint32_t isZero = (kDepth == 0);
        for (int t = 0; t < nTrees; t++)
        {
            uint32 offset = t * nTreeNodes, k = offset, k0 = (k * isZero);
            traverse(chns1, offset, k0, k);
            h += hs[k];
            if (h <= cascThr)
            {
                break;
            }
        }
        return h;
    }

    // Input params:
    const T* chns = nullptr;
    const T* thrs = nullptr;
    DetectionSink* sink = nullptr;
};

template <>
void ParallelDetectionBody<float, 0>::traverse(const float* chns1, uint32_t offset, uint32_t& k0, uint32_t& k) const
{
    while (child[k])
    {
        float ftr = chns1[cids[fids[k]]];
        k = (ftr < thrs[k]) ? 1 : 0;
        k0 = k = child[k0] - k + offset;
    }
}

template <>
void ParallelDetectionBody<uint8_t, 0>::traverse(const uint8_t* chns1, uint32_t offset, uint32_t& k0, uint32_t& k) const
{
    while (child[k])
    {
        float ftr = chns1[cids[fids[k]]];
        k = (ftr < thrs[k]) ? 1 : 0;
        k0 = k = child[k0] - k + offset;
    }
}

const cv::Mat& Detector::Classifier::getScaledThresholds(int type) const
{
    switch (type)
    {
        case CV_8UC1:
            CV_Assert(!thrsU8.empty() && (thrsU8.type() == CV_8UC1));
            return thrsU8;
        case CV_32FC1:
            CV_Assert(!thrs.empty() && (thrs.type() == CV_32FC1));
            return thrs;
        default:
            CV_Assert(type == CV_32FC1 || type == CV_8UC1);
    }
    return thrs; // unused: for static analyzer
}

template <int kDepth>
std::shared_ptr<DetectionParams> allocDetector(const MatP& I, const cv::Mat& thrs, DetectionSink* sink)
{
    switch (I.depth())
    {
        case CV_8UC1:
            CV_Assert(thrs.type() == CV_8UC1);
            return std::make_shared<ParallelDetectionBody<uint8_t, kDepth>>(I[0].ptr<uint8_t>(), thrs.ptr<uint8_t>(), sink);
        case CV_32FC1:
            CV_Assert(thrs.type() == CV_32FC1);
            return std::make_shared<ParallelDetectionBody<float, kDepth>>(I[0].ptr<float>(), thrs.ptr<float>(), sink);
        default:
            CV_Assert(I.depth() == CV_8UC1 || I.depth() == CV_32FC1);
    }
    return nullptr; // unused: for static analyzer
}

std::shared_ptr<DetectionParams> allocDetector(const MatP& I, const cv::Mat& thrs, DetectionSink* sink, int depth)
{
    // Enforce compile time constants in inner tree search:
    switch (depth)
    {
        case 0:
            return allocDetector<0>(I, thrs, sink);
        case 1:
            return allocDetector<1>(I, thrs, sink);
        case 2:
            return allocDetector<2>(I, thrs, sink);
        case 3:
            return allocDetector<3>(I, thrs, sink);
        case 4:
            return allocDetector<4>(I, thrs, sink);
        case 5:
            return allocDetector<5>(I, thrs, sink);
        case 6:
            return allocDetector<6>(I, thrs, sink);
        case 7:
            return allocDetector<7>(I, thrs, sink);
        case 8:
            return allocDetector<8>(I, thrs, sink);
        default:
            CV_Assert(depth <= 8);
    }
    return nullptr;
}

// clang-format off
auto Detector::createDetector
(
    const MatP& I,
    const RectVec& rois,
    int shrink,
    cv::Size modelDsPad,
    int stride,
    DetectionSink* sink
)
// clang-format on
    const -> DetectionParamPtr
{
    int modelHt = modelDsPad.height;
    int modelWd = modelDsPad.width;

    cv::Size chnsSize = I.size();
    int height = chnsSize.height;
    int width = chnsSize.width;
    int nChns = I.channels();
    auto rowStride = static_cast<int>(I[0].step1());

    if (!m_isRowMajor)
    {
        std::swap(height, width);
        std::swap(modelHt, modelWd);
    }

    const auto height1 = static_cast<int>(ceil(float(height * shrink - modelHt + 1) / stride));
    const auto width1 = static_cast<int>(ceil(float(width * shrink - modelWd + 1) / stride));

    // Precompute channel offsets:
    std::vector<uint32_t> cids;
    if (rois.size())
    {
        cids = computeChannelIndex(rois, rowStride, modelWd / shrink, modelHt / shrink, width, height);
    }
    else
    {
        cids = computeChannelIndexColMajor(nChns, modelWd / shrink, modelHt / shrink, width, height);
    }

    // Extract relevant fields from trees
    // Note: Need tranpose for column-major storage
    auto& trees = clf;
    int nTreeNodes = trees.fids.rows;
    int nTrees = trees.fids.cols;
    std::swap(nTrees, nTreeNodes);
    cv::Mat thresholds = trees.getScaledThresholds(I.depth());

    CV_Assert(!thresholds.empty());
    CV_Assert(trees.treeDepth <= 8);
    std::shared_ptr<DetectionParams> detector = allocDetector(I, thresholds, sink, trees.treeDepth);

    // Scanning parameters
    detector->winSize = { modelWd, modelHt };
    detector->size1 = { width1, height1 };
    detector->step1 = { 1, 1 };
    detector->stride = stride;
    detector->shrink = shrink;
    detector->rowStride = rowStride;
    detector->cids = cids;

    // Tree parameters:
    detector->fids = trees.fids.ptr<uint32_t>();
    detector->nTrees = nTrees;
    detector->nTreeNodes = nTreeNodes;
    detector->hs = trees.hs.ptr<float>();
    detector->child = trees.child.ptr<uint32_t>();
    detector->I = I;

    return detector;
}

// Changelog:
//
// 3/21/2015: Rework arithmetic for row-major storage order

// clang-format off
void Detector::acfDetect1
(
    const MatP& I,
    const RectVec& rois,
    int shrink,
    const cv::Size& modelDsPad,
    int stride,
    double cascThr,
    std::vector<Detection>& objects
)
// clang-format on
{
    DetectionSink detections;
    auto detector = createDetector(I, rois, shrink, modelDsPad, stride, &detections);
    detector->cascThr = cascThr;
    (*detector)({ 0, detector->size1.width });

    for (const auto& hit : detections.hits)
    {
        cv::Rect roi({ hit.first.x * stride, hit.first.y * stride }, detector->winSize);
#if GPU_ACF_TRANSPOSE
        std::swap(roi.x, roi.y);
        std::swap(roi.width, roi.height);
#endif
        objects.emplace_back(roi, hit.second);
    }
}

float Detector::evaluate(const MatP& I, int shrink, const cv::Size& modelDsPad, int stride) const
{
    auto detector = createDetector(I, {}, shrink, modelDsPad, stride, nullptr);
    detector->cascThr = 0.f;
    return detector->evaluate(0, 0);
}

// local static utility routines:

static UInt32Vec computeChannelIndex(const RectVec& rois, uint32 rowStride, int modelWd, int modelHt, int width, int height)
{
#if GPU_ACF_TRANSPOSE
    assert(rois.size() > 1);
    auto nChns = static_cast<int>(rois.size());
    int chnStride = rois[1].x - rois[0].x;

    UInt32Vec cids(nChns * modelWd * modelHt);

    int m = 0;
    for (int z = 0; z < nChns; z++)
    {
        for (int c = 0; c < modelWd; c++)
        {
            for (int r = 0; r < modelHt; r++)
            {
                cids[m++] = z * chnStride + c * rowStride + r;
            }
        }
    }
    return cids;
#else

    assert(rois.size() > 1);
    int nChns = static_cast<int>(rois.size());
    int chnStride = rowStride * (rois[1].y - rois[0].y);

    UInt32Vec cids(nChns * modelWd * modelHt);

    int m = 0;
    for (int z = 0; z < nChns; z++)
    {
        for (int c = 0; c < modelWd; c++)
        {
            for (int r = 0; r < modelHt; r++)
            {
                cids[m++] = z * chnStride + r * rowStride + c;
            }
        }
    }
    return cids;
#endif
}

static UInt32Vec computeChannelIndexColMajor(int nChns, int modelWd, int modelHt, int width, int height)
{
    UInt32Vec cids(nChns * modelWd * modelHt);

    int m = 0, area = (width * height);
    for (int z = 0; z < nChns; z++)
    {
        for (int c = 0; c < modelWd; c++)
        {
            for (int r = 0; r < modelHt; r++)
            {
                cids[m++] = z * area + c * height + r;
            }
        }
    }
    return cids;
}

ACF_NAMESPACE_END
