#pragma once

#include "benchmark_base.h"
#include "benchmark_suite.h"
#include "factory.h"
#include "utils.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <type_traits>

struct LatencySingleRunResult
{
  double round_trip_latency_ns_AVG;
  size_t total_msg_num;
  size_t thread_num;

  friend std::ostream& operator<<(std::ostream& o, LatencySingleRunResult s)
  {
    o << std::fixed << std::setprecision(5) << s.total_msg_num << "," << s.round_trip_latency_ns_AVG
      << "," << s.thread_num << "\n";
    return o;
  }
};

struct LatencyBenchmarkStats
{
  std::string benchmark_name;
  std::string vendor;
  size_t ring_buffer_sz;
  size_t iteration_num;
  size_t N;
  std::string msg_type_name;
  size_t thread_num;
  size_t producer_num;
  size_t consumer_num;
  size_t min;
  size_t max;
  size_t d50;
  size_t d75;
  size_t d90;
  size_t d99;

  static const char* csv_header()
  {
    return "name,vendor,ring_buffer_sz,iteration_n,msg_n,msg_type,thread_num,producer_n,consumer_n,"
           "min_msg_"
           "ns,max_msg_ns,50_msg_"
           "ns,75_"
           "msg_ns"
           ",90_msg_ns,99_msg_ns\n";
  }

  friend std::ostream& operator<<(std::ostream& o, LatencyBenchmarkStats s)
  {
    o << s.benchmark_name << "," << s.vendor << "," << s.ring_buffer_sz << "," << s.iteration_num
      << "," << s.N << "," << s.msg_type_name << "," << s.thread_num << "," << s.producer_num << ","
      << s.consumer_num << "," << std::fixed << std::setprecision(5) << s.min << "," << s.max << ","
      << s.d50 << "," << s.d75 << "," << s.d90 << "," << s.d99 << "\n";
    return o;
  }

  friend std::ostream& operator<<(std::ostream& o, const std::vector<LatencyBenchmarkStats>& ss)
  {
    for (const auto& s : ss)
      o << s;
    return o;
  }
};

class LatencyBenchmarkSuite : public BenchmarkSuiteBase<LatencySingleRunResult, LatencyBenchmarkStats>
{
public:
  using Base = BenchmarkSuiteBase<LatencySingleRunResult, LatencyBenchmarkStats>;
  using Base::BenchmarkSuiteBase;
  using Base::go;
  using BenchmarkRunResult = LatencySingleRunResult;

protected:
  std::vector<LatencyBenchmarkStats> calc_summary(typename Base::BenchmarkResultsMap& benchmark_results) override
  {
    if (this->iteration_num_ < 100)
    {
      throw std::runtime_error(
        "please configure iteration_num >= 100, otherwise percentile stats would be highly "
        "inaccurate");
    }

    std::vector<LatencyBenchmarkStats> result;
    for (auto& per_benchmark : benchmark_results)
    {
      std::sort(begin(per_benchmark.second.runs), end(per_benchmark.second.runs),
                [&](const LatencySingleRunResult& left, const LatencySingleRunResult& right)
                {
                  if (left.total_msg_num != right.total_msg_num)
                  {
                    throw std::runtime_error(
                      std::string("benchmark [").append(per_benchmark.first).append("] had CRITICAL failures as not all messages were published/consumed - queue appear to have bugs..."));
                  }

                  return left.round_trip_latency_ns_AVG < right.round_trip_latency_ns_AVG;
                });

      LatencyBenchmarkStats s;
      const std::vector<LatencySingleRunResult>& run_stats = per_benchmark.second.runs;
      {
        s.benchmark_name = per_benchmark.second.name;
        s.N = run_stats.front().total_msg_num;
        s.iteration_num = per_benchmark.second.runs.size();

        s.vendor = per_benchmark.second.vendor;
        s.ring_buffer_sz = per_benchmark.second.ring_buffer_sz;

        s.msg_type_name = per_benchmark.second.msg_type_name;
        s.producer_num = per_benchmark.second.producer_num;
        s.consumer_num = per_benchmark.second.consumer_num;
        s.thread_num = per_benchmark.second.runs.front().thread_num;

        s.min = run_stats.front().round_trip_latency_ns_AVG;
        s.max = run_stats.back().round_trip_latency_ns_AVG;

        // a bit crude percentile count, but it would work if iteration num is high enough >= 100
        s.d50 = run_stats[run_stats.size() * 0.5].round_trip_latency_ns_AVG;
        s.d75 = run_stats[run_stats.size() * 0.75].round_trip_latency_ns_AVG;
        s.d90 = run_stats[run_stats.size() * 0.9].round_trip_latency_ns_AVG;
        s.d99 = run_stats[run_stats.size() * 0.99].round_trip_latency_ns_AVG;

        result.push_back(s);
      }
    }

    return result;
  }
};

template <class T, class BenchmarkContext, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_,
          std::size_t _THREAD_N_, class LatencyA, class LatencyB, bool multicast_consumers = false>
