#include <cassert>
#include <iostream>
#include <unistd.h>

#include "protocol.h"
#include "bcp_adapter.h"
#include "mapf_common/map_reader.h"
#include "mapf_common/scenario_reader.h"
#include "mapf_common/solution_reader.h"
#include "mapf_common/solution_writer.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: bcp-worker <timeout-seconds>\n";
        return 1;
    }

    const auto timeout = std::chrono::seconds{std::stoll(argv[1])};

    while (true) {
        try {
            const auto [map_path, scenario_path, group_paths, time_limit] = mapf::solvers::protocol::read_run(STDIN_FILENO);

            const auto grid = mapf::reader::read_map(map_path);
            const auto agents = mapf::reader::read_scenario(scenario_path);
            std::vector<std::vector<mapf::Path> > group_solutions;
            group_solutions.reserve(group_paths.size());

            for (const auto &path: group_paths) {
                auto solution = mapf::reader::read_solution(path);

                std::vector<mapf::Path> paths;
                paths.reserve(solution.agent_solutions.size());

                for (auto &agent_solution: solution.agent_solutions) {
                    assert(agent_solution);
                    paths.push_back(std::move(*agent_solution));
                }

                group_solutions.push_back(std::move(paths));
            }

            auto solution = group_paths.empty()
                                ? mapf_solvers::bcp::bcp_solve(time_limit, grid, agents)
                                : mapf_mergers::bcp::bcp_merge(time_limit, grid, agents, group_solutions);

            solution.map = map_path;
            solution.scenario = scenario_path;

            const auto solution_path = scenario_path.parent_path() / "solution.sol";

            mapf::writer::write_solution(solution_path, solution);

            mapf::solvers::protocol::write_response(STDOUT_FILENO, {.solution_path = solution_path});
        } catch (const std::bad_alloc &) {
            std::cerr << "bcp-worker error: out of memory\n";
            return 2;
        } catch (const std::exception &e) {
            std::cerr << "bcp-worker error: " << e.what() << '\n';
            return 1;
        }
    }
}
