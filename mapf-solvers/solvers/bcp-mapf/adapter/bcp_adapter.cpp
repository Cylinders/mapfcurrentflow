#include "bcp_adapter.h"

#include <iostream>
#include <span>

#include "problem/includes.h"
#include "problem/output.h"

#include "scip/scipshell.h"
#include "scip/scipdefplugins.h"

#include "cxxopts.hpp"
#include "struct_scip.h"
#include "struct_set.h"

#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/solution.h"

#include "problem/bcp_instance.h"
#include "problem/problem.h"

#include "heuristics/seeded_solution.h"

namespace {
    struct AgentDataHash {
        std::size_t operator()(const AgentData &agent) const {
            std::size_t h1 = std::hash<Node>{}(agent.start);
            std::size_t h2 = std::hash<Node>{}(agent.goal);
            return h1 ^ (h2 << 1);
        }
    };

    std::vector<Edge> to_edges(const Map &map, std::span<const mapf::Pos> path) {
        assert(!path.empty());

        std::vector<Edge> edges;
        edges.reserve(path.size());

        for (size_t i = 0; i < path.size(); ++i) {
            Node n = map.get_n(path[i].col + 1, path[i].row + 1);

            Direction d = Direction::INVALID;

            if (i + 1 < path.size()) {
                Node next = map.get_n(path[i + 1].col + 1, path[i + 1].row + 1);

                d = map.get_direction(n, next);
            }

            edges.push_back({n, d});
        }

        return edges;
    }

    // Read instance from file
    SCIP_RETCODE create_instance(
        SCIP *scip, // SCIP
        Map &map,
        std::vector<AgentData> &agents,
        String instance_name
    ) {
        // Load instance.
        auto instance = std::make_shared<BcpInstance>(instance_name, map, agents);

        // Create the problem.
        SCIP_CALL(SCIPprobdataCreate(scip, instance_name.c_str(), instance));

        // Done.
        return SCIP_OKAY;
    }

