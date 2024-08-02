#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "utils.h"
#include <atomic_queue/atomic_queue.h>
#include <functional>
#include <iomanip>
#include <list>
#include <mpmc.h>
#include <random>

template <class SummaryReport>
class BenchmarkBase
{
public:
  using summary_type = SummaryReport;

  BenchmarkBase() {}
  virtual std::string name() = 0;
  virtual ~BenchmarkBase() = default;
  virtual SummaryReport go(size_t N) = 0;
};

template <class ConcreteBenchmark, class BenchmarkRunResult, class... T>
static std::function<std::unique_ptr<BenchmarkBase<BenchmarkRunResult>>()> benchmark_creator(
  T... params) requires(std::is_same_v<BenchmarkRunResult, typename ConcreteBenchmark::summary_type>)
{
  return [=]() { return std::make_unique<ConcreteBenchmark>(params...); };
}

template <class BenchmarkRunResult>
class BenchmarkSuiteBase
{
public:
  struct Summary
  {
    std::string benchmark_name;
    size_t iterations;
    size_t N;
    double min_ns;
    double max_ns;
    double d50_ns;
    double d75_ns;
    double d90_ns;
    double d99_ns;

    friend std::ostream& operator<<(std::ostream& o, Summary s)
    {
      o << s.benchmark_name << "," << s.iterations << "," << s.N << "," << std::fixed
        << std::setprecision(5) << s.min_ns << "," << s.max_ns << "," << s.d50_ns << "," << s.d75_ns
        << "," << s.d90_ns << "," << s.d99_ns << "\n";
      return o;
    }

    friend std::ostream& operator<<(std::ostream& o, const std::vector<Summary>& ss)
    {
      for (const auto& s : ss)
        o << s;
      return o;
    }
  };

  static const char* csv_header() { return "iterations,N,min_ns,max_ns,50_ns,75_ns,90_ns,99_ns\n"; }
  BenchmarkSuiteBase(size_t iteration_num,
                     std::initializer_list<std::function<std::unique_ptr<BenchmarkBase<BenchmarkRunResult>>()>> creators)
    : creators_(creators), iteration_num_(iteration_num)
  {
  }

  std::vector<Summary> go(size_t N)
  {
    if (creators_.empty())
      return {};

    std::random_device rd;
    for (size_t i = 0; i < iteration_num_; ++i)
    {
      // we want each benchmark to run exactly *iterations_num* but we
      // want them to run randomly between the benchmark calls
      auto creators = creators_;
      while (!creators.empty())
      {
        std::uniform_int_distribution<int> dist(0, creators.size() - 1);
        size_t bench_idx = dist(rd);
        auto benchmark = creators.at(bench_idx)();
        reports_[benchmark->name()].emplace_back(benchmark->go(N));
        creators.erase(begin(creators) + bench_idx);
      }
    }

    return calc_summary(reports_);
  }

protected:
  std::vector<std::function<std::unique_ptr<BenchmarkBase<BenchmarkRunResult>>()>> creators_;
  using BenchmarkToSummaryMap =
    std::unordered_map<std::string /*benchmark name*/, std::vector<BenchmarkRunResult>>;
  BenchmarkToSummaryMap reports_;
  size_t iteration_num_;

  virtual std::vector<Summary> calc_summary(BenchmarkToSummaryMap& reports) = 0;
};

struct ThroughputBenchmarkRunResult
{
  double avg_per_msg_ns;
  size_t total_msg_num;

  friend std::ostream& operator<<(std::ostream& o, ThroughputBenchmarkRunResult s)
  {
    o << std::fixed << std::setprecision(5) << s.total_msg_num << "," << s.avg_per_msg_ns << "\n";
    return o;
  }
};

