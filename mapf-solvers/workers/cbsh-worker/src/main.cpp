#include <iostream>
#include <unistd.h>

#include "protocol.h"
#include "cbsh_adapter.h"
#include "mapf_common/map_reader.h"
#include "mapf_common/scenario_reader.h"
#include "mapf_common/solution_writer.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: cbsh-worker <cbs|cbsh> <timeout-seconds>" << std::endl;
        return 1;
    }

    const std::string mode = argv[1];

    if (mode != "cbs" && mode != "cbsh") {
        std::cerr << "invalid solver mode: " << mode << std::endl;
        return 1;
    }

    const auto timeout_s = std::stoi(argv[2]);

    // TODO: the cbs engine can be reused instead of made per solve.
    const auto solve = mode == "cbs" ? mapf_solvers::cbsh::cbs_solve : mapf_solvers::cbsh::cbsh_solve;

    std::cerr << "cbsh-worker: ready\n";

    while (true) {
        try {
            const auto [map_path, scenario_path, group_paths, time_limit] = mapf::solvers::protocol::read_run(STDIN_FILENO);
            std::cerr << "cbsh-worker: received request.\n";

            if (!group_paths.empty()) {
                std::cerr << "IGNORING GROUPS\n";
            }

            const auto grid = mapf::reader::read_map(map_path);
            const auto agents = mapf::reader::read_scenario(scenario_path);

            std::cerr << "cbsh-worker: beginning solve." << std::endl;

            auto solution = solve(time_limit, grid, agents);

            solution.map = map_path.string();
            solution.scenario = scenario_path.string();
            solution.algo = mode;

            std::cerr << "cbsh-worker: solved." << std::endl;

            const auto solution_path = scenario_path.parent_path() / "solution.sol";

            mapf::writer::write_solution(solution_path, solution);

            mapf::solvers::protocol::write_response(STDOUT_FILENO, {.solution_path = solution_path});
        } catch (const std::bad_alloc &) {
            std::cerr << "cbsh-worker error: out of memory" << std::endl;
            return 2;
        } catch (const std::exception &e) {
            std::cerr << "cbsh-worker error: " << e.what() << std::endl;
            return 1;
        }
    }
}
