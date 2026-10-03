#include <pipeline_controller.h>

#include <QtTest/QtTest>

class PipelineControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void producesPreviewAfterProcessing();
};

void PipelineControllerTest::producesPreviewAfterProcessing() {
    DemoImageProvider provider;
    PipelineController controller(&provider);
    QSignalSpy previewChanged(&controller,
                              &PipelineController::previewUrlChanged);

    controller.runDemo();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.previewUrl().isEmpty(), 3000);
    QVERIFY(!previewChanged.isEmpty());

    QSize size;
    const auto preview = provider.requestImage(QStringLiteral("preview"),
                                               &size, {});
    QVERIFY(!preview.isNull());
    QCOMPARE(size, QSize(320, 180));
    QCOMPARE(preview.format(), QImage::Format_Grayscale8);
}

QTEST_MAIN(PipelineControllerTest)
#include "pipeline_controller_test.moc"

