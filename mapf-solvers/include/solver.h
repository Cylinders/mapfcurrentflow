#pragma once

#include <filesystem>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

#include "worker.h"
#include "solver_types.h"

namespace mapf::solvers {

    // A thread-safe queue to hold our idle workers
    class WorkerQueue {
    public:
        void release(std::unique_ptr<Worker> worker) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(worker));
            cv_.notify_one(); // Wake up any thread waiting for a worker
        }

        std::unique_ptr<Worker> acquire() {
            std::unique_lock<std::mutex> lock(mutex_);
            // Wait until the queue has at least one worker available
            cv_.wait(lock, [this] { return !queue_.empty(); });

            auto worker = std::move(queue_.front());
            queue_.pop();
            return worker;
        }

    private:
        std::queue<std::unique_ptr<Worker>> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
    };

    class SolverPool {
    public:
        // Added pool_size (defaulting to 2 to support your 2 threads)
        SolverPool(
            std::filesystem::path cbs,
            std::filesystem::path cbsh,
            std::filesystem::path bcp,
            WorkerOptions options,
            std::size_t pool_size = 2
        );

        SolverPool(const SolverPool &) = delete;
        SolverPool &operator=(const SolverPool &) = delete;

        [[nodiscard]]
        SolverResult solve(const SolverRequest &request);

        [[nodiscard]]
        SolverResult solve(const SolverRequest &request, int threadName);

    private:
        WorkerQueue cbs_pool_;
        WorkerQueue cbsh_pool_;
        WorkerQueue bcp_pool_;
    };
}
