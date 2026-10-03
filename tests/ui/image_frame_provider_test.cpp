#include <QNodeGraph/Lib/Image/image_frame.h>
#include <QNodeGraph/Lib/UI/QtQuick/image_frame_provider.h>

#include <QtTest/QtTest>

class ImageFrameProviderTest final : public QObject {
    Q_OBJECT

private slots:
    void servesCachedFrameAndRequestedSize();
};

void ImageFrameProviderTest::servesCachedFrameAndRequestedSize() {
    QNodeGraph::UI::ImageFrameProvider provider(2);
    QImage image(8, 4, QImage::Format_RGB32);
    image.fill(Qt::red);
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        image, QStringLiteral("provider-test"));
    QVERIFY(frame.has_value());

    provider.setFrame(QStringLiteral("preview"), *frame);
    QSize size;
    const auto result = provider.requestImage(
        QStringLiteral("preview?revision=1"), &size, QSize(4, 4));
    QVERIFY(!result.isNull());
    QCOMPARE(size, QSize(4, 2));
    QCOMPARE(result.pixelColor(0, 0), QColor(Qt::red));

    const auto missing = provider.requestImage(QStringLiteral("missing"),
                                               &size, {});
    QVERIFY(missing.isNull());
}

QTEST_MAIN(ImageFrameProviderTest)
#include "image_frame_provider_test.moc"
