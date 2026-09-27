// minitub_bench: runs one workload and writes a JSON result.
//   minitub_bench --workload=dummy --threads=4 --seconds=10 --config=configs/default.toml --out=r.json
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <CLI/CLI.hpp>

#include "common/engine_config.h"
#include "common/exception.h"
#include "report.h"
#include "runner.h"
#include "workload.h"

using minitub::EngineConfig;
namespace bench = minitub::bench;

auto main(int argc, char **argv) -> int {
  CLI::App app{"MiniTub benchmark harness"};
  std::string workload_name;
  std::string config_path;
  std::string out_path;
  bench::RunOptions options;
  app.add_option("--workload", workload_name, "Workload to run")
      ->required()
      ->check(CLI::IsMember(bench::WorkloadNames()));
  app.add_option("--config", config_path, "EngineConfig TOML file (default: built-in defaults)")
      ->check(CLI::ExistingFile);
  app.add_option("--threads", options.threads, "Worker threads")->check(CLI::Range(1, 1024))->capture_default_str();
  app.add_option("--seconds", options.seconds, "Run time")->check(CLI::PositiveNumber)->capture_default_str();
  app.add_option("--seed", options.seed, "Random seed")->capture_default_str();
  app.add_option("--out", out_path, "Result JSON file (default: stdout)");
  CLI11_PARSE(app, argc, argv);

  if (std::string(MINITUB_BUILD_TYPE) != "Release") {
    std::cerr << "warning: " << MINITUB_BUILD_TYPE << " build; use the release preset for real numbers\n";
  }

  try {
    EngineConfig config = config_path.empty() ? EngineConfig{} : EngineConfig::FromFile(config_path);
    auto workload = bench::MakeWorkload(workload_name);
    workload->Setup(config, options.threads);
    bench::RunResult result = bench::Run(*workload, options);

    std::string json = bench::ToJson({workload_name, options, &config, &result});
    if (out_path.empty()) {
      std::cout << json;
    } else {
      std::ofstream(out_path) << json;
    }
    std::fprintf(stderr, "%s: %lld ops in %.2fs = %.0f ops/s, p99 %lld ns\n", workload_name.c_str(),
                 static_cast<long long>(result.total_ops), result.elapsed_seconds,
                 static_cast<double>(result.total_ops) / result.elapsed_seconds,
                 static_cast<long long>(result.latency_ns.Percentile(99.0)));
  } catch (const minitub::Exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
