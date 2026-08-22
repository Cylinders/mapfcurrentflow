#pragma once

#include <span>

#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/solution.h"

namespace mapf_solvers::bcp {
    mapf::Solution bcp_solve(int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents);
}

namespace mapf_mergers::bcp {
    mapf::Solution bcp_merge(int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents,
                             std::span<const std::vector<mapf::Path>> group_solutions);
}
