#include "solver.h"
#include <stdexcept>

namespace mapf::solvers {

    // Helper struct: Guarantees the worker is returned to the pool even if an exception occurs
    struct WorkerGuard {
        WorkerQueue& pool;
        std::unique_ptr<Worker> worker;

        ~WorkerGuard() {
            if (worker) pool.release(std::move(worker));
        }

        Worker* operator->() { return worker.get(); }
    };

    // Constructor: Pre-spawn the background processes
    SolverPool::SolverPool(
        std::filesystem::path cbs,
        std::filesystem::path cbsh,
        std::filesystem::path bcp,
        WorkerOptions options,
        std::size_t pool_size
    ) {
        for (std::size_t i = 0; i < pool_size; ++i) {
            cbs_pool_.release(std::make_unique<Worker>(cbs, std::vector<std::string>{"cbs"}, options));
            cbsh_pool_.release(std::make_unique<Worker>(cbsh, std::vector<std::string>{"cbsh"}, options));
            bcp_pool_.release(std::make_unique<Worker>(bcp, std::vector<std::string>{}, options));
        }
    }

    [[nodiscard]]
    SolverResult SolverPool::solve(const SolverRequest &request) {
        return solve(request, 0); // Route to the named-thread version
    }

    [[nodiscard]]
    SolverResult SolverPool::solve(const SolverRequest &request, int threadName) {
        switch (request.kind) {
            case meta::SolverKind::CBS: {
                WorkerGuard guard{cbs_pool_, cbs_pool_.acquire()};
                return guard->solve(request, threadName);
            }
            case meta::SolverKind::CBSH: {
                WorkerGuard guard{cbsh_pool_, cbsh_pool_.acquire()};
                return guard->solve(request, threadName);
            }
            case meta::SolverKind::BCP: {
                WorkerGuard guard{bcp_pool_, bcp_pool_.acquire()};
                return guard->solve(request, threadName);
            }
            default:
                throw std::logic_error("solver not implemented");
        }
    }
}
