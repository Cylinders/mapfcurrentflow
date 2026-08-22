#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "manifest.h"
#include "mapf_common/grid.h"
#include "mapf_common/map_reader.h"
#include "mapf_common/scenario_reader.h"
#include "mapf_common/solution_writer.h"
#include "solver.h"

namespace {
const std::filesystem::path benchmark_root = MAPF_BENCHMARK_SOURCE_DIR;

enum class BenchmarkLevel {
  level0 = 0,
};

struct BenchmarkOptions {
  BenchmarkLevel level = BenchmarkLevel::level0;
  int timeout_seconds = 600;
  std::size_t memory_limit_mb = 8 * 1024;
  std::filesystem::path manifest_path =
      benchmark_root / "data/problems/manifest.jsonl";
  std::filesystem::path output_path = benchmark_root / "output";
};

template <typename Integer>
Integer parse_positive_integer(std::string_view value,
                               std::string_view option_name) {
  Integer number{};
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), number);
  if (error != std::errc{} || end != value.data() + value.size() ||
      number <= 0) {
    throw std::invalid_argument(std::string(option_name) +
                                " must be a positive integer");
  }
  return number;
}

BenchmarkLevel parse_level(std::string_view value) {
  int level_number = -1;
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), level_number);
  if (error != std::errc{} || end != value.data() + value.size() ||
      level_number != 0) {
    throw std::invalid_argument("Unknown benchmark level: " +
                                std::string(value));
  }
  return static_cast<BenchmarkLevel>(level_number);
}

BenchmarkOptions parse_options(int argc, char **argv) {
  BenchmarkOptions options;
  bool positional_level_seen = false;

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument = argv[index];
    const auto require_value =
        [&](std::string_view option) -> std::string_view {
      if (++index >= argc) {
        throw std::invalid_argument(std::string(option) + " requires a value");
      }
      return argv[index];
    };

    if (argument == "--level") {
      options.level = parse_level(require_value(argument));
    } else if (argument == "--timeout") {
      options.timeout_seconds =
          parse_positive_integer<int>(require_value(argument), argument);
    } else if (argument == "--memory-limit-mb") {
      options.memory_limit_mb = parse_positive_integer<std::size_t>(
          require_value(argument), argument);
    } else if (argument == "--manifest") {
      options.manifest_path = require_value(argument);
    } else if (argument == "--output") {
      options.output_path = require_value(argument);
    } else if (!argument.starts_with('-') && !positional_level_seen) {
      options.level = parse_level(argument);
      positional_level_seen = true;
    } else {
      throw std::invalid_argument("Unknown argument: " + std::string(argument));
    }
  }

  return options;
}

namespace level0 {
constexpr std::array solvers{
    mapf::meta::SolverKind::CBS,
    mapf::meta::SolverKind::CBSH,
    mapf::meta::SolverKind::BCP,
};

struct RunMetadata {
  std::string map;
  std::string scenario;
  std::string solver;
  std::size_t num_agents;
  int map_width;
  int map_height;
  std::size_t open_spaces;
  std::size_t obstacles;
  int timeout_seconds;
  std::size_t memory_limit_mb;
  long long runtime_ms;
  std::string status;
  std::string solution_file;
  std::string error;
};

std::string csv_escape(const std::string &value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }

  std::string escaped = "\"";
  for (const char character : value) {
    escaped += character == '\"' ? "\"\"" : std::string(1, character);
  }
  return escaped + "\"";
}

void write_metadata(std::ostream &csv, const RunMetadata &metadata) {
  csv << csv_escape(metadata.map) << ',' << csv_escape(metadata.scenario) << ','
      << metadata.solver << ',' << metadata.num_agents << ','
      << metadata.map_width << ',' << metadata.map_height << ','
      << metadata.open_spaces << ',' << metadata.obstacles << ','
      << metadata.timeout_seconds << ',' << metadata.memory_limit_mb << ','
      << metadata.runtime_ms << ',' << metadata.status << ','
      << csv_escape(metadata.solution_file) << ',' << csv_escape(metadata.error)
      << '\n';
  csv.flush();
}

