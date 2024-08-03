#include "../framework/benchmark_base.h"
#include "../framework/benchmark_suite.h"
#include "../framework/benchmark_throughput.h"
#include "../framework/factory.h"
#include "atomic_queue_spec.h"
#include "mgark_spec.h"
#include <iostream>

int main()
{

  std::cout << ThroughputBenchmarkStats::csv_header();

  // SPSC  tests
  for (size_t RING_BUFFER_SIZE : {512, 1024})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 1024;
    constexpr size_t ITERATION_NUM = 100;
    using MsgType = int;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AQ_SPSCBoundedDynamicContext<MsgType, -1>, PRODUCER_N, CONSUMER_N,
                                                  AtomicQueueProduceAll<ProduceIncremental<MsgType>>, AtomicQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(
              "atomic_queue_spsc_int_" + std::to_string(RING_BUFFER_SIZE), RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType, Mgark_MulticastReliableBoundedContext<MsgType, CONSUMER_N, PRODUCER_N>, PRODUCER_N, CONSUMER_N,
                                                  MgarkProduceAll<ProduceIncremental<MsgType>>, MgarkConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(
              "mgark_spsc_int_" + std::to_string(RING_BUFFER_SIZE), RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}