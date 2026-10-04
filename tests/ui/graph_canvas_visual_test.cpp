#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtTest/QtTest>

#include <memory>

class GraphCanvasVisualTest final : public QObject {
    Q_OBJECT

private slots:
    void rendersGraphCanvasWithNode();
    void connectsPortsThroughMouseDrag();
    void addsPaletteNodeThroughMouseDrag();
};

void GraphCanvasVisualTest::rendersGraphCanvasWithNode() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/GraphCanvas.qml")));
    QVERIFY2(component.status() == QQmlComponent::Ready,
             qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    auto* canvas = qobject_cast<QQuickItem*>(object.get());
    QVERIFY(canvas != nullptr);

    QQuickWindow window;
    window.resize(640, 420);
    canvas->setParentItem(window.contentItem());
    canvas->setWidth(window.width());
    canvas->setHeight(window.height());
    window.show();
    QTest::qWait(100);

    const auto controllerValue = canvas->property("controller");
    QVERIFY(controllerValue.isValid());
    auto* controller = controllerValue.value<QObject*>();
    QVERIFY(controller != nullptr);
    QVERIFY(QMetaObject::invokeMethod(controller,
                                      "addNodeType", Q_ARG(QString, "grayscale"),
                                      Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(
        controller, "setNodePreview", Q_ARG(int, 0),
        Q_ARG(QString,
              QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/icons/node.svg")),
        Q_ARG(int, 24), Q_ARG(int, 24), Q_ARG(int, 4)));
    QTest::qWait(100);
    const auto image = window.grabWindow();
    QVERIFY(!image.isNull());
    QVERIFY(image.width() > 0);
    QVERIFY(image.height() > 0);
}

void GraphCanvasVisualTest::connectsPortsThroughMouseDrag() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/GraphCanvas.qml")));
    QVERIFY2(component.status() == QQmlComponent::Ready,
             qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    auto* canvas = qobject_cast<QQuickItem*>(object.get());
    QVERIFY(canvas != nullptr);

    QQuickWindow window;
    window.resize(640, 420);
    canvas->setParentItem(window.contentItem());
    canvas->setWidth(window.width());
    canvas->setHeight(window.height());
    window.show();
    QTest::qWait(100);

    auto* controller = canvas->property("controller").value<QObject*>();
    QVERIFY(controller != nullptr);
    QVERIFY(QMetaObject::invokeMethod(controller, "addDemoNode",
                                      Q_ARG(bool, false)));
    QVERIFY(QMetaObject::invokeMethod(controller, "addDemoNode",
                                      Q_ARG(bool, false)));
    QTest::qWait(100);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier,
                      QPoint(260, 144));
    QTest::mouseMove(&window, QPoint(330, 144), 100);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier,
                        QPoint(330, 144));
    QTest::qWait(100);

    QCOMPARE(controller->property("connections").toList().size(), 1);
}

void GraphCanvasVisualTest::addsPaletteNodeThroughMouseDrag() {
    QQmlEngine engine;
    QQmlComponent canvasComponent(
        &engine,
        QUrl(QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/GraphCanvas.qml")));
    QQmlComponent paletteComponent(
        &engine,
        QUrl(QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/NodesPalette.qml")));
    QVERIFY2(canvasComponent.status() == QQmlComponent::Ready,
             qPrintable(canvasComponent.errorString()));
    QVERIFY2(paletteComponent.status() == QQmlComponent::Ready,
             qPrintable(paletteComponent.errorString()));

    std::unique_ptr<QObject> canvasObject(canvasComponent.create());
    QVERIFY2(canvasObject != nullptr, qPrintable(canvasComponent.errorString()));
    auto* canvas = qobject_cast<QQuickItem*>(canvasObject.get());
    QVERIFY(canvas != nullptr);
    auto* controller = canvas->property("controller").value<QObject*>();
    QVERIFY(controller != nullptr);

    std::unique_ptr<QObject> paletteObject(
        paletteComponent.createWithInitialProperties(
            {{QStringLiteral("controller"), QVariant::fromValue(controller)},
             {QStringLiteral("dropTarget"), QVariant::fromValue(canvas)}}));
    QVERIFY2(paletteObject != nullptr,
             qPrintable(paletteComponent.errorString()));
    auto* palette = qobject_cast<QQuickItem*>(paletteObject.get());
    QVERIFY(palette != nullptr);
    QQuickWindow window;
    window.resize(640, 420);
    canvas->setParentItem(window.contentItem());
    canvas->setX(220);
    canvas->setWidth(420);
    canvas->setHeight(window.height());
    palette->setParentItem(window.contentItem());
    palette->setWidth(200);
    palette->setHeight(window.height());
    window.show();
    QTest::qWait(150);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier,
                      QPoint(80, 105));
    QTest::mouseMove(&window, QPoint(400, 220), 150);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier,
                        QPoint(400, 220));
    QTest::qWait(150);

    QCOMPARE(controller->property("nodeCount").toInt(), 1);
}

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    GraphCanvasVisualTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "graph_canvas_visual_test.moc"