    SCIP_RETCODE bcp_start_solver(
        int timeout_s,
        const mapf::Grid &grid,
        const mapf::Agents &agents,
        std::span<const std::vector<mapf::Path>> group_solutions,
        mapf::Solution &out
    ) {
        // keep credits.
        std::cerr << "Branch-and-cut-and-price for multi-agent path finding\n";
        std::cerr << "Edward Lam <ed@ed-lam.com>\n";
        std::cerr << "Monash University, Melbourne, Australia\n";

        // Initialize SCIP.
        SCIP *scip = nullptr;
        SCIP_CALL(SCIPcreate(&scip));

        scip->set->disp_verblevel = SCIP_VERBLEVEL_NONE;
        // scip->set->disp_verblevel = SCIP_VERBLEVEL_NORMAL;

        // Set up plugins.
        {
            // Include some default SCIP plugins.
            {
                SCIP_CALL(SCIPincludeConshdlrLinear(scip));
                /* linear must be before its specializations due to constraint upgrading */
                SCIP_CALL(SCIPincludeConshdlrIndicator(scip));
                SCIP_CALL(SCIPincludeConshdlrIntegral(scip));
                SCIP_CALL(SCIPincludeConshdlrKnapsack(scip));
                SCIP_CALL(SCIPincludeConshdlrSetppc(scip));

                SCIP_CALL(SCIPincludeNodeselBfs(scip));
                SCIP_CALL(SCIPincludeNodeselBreadthfirst(scip));
                SCIP_CALL(SCIPincludeNodeselDfs(scip));
                SCIP_CALL(SCIPincludeNodeselEstimate(scip));
                SCIP_CALL(SCIPincludeNodeselHybridestim(scip));
                SCIP_CALL(SCIPincludeNodeselRestartdfs(scip));
                SCIP_CALL(SCIPincludeNodeselUct(scip));

                SCIP_CALL(SCIPincludeEventHdlrEstim(scip));
                SCIP_CALL(SCIPincludeEventHdlrSolvingphase(scip));

                SCIP_CALL(SCIPincludeHeurActconsdiving(scip));
                SCIP_CALL(SCIPincludeHeurAdaptivediving(scip));
                SCIP_CALL(SCIPincludeHeurBound(scip));
                SCIP_CALL(SCIPincludeHeurClique(scip));
                SCIP_CALL(SCIPincludeHeurCoefdiving(scip));
                SCIP_CALL(SCIPincludeHeurCompletesol(scip));
                SCIP_CALL(SCIPincludeHeurConflictdiving(scip));
                SCIP_CALL(SCIPincludeHeurCrossover(scip));
                SCIP_CALL(SCIPincludeHeurDins(scip));
                SCIP_CALL(SCIPincludeHeurDistributiondiving(scip));
                SCIP_CALL(SCIPincludeHeurDualval(scip));
                SCIP_CALL(SCIPincludeHeurFarkasdiving(scip));
                SCIP_CALL(SCIPincludeHeurFeaspump(scip));
                SCIP_CALL(SCIPincludeHeurFixandinfer(scip));
                SCIP_CALL(SCIPincludeHeurFracdiving(scip));
                SCIP_CALL(SCIPincludeHeurGins(scip));
                SCIP_CALL(SCIPincludeHeurGuideddiving(scip));
                SCIP_CALL(SCIPincludeHeurZeroobj(scip));
                SCIP_CALL(SCIPincludeHeurIndicator(scip));
                SCIP_CALL(SCIPincludeHeurIntdiving(scip));
                SCIP_CALL(SCIPincludeHeurIntshifting(scip));
                SCIP_CALL(SCIPincludeHeurLinesearchdiving(scip));
                SCIP_CALL(SCIPincludeHeurLocalbranching(scip));
                SCIP_CALL(SCIPincludeHeurLocks(scip));
                SCIP_CALL(SCIPincludeHeurLpface(scip));
                SCIP_CALL(SCIPincludeHeurAlns(scip));
                SCIP_CALL(SCIPincludeHeurNlpdiving(scip));
                SCIP_CALL(SCIPincludeHeurMutation(scip));
                SCIP_CALL(SCIPincludeHeurMultistart(scip));
                SCIP_CALL(SCIPincludeHeurMpec(scip));
                SCIP_CALL(SCIPincludeHeurObjpscostdiving(scip));
                SCIP_CALL(SCIPincludeHeurOctane(scip));
                SCIP_CALL(SCIPincludeHeurOfins(scip));
                SCIP_CALL(SCIPincludeHeurOneopt(scip));
                SCIP_CALL(SCIPincludeHeurPADM(scip));
                SCIP_CALL(SCIPincludeHeurProximity(scip));
                SCIP_CALL(SCIPincludeHeurPscostdiving(scip));
                SCIP_CALL(SCIPincludeHeurRandrounding(scip));
                SCIP_CALL(SCIPincludeHeurRens(scip));
                SCIP_CALL(SCIPincludeHeurReoptsols(scip));
                SCIP_CALL(SCIPincludeHeurRepair(scip));
                SCIP_CALL(SCIPincludeHeurRins(scip));
                SCIP_CALL(SCIPincludeHeurRootsoldiving(scip));
                SCIP_CALL(SCIPincludeHeurRounding(scip));
                SCIP_CALL(SCIPincludeHeurShiftandpropagate(scip));
                SCIP_CALL(SCIPincludeHeurShifting(scip));
                SCIP_CALL(SCIPincludeHeurSimplerounding(scip));
                SCIP_CALL(SCIPincludeHeurSubNlp(scip));
                SCIP_CALL(SCIPincludeHeurTrivial(scip));
                SCIP_CALL(SCIPincludeHeurTrivialnegation(scip));
                SCIP_CALL(SCIPincludeHeurTrustregion(scip));
                SCIP_CALL(SCIPincludeHeurTrySol(scip));
                SCIP_CALL(SCIPincludeHeurTwoopt(scip));
                SCIP_CALL(SCIPincludeHeurUndercover(scip));
                SCIP_CALL(SCIPincludeHeurVbounds(scip));
                SCIP_CALL(SCIPincludeHeurVeclendiving(scip));
                SCIP_CALL(SCIPincludeHeurZirounding(scip));

                SCIP_CALL(SCIPincludeDispDefault(scip));
                SCIP_CALL(SCIPincludeTableDefault(scip));

                SCIP_CALL(SCIPincludeConcurrentScipSolvers(scip));
            }
            // SCIP_CALL(SCIPincludeDefaultPlugins(scip));

            // Disable parallel solve.
            SCIP_CALL(SCIPsetIntParam(scip, "parallel/maxnthreads", 1));
            SCIP_CALL(SCIPsetIntParam(scip, "lp/threads", 1));

            // Set parameters.
            SCIP_CALL(SCIPsetIntParam(scip, "presolving/maxrounds", 0));
            // SCIP_CALL(SCIPsetIntParam(scip, "propagating/rootredcost/freq", -1));
            SCIP_CALL(SCIPsetIntParam(scip, "separating/maxaddrounds", -1));
            SCIP_CALL(SCIPsetIntParam(scip, "separating/maxstallrounds", 5));
            SCIP_CALL(SCIPsetIntParam(scip, "separating/maxstallroundsroot", 20));
            SCIP_CALL(SCIPsetIntParam(scip, "separating/cutagelimit", -1));

            // Turn off all separation algorithms.
            SCIP_CALL(SCIPsetSeparating(scip, SCIP_PARAMSETTING_OFF, TRUE));

            // Set node selection rule.
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/bfs/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/bfs/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/breadthfirst/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/breadthfirst/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/dfs/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/dfs/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/estimate/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/estimate/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/hybridestim/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/hybridestim/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/restartdfs/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/restartdfs/memsavepriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/uct/stdpriority", 0));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/uct/memsavepriority", 0));
#ifdef USE_BEST_FIRST_NODE_SELECTION
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/bfs/stdpriority", 500000));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/bfs/memsavepriority", 500000));
#endif
#ifdef USE_DEPTH_FIRST_NODE_SELECTION
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/dfs/stdpriority", 500000));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/dfs/memsavepriority", 500000));
#endif
#ifdef USE_BEST_ESTIMATE_NODE_SELECTION
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/estimate/stdpriority", 500000));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/estimate/memsavepriority", 500000));
#endif
#ifdef USE_HYBRID_ESTIMATE_BOUND_NODE_SELECTION
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/hybridestim/stdpriority", 500000));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/hybridestim/memsavepriority", 500000));
#endif
#ifdef USE_RESTART_DEPTH_FIRST_NODE_SELECTION
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/restartdfs/stdpriority", 500000));
            SCIP_CALL(SCIPsetIntParam(scip, "nodeselection/restartdfs/memsavepriority", 500000));
