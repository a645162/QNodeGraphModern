#include <QNodeGraph/Lib/Image/image_frame.h>
#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QtTest/QtTest>

class ImageFrameTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsNullImage();
    void preservesImageMetadata();
    void convertsToGrayscale();
};

void ImageFrameTest::rejectsNullImage() {
    const auto result = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage{}, QStringLiteral("missing"));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code,
             QNodeGraph::Core::GraphErrorCode::InvalidImage);
}

void ImageFrameTest::preservesImageMetadata() {
    QImage image(3, 2, QImage::Format_RGB888);
    image.fill(Qt::red);
    const auto result = QNodeGraph::Image::ImageFrame::fromQImage(
        image, QStringLiteral("sample.png"));
    QVERIFY(result.has_value());
    QCOMPARE(result->width(), 3);
    QCOMPARE(result->height(), 2);
    QCOMPARE(result->channels(), 3);
    QCOMPARE(result->source(), QStringLiteral("sample.png"));
}

void ImageFrameTest::convertsToGrayscale() {
    QImage image(2, 1, QImage::Format_RGB888);
    image.setPixelColor(0, 0, QColor(255, 0, 0));
    image.setPixelColor(1, 0, QColor(0, 255, 0));
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(image);
    QVERIFY(frame.has_value());

    const auto result = QNodeGraph::Image::ImageProcessors::grayscale(*frame);
    QVERIFY(result.has_value());
    QCOMPARE(result->channels(), 1);
    QVERIFY(result->image().pixelColor(0, 0).red() > 50);
    QVERIFY(result->image().pixelColor(1, 0).green() > 100);
}

QTEST_MAIN(ImageFrameTest)
#include "image_frame_test.moc"
