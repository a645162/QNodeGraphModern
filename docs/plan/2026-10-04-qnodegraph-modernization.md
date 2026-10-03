# QNodeGraphModern Implementation Plan

> **For agentic workers:** Implement this plan task-by-task with a test and build checkpoint after each task.

**Goal:** Build a C++23/Qt6/QML node graph library that reproduces the NodeGraphQt interaction surface and supports image-processing nodes with visible image previews; all runnable applications consume the library through examples.

**Architecture:** Keep graph data, commands, serialization, and execution contracts in focused C++ libraries. Package the QML canvas and Qt bridge as a reusable library module; use Qt Quick/QML for host application visuals, with custom scene-graph items for high-volume canvas rendering. Reuse ImageViewerQt's layering and CMake organization without copying its QWidget UI.

**Tech Stack:** C++23, CMake 3.24+, Qt 6.5+, Qt Quick, QML, Qt Quick Controls 2, Qt Test, PowerShell build scripts.

**Spec:** The approved architectural direction from the user conversation on 2026-10-04.

## Global Constraints

- The application targets Qt 6 and C++23.
- Core graph contracts must remain independent of Qt where practical.
- QML owns presentation; C++ owns graph state, execution, persistence, and image data flow.
- Image processing must be asynchronous and must not update the UI from worker threads.
- Saved graphs use a versioned JSON format and reference large external image assets instead of embedding them by default.
- Debug builds are the default verification configuration; Release builds are required before performance claims.

## Task 1: Project Foundation

**Files:** `CMakeLists.txt`, `src/CMakeLists.txt`, `src/libs/CMakeLists.txt`, `scripts/build/build.ps1`

- [x] Configure the root project for C++23, Qt6, CTest, and optional examples/tests; no standalone application target is built from `src/`.
- [x] Add repeatable PowerShell configure/build entry point.
- [x] Add Qt deployment and packaging after the first usable UI exists.

## Task 2: Core Graph Model

**Files:** `src/libs/core/QNodeGraph.Lib.Core/*`, `tests/core/*`

- [x] Add a minimal `GraphDocument` library and tests to prove the target graph boundary builds independently of QML.
- [x] Replace the node counter with `Node`, `Port`, `Connection`, `NodeProperty`, and `GraphDocument` entities.
- [x] Add explicit validation errors for duplicate IDs, invalid ports, incompatible data types, and illegal cycles.
- [x] Add command objects for create, delete, move, connect, disconnect, and property edits.

## Task 3: Reusable QtQuick/QML Library

**Files:** `src/libs/ui/QNodeGraph.Lib.UI.QtQuick/*`, `examples/QNodeGraph.BasicDemo/*`

- [x] Add a QML module and Qt QObject bridge to the reusable UI library.
- [x] Add a basic consumer example with a visible canvas placeholder and graph controller actions.
- [x] Add a real `GraphCanvas` item with zoom, pan, selection, and coordinate transforms.
- [ ] Add node delegates, port hit testing, connection previews, and scene-graph connection rendering.
  - [x] Node delegates, C++ port hit testing, drag previews, and QML Canvas connection rendering.

## Task 4: NodeGraphQt UI Parity

**Files:** `src/libs/ui/QNodeGraph.Lib.UI.QtQuick/qml/*`, `src/libs/graph/*`

- [ ] Reproduce node title bars, colors, icons, SVG content, disabled state, vertical layout, and embedded controls.
  - [x] Type-specific node accents, icons, disabled opacity, labels, and embedded enabled control.
- [ ] Implement Properties Bin, Nodes Tree, Nodes Palette, Tab Search, context menus, hotkeys, and undo/redo UI.
  - [x] Reusable Properties Bin with selected-node property listing and type-aware editing.
  - [x] Registry-backed searchable Nodes Palette and categorized Nodes Tree components.
  - [x] Undo/Redo toolbar actions and Ctrl+Z/Ctrl+Y shortcuts backed by the Core command stack.
  - [x] Reusable Tab Search popup (Ctrl+P) and canvas context menu actions.
- [ ] Implement Group Node, Backdrop Node, subgraph proxy ports, automatic layout, and pipe slicing.
  - [x] Add reusable Group and Backdrop node descriptors with distinct visual roles, dimensions, and z-order.
  - [x] Add Group image proxy ports and command-backed node-to-group membership.
  - [x] Add topology-based automatic layout through the reusable GraphController and canvas menu.
  - [x] Add Shift-drag pipe slicing with curve intersection and undo support.
- [x] Add an offscreen screenshot smoke checkpoint for the reusable canvas; auxiliary panels remain covered by example startup and model tests.

## Task 5: Persistence and Node Registry

**Files:** `src/libs/graph/*`, `src/libs/core/*`, `tests/graph/*`

- [x] Define a versioned JSON schema for documents, nodes, ports, properties, positions, and connections.
- [x] Add node descriptors and a registry for built-in image nodes.
- [x] Add migration tests for the initial schema version and missing external assets.
  - [x] Cover schema-version rejection and missing JSON file errors.

## Task 6: Image Data and Execution

**Files:** `src/libs/image/*`, `src/libs/execution/*`, `src/libs/ui/QNodeGraph.Lib.UI.QtQuick/*`

- [x] Define `ImageFrame`, image metadata, and image error states.
- [x] Add cache ownership and lifetime policy for image frames.
- [x] Define an asynchronous image execution contract with cancellation and request-generation filtering.
- [x] Expose previews through a reusable `QQuickImageProvider` backed by the bounded frame cache.
- [x] Keep the first implementation compatible with `QImage`, while reserving an `ImageSource` interface for tiled/large images.

## Task 7: Image Pipeline Demo

**Files:** `examples/QNodeGraph.ImagePipelineDemo/*`, `tests/image/*`

- [x] Add a runnable visual demo shell with Load Image, Grayscale, and Preview placeholders.
- [x] Implement `LoadImageNode`, `GrayscaleNode`, `BlurNode`, `EdgeDetectNode`, `ImagePreviewNode`, and `SaveImageNode`.
  - [x] Provide reusable node executors, grayscale/blur/edge-detect processors, and an asynchronous `ImagePipeline` executor.
- [x] Demonstrate `Load -> Grayscale -> Edge Detect -> Preview -> Save` with a generated image.
- [x] Show image thumbnail, dimensions, channels, processing status, and save errors in the demo node cards.

## Task 8: Verification and Delivery

**Files:** `scripts/test/test.ps1`, `docs/README.md`, `README.md`, `README.zh-cn.md`

- [x] Run CMake configure and Debug build on the supported Qt kit.
- [x] Run CTest with failure output enabled (8/8 tests passed).
- [x] Run the image pipeline demo manually with the offscreen Qt platform.
- [x] Add Release build verification before performance work.
- [x] Document build, run, test, QML module layout, node authoring, and image-node extension points.
