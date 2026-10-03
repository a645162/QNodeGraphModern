#include <QNodeGraph/Lib/Execution/image_execution_service.h>
#include <QNodeGraph/Lib/Execution/image_pipeline.h>
#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QtTest/QtTest>

class ImageExecutionTest final : public QObject {
    Q_OBJECT

private slots:
    void completesProcessorOnWorkerThread();
    void reportsProcessorFailure();
    void ignoresCancelledResult();
    void runsProcessorPipelineInOrder();
};

void ImageExecutionTest::completesProcessorOnWorkerThread() {
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(4, 4, QImage::Format_RGB888), QStringLiteral("input"));
    QVERIFY(frame.has_value());
    QNodeGraph::Execution::ImageExecutionService service;
    QSignalSpy finished(&service,
                       &QNodeGraph::Execution::ImageExecutionService::finished);
    QSignalSpy failed(&service,
                     &QNodeGraph::Execution::ImageExecutionService::failed);

    const auto request = service.submit(
        *frame, QNodeGraph::Image::ImageProcessors::grayscale);
    QVERIFY(request != 0);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(finished.at(0).at(0).toULongLong(), request);
    QCOMPARE(finished.at(0).at(1).value<QNodeGraph::Image::ImageFrame>().channels(),
             1);
}

void ImageExecutionTest::reportsProcessorFailure() {
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(2, 2, QImage::Format_RGB888));
    QVERIFY(frame.has_value());
    QNodeGraph::Execution::ImageExecutionService service;
    QSignalSpy failed(&service,
                     &QNodeGraph::Execution::ImageExecutionService::failed);

    const auto request = service.submit(*frame, [](const auto&) {
        return QNodeGraph::Core::GraphResult<QNodeGraph::Image::ImageFrame>(
            std::unexpected(QNodeGraph::Core::GraphError{
                QNodeGraph::Core::GraphErrorCode::InvalidImage,
                "processor failed"}));
    });
    QVERIFY(request != 0);
    QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
    QCOMPARE(failed.at(0).at(0).toULongLong(), request);
    QCOMPARE(failed.at(0).at(1).toString(), QStringLiteral("processor failed"));
}

void ImageExecutionTest::ignoresCancelledResult() {
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(2, 2, QImage::Format_RGB888));
    QVERIFY(frame.has_value());
    QNodeGraph::Execution::ImageExecutionService service;
    QSignalSpy finished(&service,
                       &QNodeGraph::Execution::ImageExecutionService::finished);
    const auto request = service.submit(*frame, [](const auto& input) {
        QThread::msleep(20);
        return QNodeGraph::Image::ImageProcessors::grayscale(input);
    });
    QVERIFY(service.cancel(request));
    QTest::qWait(100);
    QCOMPARE(finished.count(), 0);
}

void ImageExecutionTest::runsProcessorPipelineInOrder() {
    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(4, 4, QImage::Format_RGB888));
    QVERIFY(frame.has_value());

    QNodeGraph::Execution::ImagePipeline pipeline;
    pipeline.addStep("grayscale", QNodeGraph::Image::ImageProcessors::grayscale);
    pipeline.addStep("edge", QNodeGraph::Image::ImageProcessors::edgeDetect);
    QCOMPARE(pipeline.stepNames().size(), std::size_t{2});

    const auto result = pipeline.process(*frame);
    QVERIFY(result.has_value());
    QCOMPARE(result->channels(), 1);
}

QTEST_MAIN(ImageExecutionTest)
#include "image_execution_test.moc"
