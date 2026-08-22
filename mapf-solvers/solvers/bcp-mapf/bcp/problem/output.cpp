/*
This file is part of BCP-MAPF.

BCP-MAPF is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BCP-MAPF is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BCP-MAPF.  If not, see <https://www.gnu.org/licenses/>.

Author: Edward Lam <ed@ed-lam.com>
*/

#include "problem/output.h"

#include <iostream>

#include "problem/includes.h"
#include "problem/problem.h"
#include "problem/variable_data.h"
#include <string>

#include <fmt/core.h>
#include <string>
#include <vector>
#include "mapf_common/solution.h"
#include "scipoptsuite-9.2.0/soplex/src/soplex/external/fmt/format.h"

namespace {
    mapf::AgentSolution make_agent_solution(
        SCIP_ProbData *probdata,
        const Time path_length,
        const Edge *path
    ) {
        debug_assert(probdata);
        debug_assert(path);

        mapf::AgentSolution result;
        result.reserve(path_length);

        const auto map = SCIPprobdataGetMap(probdata);

        for (Time t = 0; t < path_length; ++t) {
            const Node n = path[t].n;

            const auto row = map.get_y(n) - 1;
            const auto col = map.get_x(n) - 1;

            result.push_back({
                static_cast<int>(row),
                static_cast<int>(col)
            });
        }

        return result;
    }
}

void get_best_solution(SCIP *scip, mapf::Solution &solution) {
    debug_assert(scip);

    auto *probdata = SCIPgetProbData(scip);
    debug_assert(probdata);

    const auto N = SCIPprobdataGetN(probdata);
    const auto solving_time = SCIPgetSolvingTime(scip);

    solution.algo = "bcp";
    solution.status = to_string(mapf::StandardStatus::Infeasible);
    solution.time_ms = static_cast<long>(solving_time * 1000.0);
    solution.agent_solutions.assign(N, std::nullopt);

    SCIP_SOL *scip_sol = SCIPgetBestSol(scip);
    if (!scip_sol) {
        // check if timeout
        SCIP_Real timelimit;
        SCIPgetRealParam(scip, "limits/time", &timelimit);

        if (solving_time - timelimit >= 0) {
            solution.status = to_string(mapf::StandardStatus::Timeout);
        }

        return;
    }

    const SCIP_Real obj = SCIPgetSolOrigObj(scip, scip_sol);
    if (obj >= ARTIFICIAL_VAR_COST) {
        return;
    }

    const auto &dummy_vars = SCIPprobdataGetDummyVars(probdata);
    const auto &agent_vars = SCIPprobdataGetAgentVars(probdata);

    for (Agent a = 0; a < N; ++a) {
        SCIP_VAR *dummy_var = dummy_vars[a];

        debug_assert(dummy_var);
        debug_assert(!SCIPvarGetData(dummy_var));

        if (SCIPisPositive(scip, SCIPgetSolVal(scip, scip_sol, dummy_var))) {
            return;
        }
    }

    for (Agent a = 0; a < N; ++a) {
        bool found_path = false;

        for (const auto &[var, _]: agent_vars[a]) {
            debug_assert(var);

            const SCIP_Real val = SCIPgetSolVal(scip, scip_sol, var);
            if (!SCIPisPositive(scip, val)) {
                continue;
            }

            auto *vardata = SCIPvarGetData(var);
            debug_assert(vardata);

            const Time path_length = SCIPvardataGetPathLength(vardata);
            const Edge *path = SCIPvardataGetPath(vardata);

            debug_assert(path);

            solution.agent_solutions[a] = make_agent_solution(probdata, path_length, path);

            found_path = true;
            break;
        }

        if (!found_path) {
            return;
        }
    }

    solution.status = to_string(mapf::StandardStatus::Solved);
}
