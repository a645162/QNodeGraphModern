# 使用与扩展指南

## 库边界

`QNodeGraph.Lib.Core` 不依赖 Qt，负责节点、端口、连接、属性和命令栈。
`QNodeGraph.Lib.Graph` 提供内置节点描述和版本化 JSON。`QNodeGraph.Lib.Image`
提供 `ImageFrame`、处理器、有界 `ImageFrameCache` 和可替换的 `ImageSource` tile 接口。
`QNodeGraph.Lib.Execution`
提供 `ImageExecutionService`、`ImagePipeline` 以及六个内置图像节点执行器。
`QNodeGraph.Lib.UI.QtQuick` 提供 `GraphController`、可复用 QML 组件和
`ImageFrameProvider`。

外部 CMake 项目应链接所需库目标，并由自己的 executable 创建
`QGuiApplication`、`QQmlApplicationEngine` 和窗口。不要让库反向依赖
`examples/`。

## QML 模块

UI 库注册 `QNodeGraph.UI 1.0`，组件包括 `GraphCanvas`、`PropertiesBin`、
`NodesPalette`、`NodesTree` 和 `TabSearch`。宿主程序调用
`QNodeGraph::UI::registerQNodeGraphQmlTypes()` 后即可加载这些组件。
`GraphCanvas` 支持缩放、平移、选择、端口连线、自动布局和 Shift+拖拽切线。

## 添加节点

在 `NodeRegistry::withBuiltins()` 或宿主注册表中增加 `NodeDescriptor`，为节点
声明输入/输出端口及 `Core::PortDataType`。图像处理节点应提供无 UI 的执行函数，
通过 `ImageExecutionService::submit()` 放到工作线程，并只在主线程更新 QML 属性。
节点预览可写入 `ImageFrameProvider::setFrame()`，QML 使用
`image://<provider>/<id>` 读取。

## 持久化与资源

`GraphJson` 使用版本化 schema；旧文档先调用 `GraphJson::migrate()`。
图片等大资源使用相对外部路径，加载后可用
`validateExternalAssets(document, baseDirectory)` 检查缺失文件，不要把原始
图像默认嵌入 JSON。

## 验证

Debug 和 Release 构建均使用 CMake/Ninja。提交前运行完整 `ctest --output-on-failure`
并在 `QT_QPA_PLATFORM=offscreen` 下启动两个示例。

## 安装与部署

使用 `cmake --install` 可导出库、头文件、QML 源文件和
`QNodeGraphModernConfig.cmake`。Windows 示例部署使用：

```powershell
.\scripts\package\package.ps1 -Configuration Release `
    -QtPrefix C:\Qt\6.12.0\llvm-mingw_64
```

宿主项目需要同时提供匹配版本的 Qt 前缀，随后通过
`find_package(QNodeGraphModern CONFIG REQUIRED)` 链接导出的
`QNodeGraph::QNodeGraph.Lib.*` 目标。
