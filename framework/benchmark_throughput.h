#pragma once

#include "benchmark_base.h"
#include "benchmark_suite.h"
#include "factory.h"
#include "utils.h"
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <type_traits>

struct ThroughputSingleRunResult
{
  double msg_per_second;
  size_t total_msg_num;

  friend std::ostream& operator<<(std::ostream& o, ThroughputSingleRunResult s)
  {
    o << std::fixed << std::setprecision(5) << s.total_msg_num << "," << s.msg_per_second << "\n";
    return o;
  }
};

struct ThroughputBenchmarkStats
{
  std::string benchmark_name;
  std::string vendor;
  size_t ring_buffer_sz;
  size_t iteration_num;
  size_t N;
  std::string msg_type_name;
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
    return "name,vendor,ring_buffer_sz,iteration_n,msg_n,msg_type,producer_n,consumer_n,min_msg_"
           "sec,max_msg_sec,50_msg_"
           "sec,75_"
           "msg_sec"
           ",90_msg_sec,99_msg_sec\n";
  }

  friend std::ostream& operator<<(std::ostream& o, ThroughputBenchmarkStats s)
  {
    o << s.benchmark_name << "," << s.vendor << "," << s.ring_buffer_sz << "," << s.iteration_num
      << "," << s.N << "," << s.msg_type_name << "," << s.producer_num << "," << s.consumer_num
      << "," << std::fixed << std::setprecision(5) << s.min << "," << s.max << "," << s.d50 << ","
      << s.d75 << "," << s.d90 << "," << s.d99 << "\n";
    return o;
  }

  friend std::ostream& operator<<(std::ostream& o, const std::vector<ThroughputBenchmarkStats>& ss)
  {
    for (const auto& s : ss)
      o << s;
    return o;
  }
};

class ThroughputBenchmarkSuite : public BenchmarkSuiteBase<ThroughputSingleRunResult, ThroughputBenchmarkStats>
{
public:
  using Base = BenchmarkSuiteBase<ThroughputSingleRunResult, ThroughputBenchmarkStats>;
  using Base::BenchmarkSuiteBase;
  using Base::go;
  using BenchmarkRunResult = ThroughputSingleRunResult;

protected:
  std::vector<ThroughputBenchmarkStats> calc_summary(typename Base::BenchmarkResultsMap& benchmark_results) override
  {
    if (this->iteration_num_ < 100)
    {
      throw std::runtime_error(
        "please configure iteration_num >= 100, otherwise percentile stats would be highly "
        "inaccurate");
    }

    std::vector<ThroughputBenchmarkStats> result;
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

                  return left.msg_per_second > right.msg_per_second;
                });

      ThroughputBenchmarkStats s;
      const std::vector<ThroughputSingleRunResult>& run_stats = per_benchmark.second.runs;
      {
        s.benchmark_name = per_benchmark.second.name;
        s.N = run_stats.front().total_msg_num;
        s.iteration_num = per_benchmark.second.runs.size();

        s.vendor = per_benchmark.second.vendor;
        s.ring_buffer_sz = per_benchmark.second.ring_buffer_sz;

        s.msg_type_name = per_benchmark.second.msg_type_name;
        s.producer_num = per_benchmark.second.producer_num;
        s.consumer_num = per_benchmark.second.consumer_num;

        s.min = run_stats.back().msg_per_second;
        s.max = run_stats.front().msg_per_second;

        // a bit crude percentile count, but it would work if iteration num is high enough >= 100
        s.d50 = run_stats[run_stats.size() * 0.5].msg_per_second;
        s.d75 = run_stats[run_stats.size() * 0.75].msg_per_second;
        s.d90 = run_stats[run_stats.size() * 0.9].msg_per_second;
        s.d99 = run_stats[run_stats.size() * 0.99].msg_per_second;

        result.push_back(s);
      }
    }

    return result;
  }
};

