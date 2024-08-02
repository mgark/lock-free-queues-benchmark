#include "../framework/benchmark.h"
#include "message_functors.h"
#include <iostream>

int main()
{
  constexpr size_t N = 1024 * 1024;
  constexpr size_t RING_BUFFER_SIZE = 1024;
  constexpr size_t ITERATION_NUM = 100;

  std::cout << ThroughputBenchmarkSuite::csv_header();
  std::cout << ThroughputBenchmarkSuite(
                 ITERATION_NUM,
                 {BenchmarkCreator<ThroughputBenchmarkRunResult>(
                   just_type<ThroughputBenchmark2<int, 1, 1, AtomicQueueConfig<int, -1>, ProduceIncremental<int>, ConsumeAndStore<int>>>{},
                   AtomicQueueConfig<int, -1>{RING_BUFFER_SIZE})})
                 .go(N);
  std::cout << ThroughputBenchmarkSuite(
                 ITERATION_NUM,
                 {BenchmarkCreator<ThroughputBenchmarkRunResult>(
                   just_type<ThroughputBenchmark<int, 1, 1, MgarkQueueConfig, ProduceIncremental<int>, ConsumeAndStore<int>>>{},
                   MgarkQueueConfig{RING_BUFFER_SIZE})})
                 .go(N);

  /* std::cout << ThroughputBenchmark<int, 1, 1, MgarkQueueConfig>(N, {RING_BUFFER_SIZE})
                  .go<ProduceSameValue<int>, ConsumeAndStore<int>>()
                  .summary();

   for (int i = 0; i < ITERATIONS; ++i)
     std::cout << ThroughputBenchmark2<int, 1, 1, AtomicQueueConfig<int, -1>>(N, {RING_BUFFER_SIZE})
                    .go<ProduceIncremental<int>, ConsumeAndStore<int>>()
                    .summary();
   for (int i = 0; i < ITERATIONS; ++i)
     std::cout << ThroughputBenchmark<int, 1, 1, MgarkQueueConfig>(N, {RING_BUFFER_SIZE})
                    .go<ProduceIncremental<int>, ConsumeAndStore<int>>()
                    .summary();
                    */
  return 0;
}