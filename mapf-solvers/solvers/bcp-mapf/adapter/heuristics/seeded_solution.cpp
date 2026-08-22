#include "seeded_solution.h"

#include "problem/problem.h"
#include "problem/variable_data.h"

#include <algorithm>
#include <iostream>

#define HEUR_NAME        "mapf-seed"
#define HEUR_DESC        "MAPF seeded incumbent solution"
#define HEUR_DISPCHAR    'S'
#define HEUR_PRIORITY    20000
#define HEUR_FREQ        1
#define HEUR_FREQOFS     0
#define HEUR_MAXDEPTH    -1
#define HEUR_TIMING      SCIP_HEURTIMING_BEFORENODE
#define HEUR_USESSUBSCIP FALSE

namespace {
    bool same_path(
        const Vector<Edge> &lhs,
        const Edge *rhs,
        Time rhs_len
    ) {
        return lhs.size() == static_cast<size_t>(rhs_len) && std::equal(lhs.begin(), lhs.end(), rhs);
    }

    SCIP_RETCODE find_or_create_seed_var(
        SCIP *scip,
        SCIP_ProbData *probdata,
        Agent a,
        const Vector<Edge> &path,
        SCIP_VAR **out
    ) {
        const auto &agent_vars = SCIPprobdataGetAgentVars(probdata);

        for (const auto &[var, _]: agent_vars[a]) {
            auto *vardata = SCIPvarGetData(var);

            if (same_path(
                path,
                SCIPvardataGetPath(vardata),
                SCIPvardataGetPathLength(vardata)
            )) {
                *out = var;
                return SCIP_OKAY;
            }
        }

        SCIP_CALL(SCIPprobdataAddInitialVar(
            scip,
            probdata,
            a,
            path.size(),
            path.data(),
            out
        ));

        return SCIP_OKAY;
    }
}

static
SCIP_DECL_HEUREXEC(heurExecSeededSolution) {
    auto *heurdata = reinterpret_cast<SeededSolutionData *>(SCIPheurGetData(heur));

    debug_assert(heurdata);

    if (heurdata->already_tried) {
        *result = SCIP_DIDNOTRUN;
        return SCIP_OKAY;
    }

    heurdata->already_tried = true;

    if (nodeinfeasible) {
        *result = SCIP_DIDNOTRUN;
        return SCIP_OKAY;
    }

    *result = SCIP_DIDNOTFIND;

    auto *probdata = SCIPgetProbData(scip);
    const auto N = SCIPprobdataGetN(probdata);

    const auto &seed_paths = heurdata->seed_paths;

    if (seed_paths.size() != static_cast<size_t>(N)) {
        return SCIP_OKAY;
    }

    for (Agent a = 0; a < N; ++a) {
        if (seed_paths[a].empty()) {
            return SCIP_OKAY;
        }
    }

    SCIP_SOL *sol = nullptr;
    SCIP_CALL(SCIPcreateSol(scip, &sol, heur));

    for (Agent a = 0; a < N; ++a) {
        SCIP_VAR *var = nullptr;

        SCIP_CALL(find_or_create_seed_var(
            scip,
            probdata,
            a,
            seed_paths[a],
            &var
        ));

        debug_assert(var);
        SCIP_CALL(SCIPsetSolVal(scip, sol, var, 1.0));
    }

    SCIP_Bool success = FALSE;

    SCIP_CALL(SCIPtrySol(
        scip,
        sol,
        FALSE,
        FALSE,
        FALSE,
        TRUE,
        TRUE,
        &success
    ));

    if (success) {
        *result = SCIP_FOUNDSOL;
    }

    SCIP_CALL(SCIPfreeSol(scip, &sol));

    return SCIP_OKAY;
}

static
SCIP_DECL_HEURFREE(heurFreeSeededSolution) {
    auto *heurdata = reinterpret_cast<SeededSolutionData *>(SCIPheurGetData(heur));

    if (heurdata != nullptr) {
        heurdata->~SeededSolutionData();
        SCIPfreeBlockMemory(scip, &heurdata);
        SCIPheurSetData(heur, nullptr);
    }

    return SCIP_OKAY;
}

SCIP_RETCODE SCIPincludeHeurSeededSolution(SCIP *scip, Vector<Vector<Edge> > seed_paths) {
    SCIP_HEUR *heur = nullptr;

    SCIP_CALL(SCIPincludeHeurBasic(
        scip,
        &heur,
        HEUR_NAME,
        HEUR_DESC,
        HEUR_DISPCHAR,
        HEUR_PRIORITY,
        HEUR_FREQ,
        HEUR_FREQOFS,
        HEUR_MAXDEPTH,
        HEUR_TIMING,
        HEUR_USESSUBSCIP,
        heurExecSeededSolution,
        nullptr
    ));

    SeededSolutionData *heurdata = nullptr;
    SCIP_CALL(SCIPallocBlockMemory(scip, &heurdata));
    new(heurdata) SeededSolutionData{
        .seed_paths = std::move(seed_paths),
        .already_tried = false
    };

    SCIPheurSetData(heur, reinterpret_cast<SCIP_HeurData *>(heurdata));

    SCIP_CALL(SCIPsetHeurFree(
        scip,
        heur,
        heurFreeSeededSolution
    ));

    return SCIP_OKAY;
}
