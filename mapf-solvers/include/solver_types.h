#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <vector>

#include "mapf_common/grid.h"
#include "mapf_common/agent.h"
#include "mapf_common/solution.h"
#include "mapf_common/meta/portfolio.h"

namespace mapf::solvers {
    using Group = std::vector<Path>;
    using Groups = std::span<const Group>;

    struct SolverRequest {
        meta::SolverKind kind;
        const Grid &grid;
        const Agents &agents;
        Groups groups;
        int timeLimit;
    };

    struct SolverResult {
        std::optional<Solution> solution;
    };

    struct WorkerOptions {
        std::chrono::seconds timeout;
        std::optional<size_t> memory_limit_mb;
    };
}
