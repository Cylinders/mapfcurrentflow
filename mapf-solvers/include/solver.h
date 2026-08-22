#pragma once

#include <filesystem>

#include "solver_types.h"
#include "worker.h"

namespace mapf::solvers {
    class SolverPool {
    public:
        SolverPool(
            std::filesystem::path cbs,
            std::filesystem::path cbsh,
            std::filesystem::path bcp,
            WorkerOptions options
        );

        SolverPool(const SolverPool &) = delete;
        SolverPool &operator=(const SolverPool &) = delete;

        [[nodiscard]]
        SolverResult solve(const SolverRequest &request);

    private:
        Worker cbs_worker_;
        Worker cbsh_worker_;
        Worker bcp_worker_;
    };
}
