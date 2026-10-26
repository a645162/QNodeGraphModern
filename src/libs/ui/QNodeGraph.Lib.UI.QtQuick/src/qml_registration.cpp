#include <QNodeGraph/Lib/UI/QtQuick/graph_controller.h>
#include <QNodeGraph/Lib/UI/QtQuick/node_palette_model.h>
#include <QNodeGraph/Lib/UI/QtQuick/qml_registration.h>

#include <QtCore/qtsymbolmacros.h>
#include <QtQml/qqml.h>

// Qt 6.12 的 qmlcache 资源由两条独立命名约定产生，必须同时存在：
//
//   1. 生成的 loader（QNodeGraph.Lib.UI.QtQuick_qmlcache_loader.cpp）把 init
//      函数命名为 URI 形式 `qInitResources_qmlcache_QNodeGraph_Lib_UI`，并用
//      Q_CONSTRUCTOR_FUNCTION 自注册 —— 这是实际能被调用的那一个。
//   2. 自动生成的插件源对 `QT_KEEP_RESOURCE(qmlcache_QNodeGraph_Lib_UI_QtQuick)`
//      发出引用，它要求的是 **target 名** 形式的符号。若该符号不存在，链接
//      静态插件时会报 LNK2019（无法解析的外部符号）。
//
// 因此这里显式声明 URI 形式（QT_DECLARE_EXTERN_RESOURCE 会以 URI 命名），再
// 提供一个 target 名形式的转发函数，静态库消费者与静态插件两种链接方式都能
// 解析到同一份资源注册。
QT_DECLARE_EXTERN_RESOURCE(qmlcache_QNodeGraph_Lib_UI)

// 返回 int：QT_KEEP_RESOURCE 生成的外部声明是 `int qInitResources_xxx(void)`，
// 签名不匹配（void）同样会解析失败。
int qInitResources_qmlcache_QNodeGraph_Lib_UI_QtQuick() {
    return qInitResources_qmlcache_QNodeGraph_Lib_UI();
}

namespace QNodeGraph::UI {

void registerQNodeGraphQmlTypes() {
    qmlRegisterType<GraphController>("QNodeGraph.UI", 1, 0, "GraphController");
    qmlRegisterType<NodePaletteModel>("QNodeGraph.UI", 1, 0,
                                      "NodePaletteModel");
}

} // namespace QNodeGraph::UI
