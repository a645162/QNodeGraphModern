#pragma once

// Execution contracts shared by the graph, presentation, and executor layers.
//
// These types live in Core rather than Execution because a node descriptor (the
// Graph layer) must be able to declare the policy an executor should honor.
// Core sits below both Graph and Execution, so keeping the contract here avoids
// a Graph -> Execution dependency cycle and lets hosts read a plan step without
// pulling in an executor implementation.
//
// Execution re-exports every name in this header (`using Core::Precision;` and
// friends), so existing `QNodeGraph::Execution::...` spellings keep working and
// both spellings denote the same type.

#include <cstdint>

namespace QNodeGraph::Core {

// Normalized output ROI. width/height <= 0 means "the full frame": callers that
// have no ROI contract of their own leave it default-constructed.
struct Roi {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr bool full() const noexcept {
        return width <= 0 || height <= 0;
    }
};

// Sampling behavior outside the source extent, for spatially local operators.
enum class BorderMode {
    Constant,
    Replicate,
    Reflect,
    Reflect101,
    Wrap,
};

// Sample storage precision requested for a frame.
enum class Precision {
    U8,
    U16,
    F32,
};

// Backend an operator prefers; Auto lets the host's selector choose.
enum class Backend {
    Auto,
    Cpu,
    OpenCl,
};

// Scheduling weight. It is a contract, not a hint: the host uses it to decide
// whether an interactive edit may be coalesced or deferred.
enum class CostClass {
    Realtime,
    Interactive,
    Offline,
};

// The complete execution contract of one operation: where it reads, how it
// samples the border, what precision and backend it wants, and how expensive
// the host may assume it is.
struct ExecutionPolicy {
    Roi roi;
    BorderMode border = BorderMode::Reflect101;
    Precision precision = Precision::U8;
    Backend backend = Backend::Auto;
    CostClass costClass = CostClass::Realtime;
};

} // namespace QNodeGraph::Core