#endif

            // Turn on aggressive primal heuristics.
            SCIP_CALL(SCIPsetHeuristics(scip, SCIP_PARAMSETTING_AGGRESSIVE, TRUE));

            // Turn off some primal heuristics.
            {
                const auto nheurs = SCIPgetNHeurs(scip);
                auto heurs = SCIPgetHeurs(scip);
                for (Int idx = 0; idx < nheurs; ++idx) {
                    auto heur = heurs[idx];
                    const String name(SCIPheurGetName(heur));
                    if (name == "alns" ||
                        name == "bound" ||
                        name == "coefdiving" ||
                        name == "crossover" ||
                        name == "dins" ||
                        name == "fixandinfer" ||
                        name == "gins" ||
                        name == "guideddiving" ||
                        name == "intdiving" ||
                        name == "localbranching" ||
                        name == "locks" ||
                        name == "mutation" ||
                        name == "oneopt" ||
                        name == "rens" ||
                        name == "repair" ||
                        name == "rins" ||
                        name == "trivial" ||
                        name == "zeroobj" ||
                        name == "zirounding" ||
                        name == "proximity" || // Buggy
                        name == "twoopt") // Buggy
                    {
                        SCIPheurSetFreq(heur, -1);
                    }
                }
            }
        }

        auto bcp_map = Map{static_cast<Position>(grid.width), static_cast<Position>(grid.height), grid.blocked};

        std::vector<AgentData> bcp_agents;

        for (const auto [start, goal]: agents) {
            bcp_agents.push_back(AgentData{
                .start = bcp_map.get_n(start.col + 1, start.row + 1),
                .goal = bcp_map.get_n(goal.col + 1, goal.row + 1)
            });
        }

        // Set time limit.
        if (timeout_s > 0) {
            SCIP_CALL(SCIPsetRealParam(scip, "limits/time", timeout_s));
        }

        // create instance.
        SCIP_CALL(create_instance(scip, bcp_map, bcp_agents, ""));

        // seed incumbent solution.
        if (!group_solutions.empty()) {
            std::unordered_map<AgentData, Agent, AgentDataHash> agent_lookup;

            for (Agent a = 0; a < static_cast<Agent>(bcp_agents.size()); ++a) {
                agent_lookup.emplace(bcp_agents[a], a);
            }

            Vector<Vector<Edge> > seed_paths(bcp_agents.size());

            for (const auto &solution: group_solutions) {
                for (const auto &path: solution) {
                    assert(!path.empty());

                    auto edges = to_edges(bcp_map, path);
                    const auto agent = agent_lookup.at({
                        edges.front().n,
                        edges.back().n
                    });

                    seed_paths[agent] = std::move(edges);
                }
            }

            const bool has_complete_seed = std::all_of(
                seed_paths.begin(),
                seed_paths.end(),
                [](const auto &path) {
                    return !path.empty();
                }
            );

            if (has_complete_seed) {
                SCIP_CALL(SCIPincludeHeurSeededSolution(
                    scip,
                    std::move(seed_paths)
                ));
            }
        }

        SCIP_CALL(SCIPsolve(scip));

        {
            // Print.
            // SCIP_CALL(SCIPprintStatistics(scip, nullptr));

            get_best_solution(scip, out);
        }

        // Free memory.
        SCIP_CALL(SCIPfree(&scip));
        BMScheckEmptyMemory();

        // Done.
        return SCIP_OKAY;
    }

    mapf::Solution bcp_returnSol(
        int timeout_s,
        const mapf::Grid &grid,
        const mapf::Agents &agents,
        std::span<const std::vector<mapf::Path>> group_solutions
    ) {
        mapf::Solution solution;

        SCIP_RETCODE retcode = bcp_start_solver(timeout_s, grid, agents, group_solutions, solution);

        if (retcode != SCIP_OKAY) {
            std::cout << "SCIPERROR " << retcode << std::endl;
        }

        return solution;
    }
}

namespace mapf_solvers::bcp {
    mapf::Solution bcp_solve(const int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents) {
        return bcp_returnSol(timeout_s, grid, agents, {});
    }
}

namespace mapf_mergers::bcp {
    mapf::Solution bcp_merge(const int timeout_s, const mapf::Grid &grid, const mapf::Agents &agents,
                             std::span<const std::vector<mapf::Path>> group_solutions) {
        return bcp_returnSol(timeout_s, grid, agents, group_solutions);
    }
}
