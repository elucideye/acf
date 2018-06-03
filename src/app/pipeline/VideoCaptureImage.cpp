#include "VideoCaptureImage.h"

#include <utility>

VideoCaptureImage::VideoCaptureImage(cv::Mat  image, int frames)
    : image(std::move(image))
    , frames(frames)
{
}

VideoCaptureImage::VideoCaptureImage(const std::string& filename, int frames)
    : frames(frames)
{
    image = cv::imread(filename, cv::IMREAD_COLOR);
}

VideoCaptureImage::~VideoCaptureImage() = default;

void VideoCaptureImage::setRepeat(int n)
{
    frames = n;
}

bool VideoCaptureImage::grab()
{
    return false;
}

bool VideoCaptureImage::isOpened() const
{
    return !image.empty();
}

void VideoCaptureImage::release()
{
    image.release();
}

bool VideoCaptureImage::open(const cv::String& filename)
{
    image = cv::imread(filename);
    return !image.empty();
}

bool VideoCaptureImage::read(cv::OutputArray image)
{
    if (++index <= frames)
    {
        image.assign(this->image);
        return true;
    }
    return false;
}

double VideoCaptureImage::get(int propId) const
{
    switch (propId)
    {
        case CV_CAP_PROP_FRAME_WIDTH:
            return static_cast<double>(image.cols);
        case CV_CAP_PROP_FRAME_HEIGHT:
            return static_cast<double>(image.rows);
        case CV_CAP_PROP_FRAME_COUNT:
            return static_cast<double>(frames);
        default:
            return 0.0;
    }
}
