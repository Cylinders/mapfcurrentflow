#pragma once

#include <span>

#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/pos.h"

namespace mapf::meta {
    constexpr auto IMAGE_SIZE = 320;

    constexpr std::size_t mapfaster_encoded_input_size() {
        return 3 * IMAGE_SIZE * IMAGE_SIZE;
    }

    void mapfaster_encode_into(
        const Grid &grid,
        std::span<const Agent> agents,
        const std::unordered_map<Agent, std::vector<Pos>> &paths,
        std::span<float> out
    );
}
