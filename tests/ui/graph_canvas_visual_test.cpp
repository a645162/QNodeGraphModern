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

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    GraphCanvasVisualTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "graph_canvas_visual_test.moc"
