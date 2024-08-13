#include "../framework/benchmark_base.h"
#include "../framework/benchmark_suite.h"
#include "../framework/benchmark_throughput.h"
#include "../framework/factory.h"
#include <cstdint>
#include <iostream>
#include <limits>

#include "types/order_book.h"
#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"

int main()
{

  std::cout << ThroughputBenchmarkStats::csv_header();

  // SPSC  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_big_object";
    using MgarkMsgType = OrderBookOptimized;
    using MsgType = OrderBook;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AQ_NonAtomic_SPSCBoundedDynamicContext<MsgType>, PRODUCER_N, CONSUMER_N,
                                                  AtomicQueueProduceAll<ProduceFreshOrderBook<MsgType>>, AtomicQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, CONSUMER_N, PRODUCER_N>, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceFreshOrderBook<MsgType>>, MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, Mgark_MulticastReliableBoundedContext<MgarkMsgType, CONSUMER_N, PRODUCER_N>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceFreshOrderBook<MgarkMsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC  multicast tests single consumer!
  /*for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 3;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpsc_uint32";
    // using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AQ_MPMCBoundedDynamicContext<MsgType,
  std::numeric_limits<MsgType>::max()>, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>>,
  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>>>, ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME,
  RING_BUFFER_SIZE), benchmark_creator<ThroughputBenchmark<MgarkMsgType,
  Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>, PRODUCER_N,
  CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME,
  RING_BUFFER_SIZE)}) .go(N);
  }

  // MPSC  anycast tests, multiple consumers!
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 3;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpmc_uint32";
    // using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AQ_MPMCBoundedDynamicContext<MsgType,
  std::numeric_limits<MsgType>::max()>, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>>,
  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>>>, ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME,
  RING_BUFFER_SIZE), benchmark_creator<ThroughputBenchmark<MgarkMsgType,
  Mgark_AnycastReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>, PRODUCER_N,
  CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>>,
                                                  MgarkSingleQueueAnycastConsumeAll<ConsumeAndStore<MgarkMsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME,
  RING_BUFFER_SIZE)}) .go(N);
  }

  // MPMC - Multicast consumers  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr bool MULTICAST_CONSUMERS = true;
    constexpr const char* BENCH_NAME = "mpmc_int_multicast";
    using MsgType = int;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType,
  Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N>, PRODUCER_N, CONSUMER_N,
  MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>, MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>,
  MULTICAST_CONSUMERS>, ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME,
  RING_BUFFER_SIZE)}) .go(N);
  }*/

  return 0;
}