class ThroughputBenchmarkSuite : BenchmarkSuiteBase<ThroughputBenchmarkRunResult>
{
public:
  using Base = BenchmarkSuiteBase<ThroughputBenchmarkRunResult>;
  using Base::BenchmarkSuiteBase;
  using Base::csv_header;
  using Base::go;
  using Base::Summary;
  using BenchmarkRunResult = ThroughputBenchmarkRunResult;

protected:
  std::vector<typename Base::Summary> calc_summary(typename Base::BenchmarkToSummaryMap& reports) override
  {
    std::vector<typename Base::Summary> result;
    for (auto& per_benchmark : reports)
    {
      std::sort(begin(per_benchmark.second), end(per_benchmark.second),
                [&](const ThroughputBenchmarkRunResult& left, const ThroughputBenchmarkRunResult& right)
                {
                  if (left.total_msg_num != right.total_msg_num)
                  {
                    throw std::runtime_error(
                      std::string("benchmark [").append(per_benchmark.first).append("] had CRITICAL failures as not all messages were published/consumed - queue appear to have bugs..."));
                  }

                  return left.avg_per_msg_ns < right.avg_per_msg_ns;
                });

      Summary s;
      std::vector<ThroughputBenchmarkRunResult>& run_stats = per_benchmark.second;
      {
        s.benchmark_name = per_benchmark.first;
        s.N = run_stats.front().total_msg_num;
        s.iterations = this->Base::iteration_num_;
        s.min_ns = run_stats.front().avg_per_msg_ns;
        s.max_ns = run_stats.back().avg_per_msg_ns;
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

template <class T, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_, class QueueConfig, class ProduceOneMessage, class ConsumeOneMessage>
class ThroughputBenchmarkBase : public BenchmarkBase<ThroughputBenchmarkRunResult>
{
public:
};

struct MgarkQueueConfig
{
  size_t ring_buffer_sz;
};

template <class T, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_, class QueueConfig, class ProduceOneMessage, class ConsumeOneMessage>
class ThroughputBenchmark
  : public ThroughputBenchmarkBase<T, _PRODUCER_N_, _CONSUMER_N_, QueueConfig, ProduceOneMessage, ConsumeOneMessage>
{
  std::deque<std::jthread> producers_;
  std::deque<std::jthread> consumers_;
  size_t N_;

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_>;
  ThroughputBenchmarkRunResult summary_;
  QueueConfig queue_config_;
  QueueType mpmc_queue_;

  static_assert(std::atomic<std::chrono::system_clock::time_point>::is_always_lock_free);

public:
  ThroughputBenchmark(QueueConfig config, const std::vector<std::size_t>& producer_cores = {},
                      const std::vector<std::size_t>& consumer_cores = {})
    : queue_config_(config), mpmc_queue_(queue_config_.ring_buffer_sz)
  {
  }

  ThroughputBenchmarkRunResult summary() const { return summary_; }

  ~ThroughputBenchmark() {}

  std::string name() override { return "mgark_queue"; }
  ThroughputBenchmarkRunResult go(size_t N) override
  {
    std::atomic<std::chrono::system_clock::time_point> start_time_ns;
    std::atomic<std::chrono::system_clock::time_point> end_time_ns;

    std::atomic_uint64_t producers_ready_num{0};
    std::atomic_uint64_t consumers_ready_num{0};

    for (size_t producer_id = 0; producer_id < _PRODUCER_N_; ++producer_id)
    {
      producers_.emplace_back(
        [&]()
        {
          ++producers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          ProducerBlocking<QueueType> p(mpmc_queue_);
          ProduceOneMessage message_creator_;
          mpmc_queue_.start();

          size_t i = 0;
          start_time_ns.store(std::chrono::system_clock::now());
          while (i < N)
          {
            p.emplace(message_creator_());
            ++i;
          }
        });
    }

    for (size_t consumer_id = 0; consumer_id < _CONSUMER_N_; ++consumer_id)
    {
      consumers_.emplace_back(
        [&]()
        {
          ++consumers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          ConsumerBlocking<QueueType> c(mpmc_queue_);
          ConsumeOneMessage message_processor_;
          size_t i = 0;
          while (i < N)
          {
            if (ConsumeReturnCode::Consumed == c.consume([&](const T& m) { message_processor_(m); }))
              ++i;
          }
        });
    }

    for (auto& t : producers_)
      t.join();

    for (auto& t : consumers_)
      t.join();

    end_time_ns = std::chrono::system_clock::now();
    summary_.avg_per_msg_ns =
      std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() / (double)N;
    summary_.total_msg_num = N;
    return summary_;
  }
};

template <class T, T NIL_VAL = -1>
struct AtomicQueueConfig
{
  size_t capacity;
  constexpr static T NIL = NIL_VAL;
};

template <class T, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_, class QueueConfig, class ProduceOneMessage, class ConsumeOneMessage>
class ThroughputBenchmark2
  : public ThroughputBenchmarkBase<T, _PRODUCER_N_, _CONSUMER_N_, QueueConfig, ProduceOneMessage, ConsumeOneMessage>

{
  std::deque<std::jthread> producers_;
  std::deque<std::jthread> consumers_;

  using Queue = atomic_queue::AtomicQueueB<T, std::allocator<T>, QueueConfig::NIL, true, false, true>;

  Queue queue_;
  ThroughputBenchmarkRunResult summary_;
  QueueConfig queue_config_;

  static_assert(std::atomic<std::chrono::system_clock::time_point>::is_always_lock_free);

public:
  ThroughputBenchmark2(QueueConfig config, const std::vector<std::size_t>& producer_cores = {},
                       const std::vector<std::size_t>& consumer_cores = {})
    : queue_config_(config), queue_(queue_config_.capacity)
  {
  }

  std::string name() override { return "atomic_queue"; }
  ThroughputBenchmarkRunResult summary() const { return summary_; }

  ~ThroughputBenchmark2() {}

  ThroughputBenchmarkRunResult go(size_t N) override
  {
    std::atomic<std::chrono::system_clock::time_point> start_time_ns;
    std::atomic<std::chrono::system_clock::time_point> end_time_ns;

    std::atomic_uint64_t producers_ready_num{0};
    std::atomic_uint64_t consumers_ready_num{0};

    for (size_t producer_id = 0; producer_id < _PRODUCER_N_; ++producer_id)
    {
      producers_.emplace_back(
        [&]()
        {
          ++producers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          size_t i = 0;
          ProduceOneMessage message_creator_;
          start_time_ns.store(std::chrono::system_clock::now());
          while (i < N)
          {
            queue_.push(message_creator_());
            ++i;
          }
        });
    }

    for (size_t consumer_id = 0; consumer_id < _CONSUMER_N_; ++consumer_id)
    {
      consumers_.emplace_back(
        [&]()
        {
          ++consumers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
          }

          ConsumeOneMessage message_processor_;
          size_t i = 0;
          while (i < N)
          {
            message_processor_(queue_.pop());
            ++i;
          }
        });
    }

    for (auto& t : producers_)
      t.join();

    for (auto& t : consumers_)
      t.join();

    end_time_ns = std::chrono::system_clock::now();
    summary_.avg_per_msg_ns =
      std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() / (double)N;
    summary_.total_msg_num = N;

    return summary_;
  }
};