#include "protocol.h"

#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
    void write_all(const int fd, std::string_view data) {
        while (!data.empty()) {
            const ssize_t written = write(fd, data.data(), data.size());

            if (written == -1) {
                if (errno == EINTR) continue;

                throw std::runtime_error{
                    std::string{"failed to write protocol message: "} +
                    std::strerror(errno)
                };
            }

            assert(written > 0);
            assert(static_cast<std::size_t>(written) <= data.size());

            data.remove_prefix(static_cast<std::size_t>(written));
        }
    }

    [[nodiscard]]
    std::string read_line(const int fd) {
        std::string line;

        while (true) {
            char c = '\0';
            const ssize_t count = read(fd, &c, 1);

            if (count == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error{
                    std::string{"failed to read protocol message: "} +
                    std::strerror(errno)
                };
            }

            if (count == 0) {
                throw std::runtime_error{"worker closed protocol pipe"};
            }

            assert(count == 1);

            if (c == '\n') {
                return line;
            }

            line.push_back(c);
        }
    }
}

namespace mapf::solvers::protocol {
    void write_run(const int fd, const RunCommand &command) {
        std::ostringstream out;

        out << "RUN "
                << command.map_path.string() << ' '
                << command.scenario_path.string() << ' '
                << command.group_paths.size() << ' ' << command.time_limit;

        for (const auto &path: command.group_paths) {
            out << ' ' << path.string();
        }

        out << '\n';

        std::cout << "protocol: " + out.str() << std::endl;

        write_all(fd, out.str());
    }

    RunCommand read_run(const int fd) {
        std::istringstream in{read_line(fd)};

        std::string op;
        std::filesystem::path map_path;
        std::filesystem::path scenario_path;
        std::size_t group_count;
        int time_limit;

        const bool parsed = static_cast<bool>(in >> op >> map_path >> scenario_path >> group_count >> time_limit);

        assert(parsed);

        std::vector<std::filesystem::path> group_paths;
        group_paths.reserve(group_count);

        for (std::size_t i = 0; i < group_count; ++i) {
            std::filesystem::path path;
            in >> path;
            group_paths.push_back(std::move(path));
        }

        return {
            .map_path = std::move(map_path),
            .scenario_path = std::move(scenario_path),
            .group_paths = std::move(group_paths),
            .time_limit = std::move(time_limit)
        };
    }

    void write_response(const int fd, const RunResponse &response) {
        write_all(fd, "SOLUTION " + response.solution_path.string() + '\n');
    }

    RunResponse read_response(const int fd) {
        std::istringstream in{read_line(fd)};

        std::string op;
        std::string solution_path;

        const bool parsed = static_cast<bool>(in >> op >> solution_path);

        assert(op == "SOLUTION");
        assert(parsed);

        return {
            .solution_path = std::move(solution_path)
        };
    }
}
