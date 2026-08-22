#pragma once
#include <ostream>

#include <vector>

namespace mapf {
    struct Grid {
        int width;
        int height;
        std::vector<bool> blocked;

        [[nodiscard]] std::size_t index(const int row, const int col) const {
            return static_cast<std::size_t>(row * width + col);
        }

        [[nodiscard]] bool is_blocked(const int row, const int col) const {
            return blocked[index(row, col)];
        }
    };

    namespace dump {
        inline void dump_grid(std::ostream &os, const Grid &grid) {
            for (int y = 0; y < grid.height; ++y) {
                for (int x = 0; x < grid.width; ++x) {
                    os << (grid.blocked[y * grid.width + x] ? '@' : '.');
                }
                os << '\n';
            }
        }

    }
}
