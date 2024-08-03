#pragma once

#include "benchmark_base.h"

template <class SingleRunResult>
class BenchmarkSuiteBase
{
public:
  struct BenchmarkStats
  {
    std::string benchmark_name;
    size_t iteration_num;
    size_t N;
    std::string msg_type_name;
    size_t producer_num;
    size_t consumer_num;
    double min_ns;
    double max_ns;
    double d50_ns;
    double d75_ns;
    double d90_ns;
    double d99_ns;

    friend std::ostream& operator<<(std::ostream& o, BenchmarkStats s)
    {
      o << s.benchmark_name << "," << s.iteration_num << "," << s.msg_type_name << ","
        << s.producer_num << "," << s.consumer_num << "," << s.N << "," << std::fixed
        << std::setprecision(5) << s.min_ns << "," << s.max_ns << "," << s.d50_ns << "," << s.d75_ns
        << "," << s.d90_ns << "," << s.d99_ns << "\n";
      return o;
    }

    friend std::ostream& operator<<(std::ostream& o, const std::vector<BenchmarkStats>& ss)
    {
      for (const auto& s : ss)
        o << s;
      return o;
    }
  };

  static const char* csv_header()
  {
    return "iteration_n,msg_n,msg_type,producer_n,consumer_n,min_ns,max_ns,50_ns,75_ns,90_ns,99_"
           "ns\n";
  }

  BenchmarkSuiteBase(size_t iteration_num,
                     std::initializer_list<std::function<std::unique_ptr<BenchmarkBase<SingleRunResult>>()>> creators)
    : benchmark_creators_(creators), iteration_num_(iteration_num)
  {
    if (iteration_num < 100)
    {
      throw std::runtime_error(
        "please configure iteration_num >= 100, otherwise percentile stats would be highly "
        "inaccurate");
    }
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
        auto& benchmark_result = benchmark_results_[benchmark->name()];
        benchmark_result.runs.emplace_back(benchmark->go(N));

        if (benchmark_result.msg_type_name.empty())
        {
          // let's populate benchmark metadata, but make sure it is called before we drop the benchmark
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
    std::string msg_type_name;
    size_t producer_num;
    size_t consumer_num;
    std::vector<SingleRunResult> runs;
  };

  using BenchmarkResultsMap = std::unordered_map<std::string /*benchmark name*/, BenchmarkResults>;
  BenchmarkResultsMap benchmark_results_;
  size_t iteration_num_;

  virtual std::vector<BenchmarkStats> calc_summary(BenchmarkResultsMap& reports) = 0;
};
