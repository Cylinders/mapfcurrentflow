#pragma once

#include <filesystem>
#include <fstream>

#include "grid.h"

namespace mapf::reader {
    Grid read_map(std::istream &in);

    inline Grid read_map(const std::filesystem::path &path) {
        std::ifstream file{path};

        if (!file) {
            throw std::runtime_error("failed to open map file");
        }

        return read_map(file);
    }
}
