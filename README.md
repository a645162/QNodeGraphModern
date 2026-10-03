# QNodeGraphModern

QNodeGraphModern is a reusable C++23 and Qt 6/QML node graph library. It does
not build a product application from `src/`; host applications consume the
library through the examples or link the library targets from their own CMake
project.

## Repository layout

- `src/libs/core/QNodeGraph.Lib.Core`: Qt-independent graph model foundation.
- `src/libs/ui/QNodeGraph.Lib.UI.QtQuick`: Qt Quick bridge and `QNodeGraph.UI`
  QML module.
- `examples/QNodeGraph.BasicDemo`: minimal library consumer.
- `examples/QNodeGraph.ImagePipelineDemo`: image-pipeline consumer scaffold.
- `tests`: CTest targets for the library.
- `docs/plan`: implementation plan and feature milestones.

## Configure and build

The PowerShell scripts auto-detect the local Qt kit when it is installed at
`C:\Qt\6.12.0\llvm-mingw_64`. A different kit can be supplied explicitly:

```powershell
.\scripts\build\build.ps1 -QtPrefix C:\Qt\6.12.0\llvm-mingw_64
```

Run the library consumer examples:

```powershell
.\scripts\run\run.ps1 -BasicDemo
.\scripts\run\run.ps1 -ImagePipelineDemo
```

Run tests:

```powershell
.\scripts\test\test.ps1
```

The current image pipeline example is intentionally a scaffold. Image
processing nodes and the image execution contract are defined in the plan and
will be added without turning the library into a standalone application.
