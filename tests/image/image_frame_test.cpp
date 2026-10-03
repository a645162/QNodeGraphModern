#include <QNodeGraph/Lib/Image/image_frame.h>
#include <QNodeGraph/Lib/Image/image_frame_cache.h>
#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QtTest/QtTest>

class ImageFrameTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsNullImage();
    void preservesImageMetadata();
    void convertsToGrayscale();
    void blursImage();
    void detectsEdges();
    void cachesFramesWithBoundedOwnership();
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

void ImageFrameTest::blursImage() {
    QImage image(3, 1, QImage::Format_Grayscale8);
    image.setPixelColor(0, 0, QColor(0, 0, 0));
    image.setPixelColor(1, 0, QColor(255, 255, 255));
    image.setPixelColor(2, 0, QColor(0, 0, 0));
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(image);
    QVERIFY(frame.has_value());

    const auto result = QNodeGraph::Image::ImageProcessors::blur(*frame, 1);
    QVERIFY(result.has_value());
    QCOMPARE(result->channels(), 1);
    QVERIFY(result->image().pixelColor(0, 0).red() > 0);
    QVERIFY(result->image().pixelColor(1, 0).red() < 255);
}

void ImageFrameTest::detectsEdges() {
    QImage image(5, 5, QImage::Format_Grayscale8);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            image.setPixelColor(x, y, x < 2 ? QColor(0, 0, 0)
                                           : QColor(255, 255, 255));
        }
    }
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(image);
    QVERIFY(frame.has_value());

    const auto result = QNodeGraph::Image::ImageProcessors::edgeDetect(*frame);
    QVERIFY(result.has_value());
    QCOMPARE(result->channels(), 1);
    QVERIFY(result->image().pixelColor(2, 2).red() > 100);
    QVERIFY(result->image().pixelColor(0, 2).red() < 20);
}

void ImageFrameTest::cachesFramesWithBoundedOwnership() {
    QNodeGraph::Image::ImageFrameCache cache(1);
    const auto first = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(2, 2, QImage::Format_RGB32), QStringLiteral("first"));
    const auto second = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(3, 1, QImage::Format_RGB32), QStringLiteral("second"));
    QVERIFY(first.has_value());
    QVERIFY(second.has_value());

    cache.put(QStringLiteral("preview"), *first);
    QVERIFY(cache.contains(QStringLiteral("preview")));
    QCOMPARE(cache.get(QStringLiteral("preview"))->source(),
             QStringLiteral("first"));

    cache.put(QStringLiteral("preview"), *second);
    const auto replacement = cache.get(QStringLiteral("preview"));
    QVERIFY(replacement.has_value());
    QCOMPARE(replacement->width(), 3);
    QCOMPARE(cache.size(), 1);

    cache.clear();
    QVERIFY(!cache.contains(QStringLiteral("preview")));
    QCOMPARE(cache.size(), 0);
}

QTEST_MAIN(ImageFrameTest)
#include "image_frame_test.moc"
