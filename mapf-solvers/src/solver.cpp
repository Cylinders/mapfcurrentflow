#include "solver.h"

#include <stdexcept>
#include <utility>

namespace mapf::solvers {
    SolverPool::SolverPool(
        std::filesystem::path cbs,
        std::filesystem::path cbsh,
        std::filesystem::path bcp,
        WorkerOptions options
    )
        : cbs_worker_(std::move(cbs), std::vector<std::string>{"cbs"}, options),
          cbsh_worker_(std::move(cbsh), std::vector<std::string>{"cbsh"}, options),
          bcp_worker_(std::move(bcp), std::vector<std::string>{}, std::move(options)) {
    }

    [[nodiscard]]
    SolverResult SolverPool::solve(const SolverRequest &request) {
        switch (request.kind) {
            case meta::SolverKind::CBS: return cbs_worker_.solve(request);
            case meta::SolverKind::CBSH: return cbsh_worker_.solve(request);
            case meta::SolverKind::BCP: return bcp_worker_.solve(request);
            default:
                throw std::logic_error("solver not implemented");
        }
    }
}
