#pragma once

#include <array>
#include <string_view>

namespace mapf::meta {
    enum class SolverKind {
        CBS,
        CBSH,
        BCP,
        MDDSAT,
        LNS
    };

    constexpr std::string to_string(SolverKind kind) {
        switch (kind) {
            case SolverKind::CBS: return "CBS";
            case SolverKind::CBSH: return "CBSH";
            case SolverKind::BCP: return "BCP";
            case SolverKind::MDDSAT: return "MDDSAT";
            case SolverKind::LNS: return "LNS";
        }

        return "<unknown>";
    }

    struct PortfolioEntry {
        SolverKind kind;
        bool implemented;
    };

    struct MapfasterPortfolio {
        static constexpr std::array entries = {
            PortfolioEntry{SolverKind::CBS, true},
            PortfolioEntry{SolverKind::CBSH, true},
            PortfolioEntry{SolverKind::BCP, true},
            PortfolioEntry{SolverKind::MDDSAT, false}
        };
    };
}