class LatencyBenchmark : public BenchmarkBase<LatencySingleRunResult>
{
  static_assert(_PRODUCER_N_ >= _THREAD_N_);
  static_assert(_CONSUMER_N_ >= _THREAD_N_);

  using Base = BenchmarkBase<LatencySingleRunResult>;

  std::deque<std::jthread> a_threads_;
  std::deque<std::jthread> b_threads_;

  std::vector<std::size_t> a_cores_;
  std::vector<std::size_t> b_cores_;

  BenchmarkContext a_ctx_;
  BenchmarkContext b_ctx_;

  static_assert(std::atomic<std::chrono::system_clock::time_point>::is_always_lock_free);

public:
  LatencyBenchmark(const std::string& name, size_t ring_buffer_sz,
                   const std::vector<std::size_t>& a_cores = {}, const std::vector<std::size_t>& b_cores = {})
    : Base(name, BenchmarkContext::VENDOR, ring_buffer_sz),
      a_cores_(a_cores),
      b_cores_(b_cores),
      a_ctx_(ring_buffer_sz),
      b_ctx_(ring_buffer_sz)
  {
    assert(a_cores_.size() == b_cores_.size());
  }

  std::string msg_type_name() const override { return typeid(T).name(); }

  size_t producer_num() const override { return _PRODUCER_N_; }
  size_t consumer_num() const override { return _CONSUMER_N_; }

  LatencySingleRunResult go(size_t N) override
  {
    using ProducerMsgCreator = typename LatencyA::message_creator;
    using ConsumerMsgProcessor = typename LatencyA::message_processor;

    std::mutex guard;
    std::atomic<std::chrono::system_clock::time_point> start_time_ns{};
    std::atomic<std::chrono::system_clock::time_point> end_time_ns;

    std::atomic_uint64_t a_ready_num{0};
    std::atomic_uint64_t b_ready_num{0};
    std::atomic_uint64_t a_total_iteration_num{0};
    std::atomic_uint64_t b_total_iteration_num{0};

    size_t per_consumer_num;
    size_t total_consume_num;

    size_t per_thread_num{N / _PRODUCER_N_};
    static_assert(!multicast_consumers);

    std::vector<std::unique_ptr<LatencyA>> a_collection_;
    for (size_t thread_idx = 0; thread_idx < _THREAD_N_; ++thread_idx)
    {
      a_threads_.emplace_back(
        [&, idx = thread_idx]()
        {
          auto a = std::make_unique<LatencyA>(a_ctx_, b_ctx_); // it is important to run this before increment below!

          ++a_ready_num;
          while (a_ready_num.load() < _THREAD_N_ || b_ready_num.load() < _THREAD_N_)
          {
            // all A and B threads must indicate that they are ready!
          }

          std::chrono::system_clock::time_point expected_time{};
          auto now = std::chrono::system_clock::now();
          start_time_ns.compare_exchange_strong(expected_time, now, std::memory_order_release);

          ProducerMsgCreator mc;
          ConsumerMsgProcessor mp;
          size_t iterations_num = (*a)(idx, per_thread_num, a_ctx_, mc, mp);
          a_total_iteration_num.fetch_add(iterations_num);

          std::unique_lock autolock(guard);
          a_collection_.emplace_back(std::move(a));
        });
    }

    std::vector<std::unique_ptr<LatencyB>> b_collection_;
    for (size_t thread_idx = 0; thread_idx < _THREAD_N_; ++thread_idx)
    {
      b_threads_.emplace_back(
        [&]()
        {
          auto b = std::make_unique<LatencyB>(a_ctx_, b_ctx_); // it is important to run this before increment below!

          ++b_ready_num;
          while (a_ready_num.load() < _THREAD_N_ || b_ready_num.load() < _THREAD_N_)
          {
            // all A and B threads must indicate that they are ready!
          }

          ProducerMsgCreator mc;
          ConsumerMsgProcessor mp;
          size_t iterations_num = (*b)(per_thread_num, b_ctx_, mc, mp);
          b_total_iteration_num.fetch_add(iterations_num);

          std::unique_lock autolock(guard);
          b_collection_.emplace_back(std::move(b));
        });
    }

    for (auto& t : a_threads_)
      t.join();

    for (auto& t : b_threads_)
      t.join();

    if (a_total_iteration_num != b_total_iteration_num)
    {
      std::stringstream ss;
      ss << "a_total_iteration_num[" << a_total_iteration_num
         << "] is not equal b_total_iteration_num [" << b_total_iteration_num << "]";
      throw std::runtime_error(ss.str());
    }

    end_time_ns = std::chrono::system_clock::now();

    LatencySingleRunResult summary;
    {
      summary.round_trip_latency_ns_AVG =
        std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() /
        static_cast<double>(a_total_iteration_num);
      summary.total_msg_num = a_total_iteration_num;
      summary.thread_num = _THREAD_N_;
    }

    return summary;
  }
};
