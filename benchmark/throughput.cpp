#include "../framework/benchmark.h"
#include "message_functors.h"
#include <iostream>

int main()
{
  constexpr size_t N = 1024 * 1024;
  constexpr size_t RING_BUFFER_SIZE = 1024;
  constexpr size_t ITERATION_NUM = 100;

  std::cout << ThroughputBenchmarkSuite::csv_header();

  std::cout
    << ThroughputBenchmarkSuite(
         ITERATION_NUM,
         {benchmark_creator<ThroughputBenchmark2<int, 1, 1, AtomicQueueConfig<int, -1>, ProduceIncremental<int>, ConsumeAndStore<int>>,
                            ThroughputBenchmarkSuite::BenchmarkRunResult>(AtomicQueueConfig<int, -1>{RING_BUFFER_SIZE}),
          benchmark_creator<ThroughputBenchmark<int, 1, 1, MgarkQueueConfig, ProduceIncremental<int>, ConsumeAndStore<int>>,
                            ThroughputBenchmarkSuite::BenchmarkRunResult>(MgarkQueueConfig{RING_BUFFER_SIZE})})
         .go(N);

  return 0;
}