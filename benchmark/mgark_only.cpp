#include "../framework/benchmark_base.h"
#include "../framework/benchmark_round_trip_latency.h"
#include "../framework/benchmark_suite.h"
#include "../framework/benchmark_throughput.h"
#include "../framework/factory.h"
#include "atomic_queue_spec.h"
#include "detail/common.h"
#include "detail/single_bit_reuse.h"
#include "mgark_spec.h"
#include <concepts>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

int main()
{

  std::cout << ThroughputBenchmarkStats::csv_header();

  // SPSC Optmized  tests
  for (size_t RING_BUFFER_SIZE : {8, 64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 1024;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_int_reused_bit";
    constexpr size_t BATCH_NUM = 4;
    using MsgType = integral_msb_always_0<uint32_t>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, CONSUMER_N, PRODUCER_N, BATCH_NUM>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // SPSC  tests
  /*for (size_t RING_BUFFER_SIZE : {8, 64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 1024;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_int";
    constexpr size_t BATCH_NUM = 4;
    using MsgType = uint32_t;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, CONSUMER_N, PRODUCER_N, BATCH_NUM>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC  tests
  for (size_t RING_BUFFER_SIZE : {8, 64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 3;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpsc_int";
    constexpr size_t BATCH_NUM = 2;
    using MsgType = uint32_t;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPMC - Multicast consumers  tests
  for (size_t RING_BUFFER_SIZE : {8, 64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr bool MULTICAST_CONSUMERS = true;
    constexpr const char* BENCH_NAME = "mpmc_int_multicast";
    constexpr size_t BATCH_NUM = 2;
    using MsgType = uint32_t;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>, MULTICAST_CONSUMERS>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }*/

  return 0;
}