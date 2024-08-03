#pragma once

#include "benchmark_base.h"
#include "benchmark_suite.h"

struct ThroughputSingleRunResult
{
  double avg_per_msg_ns;
  size_t total_msg_num;

  friend std::ostream& operator<<(std::ostream& o, ThroughputSingleRunResult s)
  {
    o << std::fixed << std::setprecision(5) << s.total_msg_num << "," << s.avg_per_msg_ns << "\n";
    return o;
  }
};

class ThroughputBenchmarkSuite : public BenchmarkSuiteBase<ThroughputSingleRunResult>
{
public:
  using Base = BenchmarkSuiteBase<ThroughputSingleRunResult>;
  using Base::BenchmarkStats;
  using Base::BenchmarkSuiteBase;
  using Base::csv_header;
  using Base::go;
  using BenchmarkRunResult = ThroughputSingleRunResult;

protected:
  std::vector<BenchmarkStats> calc_summary(typename Base::BenchmarkResultsMap& benchmark_results) override
  {
    std::vector<BenchmarkStats> result;
    for (auto& per_benchmark : benchmark_results)
    {
      std::sort(begin(per_benchmark.second.runs), end(per_benchmark.second.runs),
                [&](const ThroughputSingleRunResult& left, const ThroughputSingleRunResult& right)
                {
                  if (left.total_msg_num != right.total_msg_num)
                  {
                    throw std::runtime_error(
                      std::string("benchmark [").append(per_benchmark.first).append("] had CRITICAL failures as not all messages were published/consumed - queue appear to have bugs..."));
                  }

                  return left.avg_per_msg_ns < right.avg_per_msg_ns;
                });

      BenchmarkStats s;
      const std::vector<ThroughputSingleRunResult>& run_stats = per_benchmark.second.runs;
      {
        s.benchmark_name = per_benchmark.first;
        s.N = run_stats.front().total_msg_num;
        s.iteration_num = per_benchmark.second.runs.size();
        s.msg_type_name = per_benchmark.second.msg_type_name;
        s.producer_num = per_benchmark.second.producer_num;
        s.consumer_num = per_benchmark.second.consumer_num;
        s.min_ns = run_stats.front().avg_per_msg_ns;
        s.max_ns = run_stats.back().avg_per_msg_ns;
        // a bit crude percentile count, but it would work if iteration num is high enough >= 100
        s.d50_ns = run_stats[run_stats.size() * 0.5].avg_per_msg_ns;
        s.d75_ns = run_stats[run_stats.size() * 0.75].avg_per_msg_ns;
        s.d90_ns = run_stats[run_stats.size() * 0.9].avg_per_msg_ns;
        s.d99_ns = run_stats[run_stats.size() * 0.99].avg_per_msg_ns;
        result.push_back(s);
      }
    }

    return result;
  }
};

template <class T, class BenchmarkContext, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_, class ProduceAllMessage, class ConsumeAllMessage>
class ThroughputBenchmark : public BenchmarkBase<ThroughputSingleRunResult>
{
  using Base = BenchmarkBase<ThroughputSingleRunResult>;

  std::deque<std::jthread> producers_threads_;
  std::deque<std::jthread> consumers_threads_;

  std::vector<std::size_t> producer_cores_;
  std::vector<std::size_t> consumer_cores_;

  BenchmarkContext ctx_;

  static_assert(std::atomic<std::chrono::system_clock::time_point>::is_always_lock_free);

public:
  template <class QueueConfig>
  ThroughputBenchmark(const std::string& name, QueueConfig config,
                      const std::vector<std::size_t>& producer_cores = {},
                      const std::vector<std::size_t>& consumer_cores = {})
    : Base(name), producer_cores_(producer_cores), consumer_cores_(consumer_cores), ctx_(config)
  {
  }

  std::string msg_type_name() const override { return typeid(T).name(); }
  size_t producer_num() const override { return _PRODUCER_N_; }
  size_t consumer_num() const override { return _CONSUMER_N_; }

  ThroughputSingleRunResult go(size_t N) override
  {
    std::atomic<std::chrono::system_clock::time_point> start_time_ns;
    std::atomic<std::chrono::system_clock::time_point> end_time_ns;

    std::atomic_uint64_t producers_ready_num{0};
    std::atomic_uint64_t consumers_ready_num{0};

    for (size_t producer_id = 0; producer_id < _PRODUCER_N_; ++producer_id)
    {
      producers_threads_.emplace_back(
        [&]()
        {
          ++producers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          typename ProduceAllMessage::message_creator mc;
          start_time_ns.store(std::chrono::system_clock::now());
          ProduceAllMessage()(N, ctx_, mc);
        });
    }

    for (size_t consumer_id = 0; consumer_id < _CONSUMER_N_; ++consumer_id)
    {
      consumers_threads_.emplace_back(
        [&]()
        {
          ++consumers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          typename ConsumeAllMessage::message_consumer mp;
          ConsumeAllMessage()(N, ctx_, mp);
        });
    }

    for (auto& t : producers_threads_)
      t.join();

    for (auto& t : consumers_threads_)
      t.join();

    ThroughputSingleRunResult summary;
    end_time_ns = std::chrono::system_clock::now();
    summary.avg_per_msg_ns =
      std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() / (double)N;
    summary.total_msg_num = N;
    return summary;
  }
};
