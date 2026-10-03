#include "pipeline_controller.h"

#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QDir>
#include <QLinearGradient>
#include <QPainter>

#include <memory>

#include <utility>

DemoImageProvider::DemoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage DemoImageProvider::requestImage(const QString& id, QSize* size,
                                       const QSize& requestedSize) {
    const auto key = id.section(QLatin1Char('?'), 0, 0);
    QMutexLocker locker(&m_mutex);
    auto image = m_images.value(key);
    if (!requestedSize.isEmpty() && !image.isNull()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    if (size != nullptr) {
        *size = image.size();
    }
    return image;
}

void DemoImageProvider::setImage(QString id, QImage image) {
    QMutexLocker locker(&m_mutex);
    m_images.insert(std::move(id), std::move(image));
}

PipelineController::PipelineController(DemoImageProvider* provider,
                                       QObject* parent)
    : QObject(parent), m_provider(provider), m_executor(this) {
    Q_ASSERT(m_provider != nullptr);
    connect(&m_executor,
            &QNodeGraph::Execution::ImageExecutionService::finished, this,
            [this](quint64 requestId, QNodeGraph::Image::ImageFrame result) {
                if (requestId != m_requestId || m_provider == nullptr) {
                    return;
                }
                m_provider->setImage(QStringLiteral("preview"), result.image());
                m_previewUrl = QStringLiteral("image://pipeline/preview?revision=%1")
                                   .arg(++m_revision);
                emit previewUrlChanged();
                m_imageWidth = result.width();
                m_imageHeight = result.height();
                m_imageChannels = result.channels();
                emit imageMetadataChanged();
                const auto savedPath = QDir::temp().filePath(
                    QStringLiteral("qnodegraph-image-pipeline-preview.png"));
                if (!result.image().save(savedPath)) {
                    setProcessing(false);
                    setStatus(QStringLiteral(
                        "Load Image -> Grayscale -> Edge Detect -> Preview complete; "
                        "Save Image failed"));
                    return;
                }
                setOutputPath(savedPath);
                setProcessing(false);
                setStatus(QStringLiteral(
                              "Load Image -> Grayscale -> Edge Detect -> Preview -> "
                              "Save complete: %1 x %2, %3 channel")
                              .arg(m_imageWidth)
                              .arg(m_imageHeight)
                              .arg(m_imageChannels));
            });
    connect(&m_executor,
            &QNodeGraph::Execution::ImageExecutionService::failed, this,
            [this](quint64 requestId, const QString& message) {
                if (requestId != m_requestId) {
                    return;
                }
                setProcessing(false);
                setStatus(QStringLiteral("Processing failed: %1").arg(message));
            });
}

QString PipelineController::previewUrl() const { return m_previewUrl; }

QString PipelineController::status() const { return m_status; }

bool PipelineController::processing() const noexcept { return m_processing; }

int PipelineController::imageWidth() const noexcept { return m_imageWidth; }

int PipelineController::imageHeight() const noexcept { return m_imageHeight; }

int PipelineController::imageChannels() const noexcept { return m_imageChannels; }

QString PipelineController::outputPath() const { return m_outputPath; }

void PipelineController::runDemo() {
    if (m_processing || m_provider == nullptr) {
        return;
    }
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        createDemoImage(), QStringLiteral("generated-demo"));
    if (!frame) {
        setStatus(QStringLiteral("Unable to create demo image"));
        return;
    }
    setProcessing(true);
    setOutputPath(QString{});
    if (m_imageWidth != 0 || m_imageHeight != 0 || m_imageChannels != 0) {
        m_imageWidth = 0;
        m_imageHeight = 0;
        m_imageChannels = 0;
        emit imageMetadataChanged();
    }
    setStatus(QStringLiteral("Load Image -> Grayscale -> Edge Detect -> Preview -> Save..."));
    auto pipeline = std::make_shared<QNodeGraph::Execution::ImagePipeline>();
    pipeline->addStep(QStringLiteral("Grayscale").toStdString(),
                      QNodeGraph::Image::ImageProcessors::grayscale);
    pipeline->addStep(QStringLiteral("Edge Detect").toStdString(),
                      QNodeGraph::Image::ImageProcessors::edgeDetect);
    m_requestId = m_executor.submit(
        *frame, [pipeline](const auto& input) { return pipeline->process(input); });
    if (m_requestId == 0) {
        setProcessing(false);
        setStatus(QStringLiteral("Unable to start image processing"));
    }
}

void PipelineController::cancel() {
    if (m_processing && m_executor.cancel(m_requestId)) {
        setProcessing(false);
        setStatus(QStringLiteral("Cancelled"));
    }
}

void PipelineController::setStatus(QString value) {
    if (m_status == value) {
        return;
    }
    m_status = std::move(value);
    emit statusChanged();
}

void PipelineController::setProcessing(bool value) {
    if (m_processing == value) {
        return;
    }
    m_processing = value;
    emit processingChanged();
}

void PipelineController::setOutputPath(QString value) {
    if (m_outputPath == value) {
        return;
    }
    m_outputPath = std::move(value);
    emit outputPathChanged();
}

QImage PipelineController::createDemoImage() const {
    QImage image(QSize(320, 180), QImage::Format_RGB32);
    QPainter painter(&image);
    QLinearGradient gradient(0, 0, image.width(), image.height());
    gradient.setColorAt(0.0, QColor(QStringLiteral("#2878a8")));
    gradient.setColorAt(1.0, QColor(QStringLiteral("#d18b4b")));
    painter.fillRect(image.rect(), gradient);
    painter.setPen(QPen(QColor(QStringLiteral("#f4f7fa")), 5));
    painter.setBrush(QColor(255, 255, 255, 80));
    painter.drawEllipse(QPoint(100, 90), 48, 48);
    painter.drawRect(QRect(170, 45, 105, 90));
    painter.end();
    return image;
}
