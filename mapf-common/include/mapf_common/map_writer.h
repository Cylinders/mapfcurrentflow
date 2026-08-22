#pragma once

#include <filesystem>
#include <fstream>

#include "grid.h"

namespace mapf::writer {
    void write_map(std::ostream &out, const Grid &grid);

    inline void write_map(const std::filesystem::path &path, const Grid &grid) {
        std::ofstream file{path};

        if (!file) {
            throw std::runtime_error("failed to open map file");
        }

        return write_map(file, grid);
    }
}
