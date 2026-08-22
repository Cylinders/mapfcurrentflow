#pragma once

#include "pos.h"

namespace mapf {
    struct Agent {
        Pos start;
        Pos goal;

        auto operator<=>(const Agent&) const = default;
    };

    using Agents = std::vector<Agent>;

    namespace dump {
        inline void dump_scenario(std::ostream& os, const std::vector<Agent>& scenario) {
            for (auto [start, goal] : scenario) {
                os << start << " " << goal << "\n";
            }
        }
    }
}

template <>
struct std::hash<mapf::Agent> {
    std::size_t operator()(const mapf::Agent& a) const noexcept {
        auto hs = std::hash<mapf::Pos>{}(a.start);
        auto hg = std::hash<mapf::Pos>{}(a.goal);
        return hs ^ hg << 1;
    }
};