void run_problem(mapf::solvers::SolverPool &solver_pool,
                 const ManifestProblem &problem,
                 const std::filesystem::path &solution_output,
                 std::ostream &metadata_csv, const BenchmarkOptions &options) {
  std::cout << "benchmarker: starting benchmark on problem " << problem.map
            << std::endl;
  const mapf::Grid map = mapf::reader::read_map(problem.map);
  const std::size_t obstacles = std::ranges::count(map.blocked, true);
  const std::size_t open_spaces = map.blocked.size() - obstacles;
  const std::filesystem::path manifest_directory =
      options.manifest_path.parent_path();
  const std::string map_name =
      problem.map.lexically_relative(manifest_directory).generic_string();

  for (const auto &scenario_path : problem.scenarios) {
    std::cout << "map: " << problem.map << ", scenario: " << scenario_path
              << std::endl;

    mapf::Agents scenario;
    try {
      scenario = mapf::reader::read_scenario(scenario_path);
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] Failed to read scenario: " << e.what() << std::endl;
      continue;
    }

    const std::string instance_name =
        problem.id + "__" + scenario_path.stem().string();
    const std::string scenario_name =
        scenario_path.lexically_relative(manifest_directory).generic_string();

    for (const mapf::meta::SolverKind solver : solvers) {
      const std::string solver_name = mapf::meta::to_string(solver);
      const std::filesystem::path solution_path =
          solution_output / (instance_name + "__" + solver_name + ".sol");
      const std::string solution_output_name =
          solution_path.lexically_relative(solution_output.parent_path())
              .generic_string();
      std::string status = "NoSolution";
      std::string written_solution;

      std::cout << "benchmarker: running " << solver_name << std::endl;
      const auto start = std::chrono::steady_clock::now();
      try {
        auto result = solver_pool.solve(
            {solver, map, scenario, {}, options.timeout_seconds});
        const auto end = std::chrono::steady_clock::now();
        const auto runtime_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();

        if (result.solution) {
          result.solution->map = map_name;
          result.solution->scenario = scenario_name;
          result.solution->algo = solver_name;
          result.solution->time_ms = runtime_ms;
          status = result.solution->status;
          mapf::writer::write_solution(solution_path, *result.solution);
          written_solution = solution_output_name;
        }

        write_metadata(metadata_csv,
                       RunMetadata{
                           .map = map_name,
                           .scenario = scenario_name,
                           .solver = solver_name,
                           .num_agents = scenario.size(),
                           .map_width = map.width,
                           .map_height = map.height,
                           .open_spaces = open_spaces,
                           .obstacles = obstacles,
                           .timeout_seconds = options.timeout_seconds,
                           .memory_limit_mb = options.memory_limit_mb,
                           .runtime_ms = runtime_ms,
                           .status = status,
                           .solution_file = written_solution,
                           .error = {},
                       });
      } catch (const std::exception &e) {
        const auto end = std::chrono::steady_clock::now();
        const auto runtime_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();
        std::cerr << "[ERROR] " << solver_name << " failed: " << e.what()
                  << std::endl;

        write_metadata(metadata_csv,
                       RunMetadata{
                           .map = map_name,
                           .scenario = scenario_name,
                           .solver = solver_name,
                           .num_agents = scenario.size(),
                           .map_width = map.width,
                           .map_height = map.height,
                           .open_spaces = open_spaces,
                           .obstacles = obstacles,
                           .timeout_seconds = options.timeout_seconds,
                           .memory_limit_mb = options.memory_limit_mb,
                           .runtime_ms = runtime_ms,
                           .status = "Error",
                           .solution_file = {},
                           .error = e.what(),
                       });
      }
    }
  }
}

void run(const BenchmarkOptions &options,
         const std::vector<ManifestProblem> &problems,
         mapf::solvers::SolverPool &solver_pool) {
  const std::filesystem::path level_output = options.output_path / "level0";
  const std::filesystem::path solution_output = level_output / "solutions";
  const std::filesystem::path csv_path = level_output / "metadata.csv";

  std::filesystem::create_directories(solution_output);

  std::ofstream metadata_csv(csv_path, std::ios::trunc);
  if (!metadata_csv) {
    throw std::runtime_error("Failed to initialize level 0 metadata file");
  }
  metadata_csv << "map,scenario,solver,num_agents,map_width,map_height,open_"
                  "spaces,obstacles,timeout_seconds,memory_limit_mb,runtime_ms,"
                  "status,solution_file,error\n";

  for (const auto &problem : problems) {
    try {
      run_problem(solver_pool, problem, solution_output, metadata_csv, options);
    } catch (const std::exception &e) {
      std::cerr << "[ERROR] problem exception: " << e.what() << std::endl;
    }
  }

  std::cout << "Benchmarking finished. Results saved to: " << level_output
            << std::endl;
}
} // namespace level0

void run_benchmark(const BenchmarkOptions &options,
                   const std::vector<ManifestProblem> &problems,
                   mapf::solvers::SolverPool &solver_pool) {
  switch (options.level) {
  case BenchmarkLevel::level0:
    level0::run(options, problems, solver_pool);
    return;
  }

  throw std::invalid_argument("Unsupported benchmark level");
}
} // namespace

int main(int argc, char **argv) {
  std::cout << "mapf benchmarker" << std::endl;

  try {
    const BenchmarkOptions options = parse_options(argc, argv);
    mapf::solvers::SolverPool pool{
        MAPF_CBSH_WORKER_PATH, MAPF_CBSH_WORKER_PATH, MAPF_BCP_WORKER_PATH,
        mapf::solvers::WorkerOptions{
            std::chrono::seconds{options.timeout_seconds},
            options.memory_limit_mb}};
    const auto problems = parse_manifest(options.manifest_path);

    run_benchmark(options, problems, pool);
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
