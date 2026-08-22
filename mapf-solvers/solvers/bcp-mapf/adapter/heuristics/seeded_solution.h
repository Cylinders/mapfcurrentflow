#pragma once

#include "problem/problem.h"
#include "type_retcode.h"
#include "type_scip.h"
#include "types/map_types.h"

SCIP_RETCODE SCIPincludeHeurSeededSolution(SCIP *scip, Vector<Vector<Edge> > seed_paths);

struct SeededSolutionData {
    Vector<Vector<Edge> > seed_paths;
    bool already_tried = false;
};