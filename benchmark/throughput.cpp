#include "../framework/benchmark.h"
#include "message_functors.h"
#include <iostream>

int main()
{
  constexpr size_t N = 1024 * 1024 * 20;
  constexpr size_t MGARK_RING_BUFFER_SIZE = 1024;

  std::cout << ThroughputBenchmarkSummary::csv_header();
  std::cout << ThroughputBenchmark<int, 1, 1, MgarkQueueConfig>(N, {MGARK_RING_BUFFER_SIZE})
                 .go<ProduceIncremental<int>, ConsumeAndStore<int>>()
                 .summary();
  return 0;
}