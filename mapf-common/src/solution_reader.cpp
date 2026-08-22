#include "mapf_common/solution_reader.h"

#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
    std::optional<mapf::AgentSolution> read_agent_solution_line(const std::string &line) {
        if (line == "x") {
            return std::nullopt;
        }

        mapf::AgentSolution solution;

        std::istringstream ss{line};
        std::string token;

        while (ss >> token) {
            const auto comma = token.find(',');
            if (comma == std::string::npos) {
                throw std::runtime_error{"invalid solution coordinate"};
            }

            const int col = std::stoi(token.substr(0, comma));
            const int row = std::stoi(token.substr(comma + 1));

            solution.push_back({
                .row = row,
                .col = col,
            });
        }

        return solution;
    }
}

mapf::Solution mapf::reader::read_solution(std::istream &in) {
    Solution solution;

    if (!std::getline(in, solution.map)) {
        throw std::runtime_error{"expected map path"};
    }

    if (!std::getline(in, solution.scenario)) {
        throw std::runtime_error{"expected scenario path"};
    }

    if (!std::getline(in, solution.algo)) {
        throw std::runtime_error{"expected algorithm name"};
    }

    if (!std::getline(in, solution.status)) {
        throw std::runtime_error{"expected status"};
    }

    std::string line;
    if (!std::getline(in, line)) {
        throw std::runtime_error{"expected solution time"};
    }

    solution.time_ms = std::stol(line);

    if (solution.status != to_string(StandardStatus::Solved)) {
        if (std::getline(in, line)) {
            throw std::runtime_error{"unexpected data after failed solution"};
        }

        return solution;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        solution.agent_solutions.push_back(read_agent_solution_line(line));
    }

    return solution;
}