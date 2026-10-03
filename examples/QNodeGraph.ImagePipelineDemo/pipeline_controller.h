#pragma once

#include <QNodeGraph/Lib/Execution/image_execution_service.h>
#include <QNodeGraph/Lib/Execution/image_pipeline.h>
#include <QNodeGraph/Lib/UI/QtQuick/image_frame_provider.h>

#include <QImage>
#include <QObject>

class PipelineController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString previewUrl READ previewUrl NOTIFY previewUrlChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageMetadataChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageMetadataChanged)
    Q_PROPERTY(int imageChannels READ imageChannels NOTIFY imageMetadataChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)

public:
    explicit PipelineController(QNodeGraph::UI::ImageFrameProvider* provider,
                                QObject* parent = nullptr);

    [[nodiscard]] QString previewUrl() const;
    [[nodiscard]] QString status() const;
    [[nodiscard]] bool processing() const noexcept;
    [[nodiscard]] int imageWidth() const noexcept;
    [[nodiscard]] int imageHeight() const noexcept;
    [[nodiscard]] int imageChannels() const noexcept;
    [[nodiscard]] QString outputPath() const;

    Q_INVOKABLE void runDemo();
    Q_INVOKABLE void cancel();

signals:
    void previewUrlChanged();
    void statusChanged();
    void processingChanged();
    void imageMetadataChanged();
    void outputPathChanged();

private:
    void setStatus(QString value);
    void setProcessing(bool value);
    void setOutputPath(QString value);
    [[nodiscard]] QImage createDemoImage() const;

    QNodeGraph::UI::ImageFrameProvider* m_provider = nullptr;
    QNodeGraph::Execution::ImageExecutionService m_executor;
    quint64 m_requestId = 0;
    quint64 m_revision = 0;
    QString m_previewUrl;
    QString m_status = QStringLiteral("Ready");
    QString m_outputPath;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    int m_imageChannels = 0;
    bool m_processing = false;
};
