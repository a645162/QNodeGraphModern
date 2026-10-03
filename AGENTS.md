# Repository Guidelines

## 项目结构与模块组织

这是一个可复用的 C++23/Qt 6/QML 节点图形库，`src/` 不生成独立产品应用。

- `src/libs/core/QNodeGraph.Lib.Core`：不依赖 Qt 的图模型基础。
- `src/libs/ui/QNodeGraph.Lib.UI.QtQuick`：Qt Quick 桥接、QML 模块 `QNodeGraph.UI`。
- `examples/`：库的可运行消费者，例如 `QNodeGraph.BasicDemo` 和图像管线 Demo。
- `tests/`：CTest/QtTest 测试。
- `scripts/`：PowerShell 构建、运行和测试入口。
- `docs/plan/`：功能规划与阶段性实施计划。

## 构建、测试与本地开发

Qt kit 默认探测 `C:\Qt\6.12.0\llvm-mingw_64`，也可以显式传入路径：

```powershell
.\scripts\build\build.ps1 -QtPrefix C:\Qt\6.12.0\llvm-mingw_64
.\scripts\test\test.ps1
.\scripts\run\run.ps1 -BasicDemo
.\scripts\run\run.ps1 -ImagePipelineDemo
```

直接使用 CMake 时，使用 `cmake -S . -B build\Debug -G Ninja`，然后执行
`cmake --build build\Debug --parallel`。提交前至少运行一次完整构建和
`ctest --test-dir build\Debug --output-on-failure`。

## 编码风格与命名

- C++ 使用 C++23，四空格缩进，文件名使用小写下划线，如 `graph_document.cpp`。
- 类、QObject 和 QML 类型使用 PascalCase；方法和属性使用 camelCase。
- 公共头文件放在目标库的 `include/QNodeGraph/...`，实现放在对应 `src/`。
- QML 组件使用 PascalCase 文件名；保持 QML 视觉状态与 C++ 模型状态分离。
- 当前未配置自动格式化工具；修改后应保持现有 CMake 和 Qt 风格，并避免新增编译警告。

## 测试约定

C++ 单元测试使用 QtTest，测试目标位于 `tests/` 并通过 CTest 注册。测试文件以
`*_test.cpp` 命名，测试类以被测组件名加 `Test` 命名。图模型、序列化、端口连接
和异步图像处理应优先增加独立测试；QML 交互改动应至少验证对应 Example 能启动。

## 提交与 Pull Request

历史记录目前只有初始化提交，尚未形成固定提交格式。建议使用简短、祈使式主题，
例如 `feat(graph): add connection model` 或 `fix(qml): preserve node selection`。
PR 应说明设计影响、受影响的库和 Example，附上构建与测试命令及结果；涉及 QML
视觉变化时附截图或录屏。不要提交 `build/`、Qt 运行时文件或本地图片缓存。

## 架构与配置注意事项

库接口应保持可被外部 CMake 项目链接，运行窗口只放在 `examples/`。图像处理节点
必须通过异步执行接口更新 UI，禁止从工作线程直接操作 QML 对象。大型图像默认使用
外部资源引用，不要把原始图像嵌入图文档或提交到仓库。

## 提交与推送规范

最近的提交历史采用简短、祈使式主题，并带可选 scope，例如 `feat(Qt): ...`、`docs(Regex): ...`、`docs(Agents): ...`。
最好这个scope开头是大写吧？这样好看点！

提交要求：

- 按模块分批提交，不要把无关改动混在同一次提交里。
- 每次提交前必须先执行一次代码清理，默认使用 `python scripts/image_viewer/build/format.py`；若只想收敛局部改动，可传入本次涉及的文件路径。
- 每完成一块可验证的改造，都应执行一次 `git commit`，并尽快 `git push`。
- 必须使用简体中文撰写提交说明。
- 如果能确定模块，优先带 scope，例如 `build(scripts): ...`、`feat(WPF): ...`。
- `UI/Qt` 相关命名统一使用 `UI/Qt`，不要写成 `UiQt`

常用前缀说明：

| 前缀 | 含义 | 适用场景 |
| --- | --- | --- |
| `feat` | 新功能 | 新增功能、接口、组件等 |
| `fix` | 修复 Bug | 修复缺陷或错误 |
| `docs` | 文档变更 | 只修改文档、注释等 |
| `style` | 代码格式 | 仅格式调整，不影响逻辑 |
| `refactor` | 代码重构 | 非新功能、非修 Bug 的结构优化 |
| `perf` | 性能优化 | 提升性能或改善体验 |
| `test` | 测试相关 | 新增或修正测试 |
| `build` | 构建系统 | 构建配置、依赖、脚本相关 |
| `ci` | 持续集成 | CI/CD 配置与脚本 |
| `chore` | 日常杂项 | 依赖升级、清理文件等 |
| `revert` | 版本回滚 | 撤销历史提交 |
