#pragma once

#include <unordered_map>

#include "agent.h"
#include "grid.h"

namespace mapf {
    std::unordered_map<Agent, std::vector<Pos>> find_minimal_paths(const Grid &grid, const std::vector<Agent> &agents);
}
