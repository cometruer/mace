//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <regex>
#include <vector>

#include "mace/core/testing/test_benchmark.h"
#include "mace/utils/env_time.h"
#include "mace/utils/logging.h"

namespace mace {
namespace testing {

static std::vector<Benchmark *> *all_benchmarks = nullptr;
static int64_t bytes_processed;
static int64_t macc_processed;
static int64_t accum_time = 0;
static int64_t start_time = 0;

Benchmark::Benchmark(const char *name, void (*benchmark_func)(int))
    : name_(name), benchmark_func_(benchmark_func) {
  Register();
}

// Run all benchmarks
void Benchmark::Run() { Run("all"); }

void Benchmark::Run(const char *pattern) {
  if (!all_benchmarks) return;

  if (std::string(pattern) == "all") {
    pattern = ".*";
  }
  std::regex regex(pattern);

  // Compute name width.
  int width = 10;
  char name[100];
  std::smatch match;
  for (auto b : *all_benchmarks) {
    if (!std::regex_match(b->name_, match, regex)) continue;
    width = std::max<int>(width, b->name_.length());
  }

  printf("%-*s %10s %10s %10s %10s\n", width, "Benchmark", "Time(ns)",
         "Iterations", "Input(MB/s)", "MACC(G/s)");
  printf("%s\n", std::string(width + 44, '-').c_str());
  for (auto b : *all_benchmarks) {
    if (!std::regex_match(b->name_, match, regex)) continue;
    int iters;
    double seconds;
    b->Run(&iters, &seconds);
    float mbps = (bytes_processed * 1e-6) / seconds;
    // MACCs or other computations
    float gmaccs = (macc_processed * 1e-9) / seconds;
    printf("%-*s %10.0f %10d %10.2f %10.2f\n", width, b->name_.c_str(),
           seconds * 1e9 / iters, iters, mbps, gmaccs);
  }
}

void Benchmark::Register() {
  if (!all_benchmarks) all_benchmarks = new std::vector<Benchmark *>;
  all_benchmarks->push_back(this);
}

void Benchmark::Run(int *run_count, double *run_seconds) {
  static const int64_t kMinIters = 10;
  static const int64_t kMaxIters = 1000000000;
  static const double kMinTime = 0.5;
  int64_t iters = kMinIters;
  while (true) {
    bytes_processed = -1;
    macc_processed = -1;
    RestartTiming();
    (*benchmark_func_)(iters);
    StopTiming();
    const double seconds = accum_time * 1e-6;
    if (seconds >= kMinTime || iters >= kMaxIters) {
      *run_count = iters;
      *run_seconds = seconds;
      return;
    }

    // Update number of iterations.
    // Overshoot by 100% in an attempt to succeed the next time.
    double multiplier = 2.0 * kMinTime / std::max(seconds, 1e-9);
    iters = std::min<int64_t>(multiplier * iters, kMaxIters);
  }
}

void BytesProcessed(int64_t n) { bytes_processed = n; }
void MaccProcessed(int64_t n) { macc_processed = n; }
void RestartTiming() {
  accum_time = 0;
  start_time = NowMicros();
}
void StartTiming() {
  start_time = NowMicros();
}
void StopTiming() {
  if (start_time != 0) {
    accum_time += (NowMicros() - start_time);
    start_time = 0;
  }
}

}  // namespace testing
}  // namespace mace
