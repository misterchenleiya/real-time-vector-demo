#include "core/processing/media_processor.h"

#include "core/model/geometry.h"

#include <QFileInfo>

#include <algorithm>
#include <cmath>

#if DEMO_HAS_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#endif

namespace demo {

#if DEMO_HAS_OPENCV

class MediaProcessor::Impl {
public:
    cv::VideoCapture capture;
    cv::Mat stillImage;
};

namespace {

bool hasImageExtension(const QString &path)
{
    static const QStringList imageExtensions = {
        QStringLiteral("png"),
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("bmp"),
        QStringLiteral("webp")
    };
    return imageExtensions.contains(QFileInfo(path).suffix().toLower());
}

QImage matToImage(const cv::Mat &bgr)
{
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
}

cv::Mat resizeToProcessingBounds(const cv::Mat &input, const ProcessingConfig &config)
{
    if (input.empty()) {
        return {};
    }

    const double scaleX = static_cast<double>(config.processingWidth) / static_cast<double>(input.cols);
    const double scaleY = static_cast<double>(config.processingHeight) / static_cast<double>(input.rows);
    const double scale = std::min(scaleX, scaleY);

    if (scale >= 1.0) {
        return input.clone();
    }

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(), scale, scale, cv::INTER_AREA);
    return resized;
}

QVector<QPointF> contourToNormalizedPoints(const std::vector<cv::Point> &contour, const cv::Size &size)
{
    QVector<QPointF> points;
    points.reserve(static_cast<int>(contour.size()));
    for (const cv::Point &point : contour) {
        points.push_back(QPointF(
            static_cast<qreal>(point.x) / static_cast<qreal>(size.width),
            static_cast<qreal>(point.y) / static_cast<qreal>(size.height)));
    }
    return points;
}

QColor indexedColor(int index)
{
    return QColor::fromHsv((index * 53) % 360, 200, 255);
}

}  // namespace

#endif

bool MediaProcessor::open(const QString &path, const ProcessingConfig &config, const QString &sourceId, QString *error)
{
    close();
    config_ = config;
    sourceId_ = sourceId;
    mediaPath_ = path;
    nextFrameId_ = 1;
    nextPathId_ = 1;
    previousPaths_.clear();

#if DEMO_HAS_OPENCV
    impl_ = new Impl();
    if (hasImageExtension(path)) {
        if (!openImage(path, error)) {
            close();
            return false;
        }
    } else if (!openVideo(path, error)) {
        close();
        return false;
    }
    return true;
#else
    Q_UNUSED(config)
    Q_UNUSED(sourceId)
    if (error != nullptr) {
        *error = QStringLiteral("当前构建未启用 OpenCV，无法处理本地图片/视频输入");
    }
    return false;
#endif
}

void MediaProcessor::close()
{
#if DEMO_HAS_OPENCV
    delete impl_;
    impl_ = nullptr;
    stillImageMode_ = false;
#endif
    mediaPath_.clear();
    previousPaths_.clear();
}

bool MediaProcessor::isOpen() const
{
#if DEMO_HAS_OPENCV
    return impl_ != nullptr;
#else
    return false;
#endif
}

QString MediaProcessor::mediaPath() const
{
    return mediaPath_;
}

bool MediaProcessor::next(Output *output, QString *error)
{
    if (output == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("输出帧对象为空");
        }
        return false;
    }

#if DEMO_HAS_OPENCV
    return readNextFrame(&output->previewImage, &output->sceneFrame, error);
#else
    if (error != nullptr) {
        *error = QStringLiteral("当前构建未启用 OpenCV");
    }
    return false;
#endif
}

