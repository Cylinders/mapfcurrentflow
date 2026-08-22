#include "mapf_common/minimal_paths.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <ranges>

std::unordered_map<mapf::Agent, std::vector<mapf::Pos>> mapf::find_minimal_paths(const Grid &grid, const std::vector<Agent> &agents) {
    std::unordered_map<Agent, std::vector<Pos>> paths;

    const auto in_bounds = [&](const Pos p) {
        return p.row >= 0 && p.row < grid.height &&
               p.col >= 0 && p.col < grid.width;
    };

    const auto passable = [&](const Pos p) {
        return in_bounds(p) && !grid.is_blocked(p.row, p.col);
    };

    const Pos dirs[] = {
        {-1, 0},
        { 1, 0},
        { 0,-1},
        { 0, 1},
    };

    for (const Agent &agent : agents) {
        std::vector<int> parent(
            static_cast<std::size_t>(grid.width * grid.height),
            -1
        );

        std::queue<Pos> q;

        if (!passable(agent.start) || !passable(agent.goal)) {
            paths.emplace(agent, std::vector<Pos>{});
            continue;
        }

        const int start_idx = static_cast<int>(grid.index(agent.start.row, agent.start.col));
        const int goal_idx  = static_cast<int>(grid.index(agent.goal.row, agent.goal.col));

        parent[static_cast<std::size_t>(start_idx)] = start_idx;
        q.push(agent.start);

        while (!q.empty()) {
            const Pos cur = q.front();
            q.pop();

            const int cur_idx = static_cast<int>(grid.index(cur.row, cur.col));

            if (cur_idx == goal_idx) {
                break;
            }

            for (const Pos d : dirs) {
                const Pos next{
                    cur.row + d.row,
                    cur.col + d.col,
                };

                if (!passable(next)) {
                    continue;
                }

                const int next_idx =
                    static_cast<int>(grid.index(next.row, next.col));

                if (parent[static_cast<std::size_t>(next_idx)] != -1) {
                    continue;
                }

                parent[static_cast<std::size_t>(next_idx)] = cur_idx;
                q.push(next);
            }
        }

        std::vector<Pos> path;

        if (parent[static_cast<std::size_t>(goal_idx)] != -1) {
            for (int at = goal_idx; at != start_idx;
                 at = parent[static_cast<std::size_t>(at)]) {
                path.push_back(Pos{
                    .row = at / grid.width,
                    .col = at % grid.width,
                });
            }

            path.push_back(agent.start);
            std::ranges::reverse(path);
        }

        paths.emplace(agent, std::move(path));
    }

    return paths;
}