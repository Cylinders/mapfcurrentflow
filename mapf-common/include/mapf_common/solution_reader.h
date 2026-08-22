#pragma once

#include <filesystem>
#include <fstream>

#include "solution.h"

namespace mapf::reader {
    Solution read_solution(std::istream & in);

    inline Solution read_solution(const std::filesystem::path &path) {
        std::ifstream file{path};

        if (!file) {
            throw std::runtime_error("failed to open solution file");
        }

        return read_solution(file);
    }
}
