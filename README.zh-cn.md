# QNodeGraphModern

QNodeGraphModern 是一个库优先的 C++23、Qt 6、Qt Quick/QML 节点图框架。
`src/` 只生成可复用库，窗口和可运行程序全部位于 `examples/` 或外部宿主项目。

## 快速开始

```powershell
.\scripts\build\build.ps1 -QtPrefix C:\Qt\6.12.0\llvm-mingw_64
.\scripts\test\test.ps1
.\scripts\run\run.ps1 -BasicDemo
.\scripts\run\run.ps1 -ImagePipelineDemo
```

核心库包括：Core 图模型、Graph 注册表和 JSON 持久化、Image 图像帧与处理器、
Execution 异步执行服务，以及提供 `QNodeGraph.UI` QML 模块的 UI/QtQuick 库。

图像示例通过库中的 `LoadImageNode`、`GrayscaleNode`、`BlurNode`、
`EdgeDetectNode`、`ImagePreviewNode` 和 `SaveImageNode` 组成处理链，并使用
可复用的 `ImageFrameProvider` 在 QML 中显示预览。

详细的外部 CMake 接入、QML 组件、节点扩展、持久化和线程约束见
[docs/README.md](docs/README.md)。
