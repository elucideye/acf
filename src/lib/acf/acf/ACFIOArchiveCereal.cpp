// clang-format off
#if defined(ACF_ADD_TO_STRING)
#  include <io/stdlib_string.h>
#endif
// clang-format on
#include <io/cereal_pba.h>
#include <io/cvmat_cereal.h>
#include <acf/ACFIOArchive.h>

#include <opencv2/opencv.hpp>

CEREAL_CLASS_VERSION(acf::Detector, 1);

ACF_NAMESPACE_BEGIN

// ##################################################################
// #################### PortableBinary[IO]Archive ###################
// ##################################################################

using OArchive = cereal::PortableBinaryOutputArchive;
template ACF_EXPORT void Detector::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Boost::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Boost::Tree::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Jitter::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Pyramid::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Nms::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::Color::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::GradMag::serialize<OArchive>(OArchive& ar, const std::uint32_t);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::GradHist::serialize<OArchive>(OArchive& ar, const std::uint32_t);

using IArchive = cereal::PortableBinaryInputArchive;
template ACF_EXPORT void Detector::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Boost::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Boost::Tree::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Jitter::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Pyramid::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Nms::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::Color::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::GradMag::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
template ACF_EXPORT void Detector::Options::Pyramid::Chns::GradHist::serialize<IArchive>(IArchive& ar, const std::uint32_t version);
ACF_NAMESPACE_END