QVector<PathStroke> MediaProcessor::stabilizePaths(const QVector<PathStroke> &paths)
{
    QVector<PathStroke> stabilized;
    stabilized.reserve(paths.size());

    QVector<bool> previousMatched(previousPaths_.size(), false);
    for (int currentIndex = 0; currentIndex < paths.size(); ++currentIndex) {
        const PathStroke &path = paths[currentIndex];
        const QPointF centroid = path.centroid();
        const double maxDistanceSquared = config_.matchDistanceThreshold * config_.matchDistanceThreshold;

        int bestIndex = -1;
        double bestDistance = maxDistanceSquared;
        for (int previousIndex = 0; previousIndex < previousPaths_.size(); ++previousIndex) {
            if (previousMatched[previousIndex]) {
                continue;
            }

            const double distance = squaredDistance(centroid, previousPaths_[previousIndex].centroid);
            if (distance <= bestDistance) {
                bestDistance = distance;
                bestIndex = previousIndex;
            }
        }

        PathStroke stabilizedPath = path;
        if (bestIndex >= 0) {
            const StablePathState &previous = previousPaths_[bestIndex];
            previousMatched[bestIndex] = true;
            stabilizedPath.id = previous.id;
            stabilizedPath.color = previous.color;

            QVector<QPointF> blendedPoints;
            blendedPoints.reserve(path.points.size());
            for (int pointIndex = 0; pointIndex < path.points.size(); ++pointIndex) {
                const QPointF &currentPoint = path.points[pointIndex];
                const QPointF &previousPoint = previous.points[pointIndex];
                blendedPoints.push_back(QPointF(
                    (previousPoint.x() * config_.stabilityBlend) + (currentPoint.x() * (1.0 - config_.stabilityBlend)),
                    (previousPoint.y() * config_.stabilityBlend) + (currentPoint.y() * (1.0 - config_.stabilityBlend))));
            }
            stabilizedPath.points = blendedPoints;
        } else {
            stabilizedPath.id = QStringLiteral("path-%1").arg(nextPathId_++);
        }

        stabilized.push_back(stabilizedPath);
    }

    previousPaths_.clear();
    previousPaths_.reserve(stabilized.size());
    for (const PathStroke &path : stabilized) {
        previousPaths_.push_back({path.id, path.color, path.centroid(), path.points});
    }

    return stabilized;
}

QVector<QPointF> MediaProcessor::resampleClosedPath(const QVector<QPointF> &points) const
{
    if (points.size() < 3 || config_.resamplePoints < 3) {
        return points;
    }

    QVector<QPointF> closedPoints = points;
    if (closedPoints.front() != closedPoints.back()) {
        closedPoints.push_back(closedPoints.front());
    }

    QVector<double> cumulativeLengths;
    cumulativeLengths.reserve(closedPoints.size());
    cumulativeLengths.push_back(0.0);
    for (int index = 1; index < closedPoints.size(); ++index) {
        const double segmentLength = std::sqrt(squaredDistance(closedPoints[index - 1], closedPoints[index]));
        cumulativeLengths.push_back(cumulativeLengths.back() + segmentLength);
    }

    const double totalLength = cumulativeLengths.back();
    if (totalLength <= 1e-6) {
        return points;
    }

    QVector<QPointF> sampled;
    sampled.reserve(config_.resamplePoints);
    for (int sampleIndex = 0; sampleIndex < config_.resamplePoints; ++sampleIndex) {
        const double targetDistance = (totalLength * sampleIndex) / static_cast<double>(config_.resamplePoints);
        int segmentIndex = 1;
        while (segmentIndex < cumulativeLengths.size() && cumulativeLengths[segmentIndex] < targetDistance) {
            ++segmentIndex;
        }

        if (segmentIndex >= cumulativeLengths.size()) {
            sampled.push_back(closedPoints.back());
            continue;
        }

        const double previousDistance = cumulativeLengths[segmentIndex - 1];
        const double nextDistance = cumulativeLengths[segmentIndex];
        const double segmentSpan = std::max(1e-6, nextDistance - previousDistance);
        const double localT = (targetDistance - previousDistance) / segmentSpan;
        const QPointF startPoint = closedPoints[segmentIndex - 1];
        const QPointF endPoint = closedPoints[segmentIndex];
        sampled.push_back(QPointF(
            startPoint.x() + ((endPoint.x() - startPoint.x()) * localT),
            startPoint.y() + ((endPoint.y() - startPoint.y()) * localT)));
    }

    return sampled;
}

