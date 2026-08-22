#include "cbsh_adapter.h"
#include <iostream>
#include <string>
#include "CBS.h"
#include "CBSHeuristic.h"
#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/solution.h"

namespace {
    mapf::Solution cbsh_solve_with_params(
        int timeout_s,
        heuristics_type h, // Options: "Zero", "CG", "DG", "WDG"
        rectangle_strategy r, // Options: "None", "R", "RM", "GR", "Disjoint"
        corridor_strategy c, // Options: "None", "C", "PC", "STC", "GC", "Disjoint"
        const mapf::Grid &grid,
        const mapf::Agents &agents
        // const std::string &mapFilePath, const std::string &scenFilePath, int n, const std::string &algo
    ) {
        int screenOption = 0; // 0: none, 1: results, 2: all
        int seed = 0;
        bool usingSipp = false;

        // std::cerr << "cbsh-adapter: setting up agents" << std::endl;
        std::vector<std::pair<int, int> > cbsh_agents;
        cbsh_agents.reserve(agents.size());

        for (auto [start, goal]: agents) {
            // std::cerr << "cbsh-adapter: converting agent (" << start << ", " << goal << ")" << std::endl;
            cbsh_agents.emplace_back(grid.index(start.row, start.col), grid.index(goal.row, goal.col));
        }

        // std::cerr << "cbsh-adapter: converted agents" << std::endl;

        // std::cerr << "cbsh-adapter: starting instance" << std::endl;
        // --- Load the Instance ---
        // Assuming constructor matches: (map_file, agents_file, num_agents, agent_indices, rows, cols, obs, warehouse_width)
        // Instance instance(mapFilePath, scenFilePath, n, "", 0, 0, 0, 0);
        Instance instance(grid.blocked, grid.height, grid.width, cbsh_agents);
        srand(seed);

        // std::cerr << "cbsh-adapter: created instance" << std::endl;

        // --- Initialize the Solver ---
        CBS cbs(instance, usingSipp, screenOption);
        cbs.setPrioritizeConflicts(true);
        cbs.setDisjointSplitting(false);
        cbs.setBypass(true);
        cbs.setRectangleReasoning(r);
        cbs.setCorridorReasoning(c);
        cbs.setHeuristicType(h);
        cbs.setTargetReasoning(true);
        cbs.setMutexReasoning(false);
        cbs.setSavingStats(false);
        // cbs.setNodeLimit(MAX_NODES); // Uncomment if MAX_NODES is defined in your headers

        // --- Run CBS ---
        int min_f_val = 0; // TODO: we can probably hint this
        cbs.solve(timeout_s, min_f_val);
        // std::cerr << "cbs_adapted: solved is " << std::boolalpha << solved << std::endl;

        min_f_val = (int) cbs.min_f_val;
        mapf::Solution solution = cbs.get_solution();
        cbs.clearSearchEngines();
        return solution;
    }
}

namespace mapf_solvers::cbsh {
    mapf::Solution cbs_solve(const int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents) {
        auto solution = cbsh_solve_with_params(timeout_s, ZERO, NR, NC, grid, agents);
        solution.algo = "cbs";
        return solution;
    }

    mapf::Solution cbsh_solve(const int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents) {
        auto solution = cbsh_solve_with_params(timeout_s, WDG, GR, GC, grid, agents);
        solution.algo = "cbsh";
        return solution;
    }
}
