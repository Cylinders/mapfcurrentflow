#pragma once

#include <filesystem>
#include <debug/vector>

namespace mapf::solvers::protocol {
    struct RunCommand {
        std::filesystem::path map_path;
        std::filesystem::path scenario_path;
        std::vector<std::filesystem::path> group_paths;
        int time_limit;
    };

    struct RunResponse {
        std::filesystem::path solution_path;
    };

    void write_run(int fd, const RunCommand &command);

    RunCommand read_run(int fd);

    void write_response(int fd, const RunResponse &response);

    RunResponse read_response(int fd);
}
