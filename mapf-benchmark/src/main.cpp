#include <boost/unordered/detail/map.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>

#include "manifest.h"
#include "mapf_common/agent.h"
#include "mapf_common/grid.h"
#include "mapf_common/map_reader.h"
#include "mapf_common/minimal_paths.h"
#include "mapf_common/scenario_reader.h"
#include "mapf_common/solution.h"
#include "mapf_common/solution_writer.h"
#include "solver.h"

std::vector<mapf::Agent> sample_n(const std::vector<mapf::Agent> &agents, std::size_t n);

const std::filesystem::path root = "../../mapf-benchmark";
const std::filesystem::path output = root / "output";
const std::filesystem::path tmp = root / "tmp";

void benchmark_problem(mapf::solvers::SolverPool &solver_pool, const ManifestProblem &problem, const std::filesystem::path &csv_path)
{
    std::cout << "benchmarker: starting benchmark on problem " << problem.map << std::endl;
    const mapf::Grid map = mapf::reader::read_map(problem.map);

    for (const auto &scenario_path : problem.scenarios)
    {
        std::cout << "map: " << problem.map << ", scenario: " << scenario_path << std::endl;

        std::vector<mapf::Agent> scenario;
        try
        {
            scenario = mapf::reader::read_scenario(scenario_path);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Failed to read scenario: " << e.what() << std::endl;
            continue; // Skip this scenario if we can't read it
        }

        // Track runtimes in seconds (-1.0 signifies failure)
        double cbs_time = -1.0;
        double bcp_time = -1.0;
        double cbsh_time = -1.0;
        // --- 1. CBS ---
        // try {
        //     std::cout << "starting cbs...\n";
        //     auto start = std::chrono::high_resolution_clock::now();
        //     auto solved = *solver_pool.solve({mapf::meta::SolverKind::CBS, map, scenario, {}, 600}).solution;
        //     auto end = std::chrono::high_resolution_clock::now();

        //     if (solved.status == to_string(mapf::StandardStatus::Solved)) {
        //         cbs_time = std::chrono::duration<double>(end - start).count();
        //     }
        // } catch (const std::exception& e) {
        //     std::cerr << "[ERROR] CBS exception: " << e.what() << std::endl;
        // } catch (...) { std::cerr << "[ERROR] CBS unknown exception!" << std::endl; }

        // // --- 2. BCP ---
        // try {
        //     std::cout << "starting bcp...\n";
        //     auto start = std::chrono::high_resolution_clock::now();
        //     mapf::Solution solved = *solver_pool.solve({mapf::meta::SolverKind::BCP, map, scenario, {}, 600}).solution;
        //     auto end = std::chrono::high_resolution_clock::now();

        //     if (solved.status == to_string(mapf::StandardStatus::Solved)) {
        //         bcp_time = std::chrono::duration<double>(end - start).count();
        //     }
        // } catch (const std::exception& e) {
        //     std::cerr << "[ERROR] BCP exception: " << e.what() << std::endl;
        // } catch (...) { std::cerr << "[ERROR] BCP unknown exception!" << std::endl; }

        // // --- 3. CBSH ---
        // try {
        //     std::cout << "starting cbsh...\n";
        //     auto start = std::chrono::high_resolution_clock::now();
        //     mapf::Solution solved = *solver_pool.solve({mapf::meta::SolverKind::CBSH, map, scenario, {}, 600}).solution;
        //     auto end = std::chrono::high_resolution_clock::now();

        //     if (solved.status == to_string(mapf::StandardStatus::Solved)) {
        //         cbsh_time = std::chrono::duration<double>(end - start).count();
        //     }
        // } catch (const std::exception& e) {
        //     std::cerr << "[ERROR] CBSH exception: " << e.what() << std::endl;
        // } catch (...) { std::cerr << "[ERROR] CBSH unknown exception!" << std::endl; }

        // --- OUTPUT ROW ---
        // Open file in append mode, write the line, and close immediately to save state.
        std::ofstream out(csv_path, std::ios::app);
        if (out.is_open())
        {
            out << problem.map << ","
                << scenario_path.string() << ","
                << cbs_time << ","
                << bcp_time << ","
                << cbsh_time << "\n";
            out.close(); // Ensures flush to disk!
        }
        else
        {
            std::cerr << "[ERROR] Failed to open CSV file for appending: " << csv_path << std::endl;
        }
    }
}

int main()
{
    std::cout << "mapf benchmarker" << std::endl;

    // 1. Setup outputs FIRST
    std::filesystem::create_directories(output);
    std::filesystem::path csv_path = output / "benchmark_results.csv";

    // 2. Initialize the single CSV file and write the header once
    {
        std::ofstream init_csv(csv_path, std::ios::trunc);
        init_csv << "map,scenario,CBS runtime,BCP runtime,CBSH runtime\n";
        init_csv.close();
    }

    mapf::solvers::SolverPool pool{
        MAPF_CBSH_WORKER_PATH, MAPF_CBSH_WORKER_PATH, MAPF_BCP_WORKER_PATH,
        mapf::solvers::WorkerOptions{std::chrono::seconds{300}, 8 * 1024}};

    const auto problems = parse_manifest(root / "data/problems/manifest.jsonl");

    // 3. Pass the single CSV path down to benchmark_problem
    for (const auto &problem : problems)
    {
        try
        {
            benchmark_problem(pool, problem, csv_path);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Top-level problem exception: " << e.what() << std::endl;
        }
    }

    std::cout << "Benchmarking finished. Results saved to: " << csv_path << std::endl;
    return 0;
}
/*
void paper1_benchmarker(std::vector<ManifestProblem> &problems, mapf::solvers::SolverPool &solvers);
void paper2_benchmarker(std::vector<ManifestProblem> problems, mapf::solvers::SolverPool solvers);
*/
std::vector<mapf::Agent> sample_n(const std::vector<mapf::Agent> &agents, std::size_t n)
{
    std::vector<mapf::Agent> result;
    result.reserve(n);

    std::ranges::sample(agents, std::back_inserter(result), n, std::mt19937{std::random_device{}()});

    return result;
}
