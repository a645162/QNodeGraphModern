#pragma once

#include <QNodeGraph/Lib/Core/graph_document.h>

#include <QImage>
#include <QString>

namespace QNodeGraph::Image {

class ImageFrame final {
public:
    static Core::GraphResult<ImageFrame> fromQImage(
        QImage image, QString source = {});

    [[nodiscard]] const QImage& image() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] int channels() const noexcept;
    [[nodiscard]] const QString& source() const noexcept;

private:
    ImageFrame(QImage image, QString source, int channels);

    QImage m_image;
    QString m_source;
    int m_channels = 0;
};

} // namespace QNodeGraph::Image

