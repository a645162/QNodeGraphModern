#include <QNodeGraph/Lib/Execution/image_execution_service.h>
#include <QNodeGraph/Lib/Execution/image_nodes.h>
#include <QNodeGraph/Lib/Execution/image_pipeline.h>
#include <QNodeGraph/Lib/Image/image_processors.h>

#include <QtTest/QtTest>

#include <QTemporaryDir>

class ImageExecutionTest final : public QObject {
    Q_OBJECT

private slots:
    void completesProcessorOnWorkerThread();
    void reportsProcessorFailure();
    void ignoresCancelledResult();
    void runsProcessorPipelineInOrder();
    void executesBuiltInImageNodes();
    void reportsImageNodeIoErrors();
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

void ImageExecutionTest::executesBuiltInImageNodes() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto inputPath = directory.filePath(QStringLiteral("input.png"));
    const auto outputPath = directory.filePath(QStringLiteral("output.png"));
    QImage input(6, 4, QImage::Format_RGB32);
    input.fill(Qt::green);
    QVERIFY(input.save(inputPath));

    const auto loaded =
        QNodeGraph::Execution::LoadImageNode::execute(inputPath);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->width(), 6);
    QCOMPARE(loaded->height(), 4);

    const auto grayscale =
        QNodeGraph::Execution::GrayscaleNode::execute(*loaded);
    QVERIFY(grayscale.has_value());
    QCOMPARE(grayscale->channels(), 1);

    const auto blurred =
        QNodeGraph::Execution::BlurNode::execute(*grayscale, 1);
    QVERIFY(blurred.has_value());
    const auto edges =
        QNodeGraph::Execution::EdgeDetectNode::execute(*blurred);
    QVERIFY(edges.has_value());
    const auto preview =
        QNodeGraph::Execution::ImagePreviewNode::execute(*edges);
    QVERIFY(preview.has_value());
    QCOMPARE(preview->image(), edges->image());

    QVERIFY(QNodeGraph::Execution::SaveImageNode::execute(*preview, outputPath));
    QVERIFY(QFileInfo::exists(outputPath));
}

void ImageExecutionTest::reportsImageNodeIoErrors() {
    const auto missing =
        QNodeGraph::Execution::LoadImageNode::execute(QStringLiteral(""));
    QVERIFY(!missing.has_value());
    QCOMPARE(missing.error().code,
             QNodeGraph::Core::GraphErrorCode::ImageIoError);

    const auto frame = QNodeGraph::Image::ImageFrame::fromQImage(
        QImage(1, 1, QImage::Format_RGB32));
    QVERIFY(frame.has_value());
    const auto saved = QNodeGraph::Execution::SaveImageNode::execute(
        *frame, QStringLiteral(""));
    QVERIFY(!saved.has_value());
    QCOMPARE(saved.error().code,
             QNodeGraph::Core::GraphErrorCode::ImageIoError);
}

QTEST_MAIN(ImageExecutionTest)
#include "image_execution_test.moc"
