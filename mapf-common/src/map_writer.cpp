#include "mapf_common/map_writer.h"

void mapf::writer::write_map(std::ostream &out, const Grid &grid) {
    out << "type octile" << std::endl;
    out << "height " << grid.height << std::endl;
    out << "width " << grid.width << std::endl;
    out << "map" << std::endl;

    for (int row = 0; row < grid.height; ++row) {
        for (int col = 0; col < grid.width; ++col) {
            out << (grid.blocked[grid.index(row, col)] ? '@' : '.');
        }
        out << std::endl;
    }

    if (!out) {
        throw std::runtime_error{"failed to write map"};
    }
}