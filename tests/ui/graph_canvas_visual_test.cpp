#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QColor>
#include <QImage>
#include <QQuickItem>
#include <QQuickWindow>
#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest/QtTest>

#include <memory>

Q_IMPORT_PLUGIN(QNodeGraph_UIPlugin)

class GraphCanvasVisualTest final : public QObject {
    Q_OBJECT

private slots:
    void rendersGraphCanvasWithNode();
    void connectsPortsThroughMouseDrag();
    void addsPaletteNodeThroughMouseDrag();
    void togglesThemeBetweenLightAndDark();
    void loadsCustomNodeContent();
    void rendersConnectionWireBetweenNodes();
};

void GraphCanvasVisualTest::rendersConnectionWireBetweenNodes() {
    QQmlApplicationEngine engine;
    engine.loadFromModule("QNodeGraph.UI", "GraphCanvas");
    QVERIFY2(!engine.rootObjects().isEmpty(), "canvas load failed");
    auto* canvas = qobject_cast<QQuickItem*>(engine.rootObjects().first());
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
    QMetaObject::invokeMethod(controller, "addDemoNode", Q_ARG(bool, false));
    QMetaObject::invokeMethod(controller, "addDemoNode", Q_ARG(bool, false));
    QMetaObject::invokeMethod(controller, "beginConnection", Q_ARG(int, 0));
    QMetaObject::invokeMethod(controller, "completeConnectionAt",
                              Q_ARG(double, 330.0), Q_ARG(double, 144.0));
    QTest::qWait(200);

    const auto image = window.grabWindow();
    const auto background = QColor(QStringLiteral("#191d22"));
    int wirePixels = 0;
    for (int x = 270; x <= 320; ++x) {
        const auto color = image.pixelColor(x, 144);
        if (color != background)
            ++wirePixels;
    }
    QVERIFY2(wirePixels > 0,
             "the connection wire is not visible between the two ports");
}

void GraphCanvasVisualTest::togglesThemeBetweenLightAndDark() {
    QQmlApplicationEngine engine;
    engine.loadFromModule("QNodeGraph.UI", "GraphCanvas");
    QVERIFY2(!engine.rootObjects().isEmpty(),
             qPrintable(QStringLiteral("GraphCanvas module failed to load")));
    auto* canvas = qobject_cast<QQuickItem*>(engine.rootObjects().first());
    QVERIFY(canvas != nullptr);
    QVERIFY(QMetaObject::invokeMethod(canvas, "setThemeMode",
                                      Q_ARG(QVariant, 2)));
    QTest::qWait(50);
    const auto dark = canvas->property("darkTheme").toBool();
    QVERIFY(QMetaObject::invokeMethod(canvas, "setThemeMode",
                                      Q_ARG(QVariant, 1)));
    QTest::qWait(50);
    const auto light = canvas->property("darkTheme").toBool();
    qDebug() << "dark:" << dark << "light:" << light;
    QVERIFY(dark);
    QVERIFY(!light);
}

void GraphCanvasVisualTest::loadsCustomNodeContent() {
    QQmlEngine engine;
    QQmlComponent component(
        &engine,
        QUrl(QStringLiteral("qrc:/qt/qml/QNodeGraph/UI/qml/GraphCanvas.qml")));
    QVERIFY2(component.status() == QQmlComponent::Ready,
             qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object != nullptr);
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
    QVERIFY(QMetaObject::invokeMethod(controller, "addNodeType",
                                      Q_ARG(QString, "blur"),
                                      Q_ARG(bool, false)));
    QTest::qWait(300);
    QCOMPARE(controller->property("nodeCount").toInt(), 1);
    const auto image = window.grabWindow();
    QVERIFY(!image.isNull());
}

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
#ifdef QNODEGRAPH_QML_IMPORT_DIR
    qputenv("QML_IMPORT_PATH", QNODEGRAPH_QML_IMPORT_DIR);
#endif
    QGuiApplication app(argc, argv);
    GraphCanvasVisualTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "graph_canvas_visual_test.moc"
