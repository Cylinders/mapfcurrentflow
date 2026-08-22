#include "mapf_common/scenario_writer.h"

void mapf::writer::write_scenario(std::ostream &out, const Agents &agents) {
    out << "version 1.0" << std::endl;

    for (auto [start, goal]: agents) {
        out << 0 << '\t'
                << "temp.map" << '\t'
                << 0 << '\t'
                << 0 << '\t'
                << +start.col << '\t'
                << +start.row << '\t'
                << +goal.col << '\t'
                << +goal.row << '\t'
                << 0 << std::endl;
    }

    if (!out) {
        throw std::runtime_error{"failed to write scenario"};
    }
}