template <class T, class BenchmarkContext, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_,
          class ProduceAllMessage, class ConsumeAllMessage, bool multicast_consumers = false>
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
  ThroughputBenchmark(const std::string& name, size_t ring_buffer_sz,
                      const std::vector<std::size_t>& producer_cores = {},
                      const std::vector<std::size_t>& consumer_cores = {})
    : Base(name, BenchmarkContext::VENDOR, ring_buffer_sz),
      producer_cores_(producer_cores),
      consumer_cores_(consumer_cores),
      ctx_(ring_buffer_sz)
  {
  }

  std::string msg_type_name() const override { return typeid(T).name(); }
  size_t producer_num() const override { return _PRODUCER_N_; }
  size_t consumer_num() const override { return _CONSUMER_N_; }

  ThroughputSingleRunResult go(size_t N) override
  {
    using ProducerMsgCreator = typename ProduceAllMessage::message_creator;
    using ConsumerMsgProcessor = typename ConsumeAllMessage::message_processor;

    std::atomic<std::chrono::system_clock::time_point> start_time_ns;
    std::atomic<std::chrono::system_clock::time_point> end_time_ns;

    std::atomic_uint64_t producers_ready_num{0};
    std::atomic_uint64_t consumers_ready_num{0};
    std::atomic_uint64_t total_msg_published{0};
    std::atomic_uint64_t total_msg_consumed{0};

    size_t per_producer_num{N / _PRODUCER_N_};
    size_t per_consumer_num;
    size_t total_consume_num;
    if constexpr (multicast_consumers)
    {
      per_consumer_num = per_producer_num * _PRODUCER_N_;
    }
    else
    {
      double tmp = (per_producer_num * _PRODUCER_N_) / (double)_CONSUMER_N_;
      per_consumer_num = tmp; // it would be approximate number, but as it gets rounded down, consumers would just under-consume 1 message in the worse case, but terminate gracefully!
    }

    total_consume_num = per_consumer_num * _CONSUMER_N_;
    for (size_t producer_id = 0; producer_id < _PRODUCER_N_; ++producer_id)
    {
      producers_threads_.emplace_back(
        [&]()
        {
          ++producers_ready_num;
          while (producers_ready_num.load() < _PRODUCER_N_ || consumers_ready_num.load() < _CONSUMER_N_)
          {
            // all producers and consumers must indicate that they are ready!
          }

          ProducerMsgCreator mc;
          start_time_ns.store(std::chrono::system_clock::now());
          size_t published_num = ProduceAllMessage()(per_producer_num, ctx_, mc);
          total_msg_published.fetch_add(published_num);
        });
    }

    for (size_t consumer_id = 0; consumer_id < _CONSUMER_N_; ++consumer_id)
    {
      consumers_threads_.emplace_back(
        [&]()
        {
          ConsumerMsgProcessor mp;
          auto actual_consumed_num = ConsumeAllMessage()(per_consumer_num, consumers_ready_num, ctx_, mp);
          total_msg_consumed.fetch_add(actual_consumed_num);
          if (actual_consumed_num != per_consumer_num)
          {
            std::stringstream ss;
            ss << "Consumer [" << consumer_id << "] should have consumed [" << per_consumer_num
               << "], but instead consumed [" << actual_consumed_num
               << "] which suggest a serious issue with queue's implementation";
            throw std::runtime_error(ss.str());
          }

          /*if constexpr (std::is_same_v<ConsumerMsgProcessor, ConsumeAndStore<T>>)
          {
            if constexpr (std::is_same_v<ProducerMsgCreator, ProduceIncremental<T>>)
            {
              if (mp.last_val < per_producer_num)
              {
                std::stringstream ss;
                ss << "Consumer [" << consumer_id << "] should has last value at least as ["
                   << per_producer_num << "], instead it got consumed [" << mp.last_val << "]";
                throw std::runtime_error(ss.str());
              }
            }
          }*/
        });
    }

    for (auto& t : producers_threads_)
      t.join();

    for (auto& t : consumers_threads_)
      t.join();

    if (total_consume_num != total_msg_consumed)
    {
      std::stringstream ss;
      ss << "total_msg_consumed[" << total_msg_consumed << "] not equal target number ["
         << total_consume_num << "]";
      throw std::runtime_error(ss.str());
    }

    ThroughputSingleRunResult summary;
    end_time_ns = std::chrono::system_clock::now();
    summary.msg_per_second = static_cast<double>(total_msg_published) /
      (std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() /
       static_cast<double>(NANO_PER_SEC));
    summary.total_msg_num = total_msg_published;
    return summary;
  }
};
