#pragma once
#include <optional>
#include <string>

#include "pos.h"

namespace mapf {
    using AgentSolution = std::vector<Pos>;

    enum class StandardStatus {
        Solved,
        Infeasible,
        Timeout,
        MemoryLimit,
        Crash
    };

    constexpr std::string to_string(const StandardStatus status) {
        switch (status) {
            case StandardStatus::Solved: return "Solved";
            case StandardStatus::Infeasible: return "Infeasible";
            case StandardStatus::Timeout: return "Timeout";
            case StandardStatus::MemoryLimit: return "MemoryLimit";
            case StandardStatus::Crash: return "Crash";
        }

        return "impossible standard status";
    }

    struct Solution {
        std::string map;
        std::string scenario;
        std::string algo;
        std::string status;
        long time_ms;
        std::vector<std::optional<AgentSolution> > agent_solutions;

        Solution() = default;

        Solution(
            std::string map,
            std::string scenario,
            std::string algo,
            std::string status,
            long time_ms,
            std::vector<std::optional<AgentSolution> > agent_solutions
        )
            : map(std::move(map))
              , scenario(std::move(scenario))
              , algo(std::move(algo))
              , status(std::move(status))
              , time_ms(time_ms)
              , agent_solutions(std::move(agent_solutions)) {
        }

        Solution(
            std::string map,
            std::string scenario,
            std::string algo,
            StandardStatus status,
            long time_ms,
            std::vector<std::optional<AgentSolution> > agent_solutions
        ) : Solution(
            std::move(map),
            std::move(scenario),
            std::move(algo),
            std::string{to_string(status)},
            time_ms,
            std::move(agent_solutions)
        ) {
        }
    };
}
