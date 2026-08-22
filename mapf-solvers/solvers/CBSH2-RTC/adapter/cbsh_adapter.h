#pragma once
#include <string>

#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/solution.h"

namespace mapf_solvers::cbsh {
    mapf::Solution cbs_solve(int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents);
    mapf::Solution cbsh_solve(int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents);
}
