#include "worker.h"

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "protocol.h"
#include "mapf_common/map_writer.h"
#include "mapf_common/scenario_writer.h"
#include "mapf_common/solution_reader.h"
#include "mapf_common/solution_writer.h"
#include "protocol.h"

namespace mapf::solvers {
    namespace {
        void close_if_open(int &fd) {
            if (fd != -1) {
                close(fd);
                fd = -1;
            }
        }

        std::string errno_string(const char *what) {
            return std::string{what} + ": " + std::strerror(errno);
        }
    }

    Worker::Worker(std::filesystem::path executable, std::vector<std::string> arguments, WorkerOptions options)
        : executable_(std::move(executable)), arguments_(std::move(arguments)), options_(std::move(options)) {
        start();
    }

    Worker::~Worker() {
        stop();
    }

    void Worker::start() {
        int in_pipe[2]; // parent writes -> child stdin
        int out_pipe[2]; // child stdout -> parent reads

        if (pipe(in_pipe) == -1) {
            throw std::runtime_error(errno_string("pipe"));
        }

        if (pipe(out_pipe) == -1) {
            close(in_pipe[0]);
            close(in_pipe[1]);
            throw std::runtime_error(errno_string("pipe"));
        }

        pid_ = fork();

        if (pid_ == -1) {
            close(in_pipe[0]);
            close(in_pipe[1]);
            close(out_pipe[0]);
            close(out_pipe[1]);
            throw std::runtime_error(errno_string("fork"));
        }

        if (pid_ == 0) {
            dup2(in_pipe[0], STDIN_FILENO);
            dup2(out_pipe[1], STDOUT_FILENO);

            close(in_pipe[0]);
            close(in_pipe[1]);
            close(out_pipe[0]);
            close(out_pipe[1]);

            if (options_.memory_limit_mb.has_value()) {
                const rlim_t bytes = options_.memory_limit_mb.value() * 1024 * 1024;

                const rlimit limit{
                    .rlim_cur = bytes,
                    .rlim_max = bytes
                };

                if (setrlimit(RLIMIT_AS, &limit) != 0) {
                    _exit(126);
                }
            }

            std::vector<std::string> argv_storage;
            argv_storage.reserve(arguments_.size() + 2);

            argv_storage.push_back(executable_.string());

            for (const auto &argument: arguments_) {
                argv_storage.push_back(argument);
            }

            argv_storage.push_back(std::to_string(options_.timeout.count()));

            std::vector<char *> argv;
            argv.reserve(argv_storage.size() + 1);

            for (auto &argument: argv_storage) {
                argv.push_back(argument.data());
            }

            argv.push_back(nullptr);

            execv(
                executable_.c_str(),
                argv.data()
            );

            _exit(127);
        }

        close(in_pipe[0]);
        close(out_pipe[1]);

        stdin_fd_ = in_pipe[1];
        stdout_fd_ = out_pipe[0];
    }

    void Worker::stop() {
        close_if_open(stdin_fd_);
        close_if_open(stdout_fd_);

        if (pid_ != -1) {
            kill(pid_, SIGTERM);

            int status = 0;
            waitpid(pid_, &status, 0);

            pid_ = -1;
        }
    }

    void Worker::restart() {
        stop();
        start();
    }


    SolverResult Worker::solve(const SolverRequest &request, int threadName) {
            if (pid_ == -1) {
                start();
            }

            // 1. Appended threadName to the directory name to prevent thread collisions
            const auto dir = std::filesystem::temp_directory_path() / std::format("mapf-solve-{}-{}", getpid(), threadName);
            std::filesystem::create_directories(dir);

            const auto map_path = dir / "request.map";
            const auto scenario_path = dir / "request.scen";
            const auto solution_path = dir / "solution.sol";
            std::vector<std::filesystem::path> group_paths;

            auto start = std::chrono::steady_clock::now();

            try {
                // init tmp files
                writer::write_map(map_path, request.grid);
                writer::write_scenario(scenario_path, request.agents);

                if (!request.groups.empty()) {
                    for (int i = 0; i < request.groups.size(); ++i) {
                        auto group = request.groups[i];
                        auto group_path = dir / std::format("group{}.sol", i);

                        std::vector<std::optional<Path> > agent_solutions;
                        agent_solutions.reserve(group.size() + 1);
                        for (const auto &path: group) {
                            agent_solutions.emplace_back(path);
                        }

                        auto group_solution = Solution(map_path, "generated", "generated", StandardStatus::Solved, 0, agent_solutions);
                        writer::write_solution(group_path, group_solution);
                        group_paths.push_back(group_path);
                    }
                }

                // 2. Added threadName to logging for easier debugging
                std::cout << "worker [" << threadName << "]: wrote tmp input files to " << map_path.string() << std::endl;

                start = std::chrono::steady_clock::now();

                protocol::write_run(
                    stdin_fd_,
                    protocol::RunCommand{
                        .map_path = map_path,
                        .scenario_path = scenario_path,
                        .group_paths = group_paths,
                        .time_limit = request.timeLimit
                    }
                );

                std::cout << "worker [" << threadName << "]: sent request." << std::endl;

                const auto [response_solution_path] = protocol::read_response(stdout_fd_);

                std::cout << "worker [" << threadName << "]: got response in " << response_solution_path << std::endl;

                auto solution = reader::read_solution(response_solution_path);

                return {
                    .solution = solution,
                };
            } catch (const std::exception &e) {
                // TODO: catch memory alloc error?
                auto end = std::chrono::steady_clock::now();

                std::cerr << "worker [" << threadName << "] error: " << e.what() << '\n';
                restart();

                // TODO: save failed problem for reference.
                auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                auto failed_solution = Solution(map_path, scenario_path, std::to_string(static_cast<int>(request.kind)), StandardStatus::Crash, time, {});

                return {
                    .solution = {failed_solution},
                };
            }
        }
}
