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

    std::cerr << "cbsh-worker[" << mode << "]: ready (timeout " << timeout_s << "s)\n";

    while (true) {
        try {
            const auto [map_path, scenario_path, group_paths, time_limit] = mapf::solvers::protocol::read_run(STDIN_FILENO);
            std::cerr << "cbsh-worker[" << mode << "]: received request"
                      << " map=" << map_path
                      << " scenario=" << scenario_path
                      << " groups=" << group_paths.size()
                      << " timeout=" << time_limit << "s\n";

            if (!group_paths.empty()) {
                std::cerr << "cbsh-worker[" << mode << "]: warning: ignoring groups\n";
            }

            const auto grid = mapf::reader::read_map(map_path);
            const auto agents = mapf::reader::read_scenario(scenario_path);
            std::cerr << "cbsh-worker[" << mode << "]: loaded " << grid.width << 'x' << grid.height
                      << " map with " << agents.size() << " agents\n";

            std::cerr << "cbsh-worker[" << mode << "]: starting solve\n";

            auto solution = solve(time_limit, grid, agents);

            solution.map = map_path.string();
            solution.scenario = scenario_path.string();
            solution.algo = mode;

            std::cerr << "cbsh-worker[" << mode << "]: finished with status=" << solution.status
                      << " time_ms=" << solution.time_ms << '\n';

            const auto solution_path = scenario_path.parent_path() / "solution.sol";

            mapf::writer::write_solution(solution_path, solution);

            mapf::solvers::protocol::write_response(STDOUT_FILENO, {.solution_path = solution_path});
            std::cerr << "cbsh-worker[" << mode << "]: response sent solution=" << solution_path << '\n';
        } catch (const std::bad_alloc &) {
            std::cerr << "cbsh-worker[" << mode << "] error: out of memory\n";
            return 2;
        } catch (const std::exception &e) {
            std::cerr << "cbsh-worker[" << mode << "] error: " << e.what() << '\n';
            return 1;
        }
    }
}
