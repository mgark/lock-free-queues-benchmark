#include "../framework/benchmark_base.h"
#include "../framework/benchmark_round_trip_latency.h"
#include "../framework/benchmark_suite.h"
#include "../framework/factory.h"
#include "atomic_queue_spec.h"
#include "mgark_spec.h"
#include "order_book.h"
#include <cstdint>
#include <iostream>
#include <limits>

int main()
{

  std::cout << LatencyBenchmarkStats::csv_header();

  // SPSC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t THREAD_NUM = 1;
    constexpr size_t N = 1024 * 64;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_big_object";

    using MgarkMsgType = OrderBookOptimized;
    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t THREAD_NUM = 1;
    constexpr size_t N = 1024 * 64;
    constexpr size_t ITERATION_NUM = 100;

    constexpr const char* BENCH_NAME = "mpsc_big_object";

    using MgarkMsgType = OrderBookOptimized;
    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPMC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t THREAD_NUM = 2;
    constexpr size_t N = 1024 * 32;
    constexpr size_t ITERATION_NUM = 100;

    constexpr const char* BENCH_NAME = "mpmc_big_object";

    using MgarkMsgType = OrderBookOptimized;
    using MgarkBenchmarkContext =
      Mgark_AnycastReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               Mgark_Anycast_SingleQueueLatencyA<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               Mgark_Anycast_SingleQueueLatencyB<ProduceOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}