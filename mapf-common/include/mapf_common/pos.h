#pragma once

#include <cstddef>
#include <functional>
#include <ostream>  // Required for std::ostream

namespace mapf {
    struct Pos {
        int row{};
        int col{};

        auto operator<=>(const Pos&) const = default;
    };

    using Path = std::vector<Pos>;
}

template <>
struct std::hash<mapf::Pos> {
    std::size_t operator()(const mapf::Pos& p) const noexcept {
        auto hrow = std::hash<int>{}(p.row);
        auto hcol = std::hash<int>{}(p.col);
        return hrow ^ (hcol << 1);
    }
};

inline std::ostream& operator<<(std::ostream& os, const mapf::Pos& pos) {
    return os << "(" << pos.row << ", " << pos.col << ")";
}