QVector<QPointF> MediaProcessor::smoothPoints(const QVector<QPointF> &points) const
{
    if (points.size() < 5) {
        return points;
    }

    QVector<QPointF> smoothed = points;
    for (int index = 0; index < points.size(); ++index) {
        const QPointF &prev = points[(index - 1 + points.size()) % points.size()];
        const QPointF &current = points[index];
        const QPointF &next = points[(index + 1) % points.size()];
        smoothed[index] = QPointF(
            (prev.x() + (current.x() * 2.0) + next.x()) / 4.0,
            (prev.y() + (current.y() * 2.0) + next.y()) / 4.0);
    }
    return smoothed;
}

#if DEMO_HAS_OPENCV

bool MediaProcessor::openImage(const QString &path, QString *error)
{
    stillImageMode_ = true;
    impl_->stillImage = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
    if (impl_->stillImage.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("无法读取图片文件: %1").arg(path);
        }
        return false;
    }
    return true;
}

bool MediaProcessor::openVideo(const QString &path, QString *error)
{
    stillImageMode_ = false;
    if (!impl_->capture.open(path.toStdString())) {
        if (error != nullptr) {
            *error = QStringLiteral("无法打开视频文件: %1").arg(path);
        }
        return false;
    }
    return true;
}

bool MediaProcessor::readNextFrame(QImage *previewImage, SceneFrame *sceneFrame, QString *error)
{
    if (previewImage == nullptr || sceneFrame == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("媒体处理输出参数为空");
        }
        return false;
    }

    cv::Mat sourceFrame;
    if (stillImageMode_) {
        sourceFrame = impl_->stillImage.clone();
    } else {
        impl_->capture >> sourceFrame;
        if (sourceFrame.empty() && config_.loopVideo) {
            impl_->capture.set(cv::CAP_PROP_POS_FRAMES, 0);
            impl_->capture >> sourceFrame;
        }
        if (sourceFrame.empty()) {
            if (error != nullptr) {
                *error = QStringLiteral("视频已经结束，且无法继续读取帧");
            }
            return false;
        }
    }

    const cv::Mat resized = resizeToProcessingBounds(sourceFrame, config_);
    *previewImage = matToImage(resized);

    cv::Mat gray;
    cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);
    int kernelSize = std::max(3, config_.gaussianKernelSize | 1);
    cv::GaussianBlur(gray, gray, cv::Size(kernelSize, kernelSize), 0.0);

    cv::Mat edges;
    cv::Canny(gray, edges, config_.cannyLowThreshold, config_.cannyHighThreshold);

    if (config_.morphologyIterations > 0) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), config_.morphologyIterations);
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::sort(contours.begin(), contours.end(), [](const auto &left, const auto &right) {
        return cv::contourArea(left) > cv::contourArea(right);
    });

    QVector<PathStroke> extractedPaths;
    const double minArea = config_.minAreaRatio * static_cast<double>(resized.cols * resized.rows);
    int colorIndex = 0;
    for (const auto &contour : contours) {
        if (extractedPaths.size() >= config_.maxPaths) {
            break;
        }

        if (cv::contourArea(contour) < minArea || contour.size() < 3) {
            continue;
        }

        std::vector<cv::Point> simplified;
        cv::approxPolyDP(contour, simplified, config_.approximationEpsilonRatio * cv::arcLength(contour, true), true);
        if (simplified.size() < 3) {
            continue;
        }

        QVector<QPointF> points = contourToNormalizedPoints(simplified, resized.size());
        points = resampleClosedPath(points);
        points = smoothPoints(points);
        if (points.size() < 3) {
            continue;
        }

        PathStroke path;
        path.id = QStringLiteral("candidate-%1").arg(colorIndex);
        path.color = indexedColor(colorIndex++);
        path.closed = true;
        path.points = points;
        extractedPaths.push_back(path);
    }

    SceneFrame frame;
    frame.sourceId = sourceId_;
    frame.frameId = nextFrameId_++;
    frame.timestampUs = currentTimestampMicros();
    frame.sceneSize = QSize(resized.cols, resized.rows);
    frame.paths = stabilizePaths(extractedPaths);

    *sceneFrame = frame;
    return true;
}

#endif

}  // namespace demo
