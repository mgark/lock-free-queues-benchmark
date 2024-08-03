#include "../framework/benchmark.h"
#include "message_functors.h"
#include <iostream>

int main()
{
  constexpr size_t N = 1024 * 1024;
  constexpr size_t RING_BUFFER_SIZE = 1024;
  constexpr size_t ITERATION_NUM = 100;

  std::cout << ThroughputBenchmarkSuite::csv_header();

  // SPSC  tests
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    using MsgType = int;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueBenchmarkContext<MsgType, -1>, PRODUCER_N, CONSUMER_N,
                                                  AtomicQueueProduceAll<ProduceIncremental<MsgType>>, AtomicQueueConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType, MgarkBenchmarkContext<MsgType, CONSUMER_N, PRODUCER_N>, PRODUCER_N, CONSUMER_N,
                                                  MgarkProduceAll<ProduceIncremental<MsgType>>, MgarkConsumeAll<ConsumeAndStore<MsgType>>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}