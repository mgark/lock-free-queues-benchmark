#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <ostream>
#include <thread>
#include <vector>

#include "utils.h"
#include <iomanip>
#include <mpmc.h>

struct ThroughputBenchmarkSummary
{
  double avg_per_msg_ns;
  size_t total_msg_num;

  static const char* csv_header() { return "total_msg_num,avg_time_per_msg_ns\n"; }
  friend std::ostream& operator<<(std::ostream& o, ThroughputBenchmarkSummary s)
  {
    o << std::fixed << std::setprecision(5) << s.total_msg_num << "," << s.avg_per_msg_ns << "\n";
    return o;
  }
};

struct MgarkQueueConfig
{
  size_t N;
};

template <class T, std::size_t _PRODUCER_N_, std::size_t _CONSUMER_N_, class QueueConfig>
class ThroughputBenchmark
{
  std::deque<std::jthread> producers_;
  std::deque<std::jthread> consumers_;
  size_t N_;

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_>;
  ThroughputBenchmarkSummary summary_;
  QueueConfig queue_config_;
  QueueType mpmc_queue_;

  static_assert(std::atomic<std::chrono::system_clock::time_point>::is_always_lock_free);

public:
  ThroughputBenchmark(size_t N, QueueConfig config, const std::vector<std::size_t>& producer_cores = {},
                      const std::vector<std::size_t>& consumer_cores = {})
    : N_(N), queue_config_(config), mpmc_queue_(queue_config_.N)
  {
  }

  ThroughputBenchmarkSummary summary() const { return summary_; }

  ~ThroughputBenchmark() {}

  template <class ProduceOneMessage, class ConsumeOneMessage>
  ThroughputBenchmark& go()
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
          while (i < N_)
          {
            if (ProduceReturnCode::Published == p.emplace(message_creator_()))
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
          while (i < N_)
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
      std::chrono::nanoseconds{end_time_ns.load() - start_time_ns.load()}.count() / (double)N_;
    summary_.total_msg_num = N_;

    return *this;
  }
};