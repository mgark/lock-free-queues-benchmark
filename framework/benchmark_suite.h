#pragma once

#include "benchmark_base.h"
#include <iostream>

template <class SingleRunResult, class BenchmarkStats>
class BenchmarkSuiteBase
{
public:
  BenchmarkSuiteBase(size_t iteration_num,
                     std::initializer_list<std::function<std::unique_ptr<BenchmarkBase<SingleRunResult>>()>> creators)
    : benchmark_creators_(creators), iteration_num_(iteration_num)
  {
  }

  std::vector<BenchmarkStats> go(size_t N)
  {
    if (benchmark_creators_.empty())
      return {};

    std::random_device rd;
    for (size_t i = 0; i < iteration_num_; ++i)
    {
      // we want each benchmark to run exactly *iterations_num* but we
      // want them to run randomly between the benchmark calls
      auto benchmark_creators = benchmark_creators_;
      while (!benchmark_creators.empty())
      {
        std::uniform_int_distribution<int> dist(0, benchmark_creators.size() - 1);
        size_t bench_idx = dist(rd);
        auto benchmark = benchmark_creators.at(bench_idx)();
        auto& benchmark_result = benchmark_results_[benchmark->key()];
        benchmark_result.runs.emplace_back(benchmark->go(N));

        if (benchmark_result.msg_type_name.empty())
        {
          // let's populate benchmark metadata, but make sure it is called before we drop the benchmark
          benchmark_result.vendor = benchmark->vendor();
          benchmark_result.name = benchmark->name();
          benchmark_result.ring_buffer_sz = benchmark->ring_buffer_sz();
          benchmark_result.msg_type_name = benchmark->msg_type_name();
          benchmark_result.producer_num = benchmark->producer_num();
          benchmark_result.consumer_num = benchmark->consumer_num();
        }

        benchmark_creators.erase(begin(benchmark_creators) + bench_idx);
      }
    }

    return calc_summary(benchmark_results_);
  }

protected:
  std::vector<std::function<std::unique_ptr<BenchmarkBase<SingleRunResult>>()>> benchmark_creators_;

  struct BenchmarkResults
  {
    std::string name;
    std::string vendor;
    std::string msg_type_name;
    size_t producer_num;
    size_t consumer_num;
    size_t ring_buffer_sz;
    std::vector<SingleRunResult> runs;
  };

  using BenchmarkResultsMap = std::unordered_map<std::string /*benchmark name*/, BenchmarkResults>;
  BenchmarkResultsMap benchmark_results_;
  size_t iteration_num_;

  virtual std::vector<BenchmarkStats> calc_summary(BenchmarkResultsMap& reports) = 0;
};
