#pragma once

#include <mapf_common/grid.h>
#include <mapf_common/agent.h>

#include "portfolio.h"

namespace mapf::meta {
    struct ValuationInput {
        const Grid &grid;
        const std::vector<Agent> &agents;
        const std::unordered_map<Agent, std::vector<Pos> > &paths;
    };

    struct SolverWeight {
        SolverKind solver;
        double weight;
    };

    template<typename Portfolio>
    struct Valuator {
        virtual ~Valuator() = default;

        [[nodiscard]]
        virtual std::vector<std::array<SolverWeight, Portfolio::entries.size()> > evaluate_batch(
            const Grid &grid,
            std::span<const std::vector<Agent>> agents_sets,
            const std::unordered_map<Agent, std::vector<Pos>> &paths
        ) = 0;
    };
}
