#pragma once

#include <filesystem>
#include <fstream>

#include "agent.h"

namespace mapf::writer {
    void write_scenario(std::ostream &out, const Agents &agents);

    inline void write_scenario(const std::filesystem::path &path, const Agents &agents) {
        std::ofstream file{path};

        if (!file) {
            throw std::runtime_error("failed to open agents file");
        }

        return write_scenario(file, agents);
    }
}
