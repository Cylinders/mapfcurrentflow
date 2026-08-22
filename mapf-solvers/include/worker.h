#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <sys/types.h>

#include "solver_types.h"

namespace mapf::solvers {
    class Worker {
    public:
        explicit Worker(
            std::filesystem::path executable,
            std::vector<std::string> arguments,
            WorkerOptions options
        );

        ~Worker();

        [[nodiscard]]
        SolverResult solve(const SolverRequest &request);

    private:
        void start();

        void stop();

        void restart();

        void log_process_status();

        [[nodiscard]]
        std::string name() const;

        std::filesystem::path executable_;
        std::vector<std::string> arguments_;
        WorkerOptions options_;

        int stdin_fd_ = -1;
        int stdout_fd_ = -1;
        pid_t pid_ = -1;
    };
}
