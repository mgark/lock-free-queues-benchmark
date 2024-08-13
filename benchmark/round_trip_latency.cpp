#include "../framework/benchmark_base.h"
#include "../framework/benchmark_round_trip_latency.h"
#include "../framework/benchmark_suite.h"
#include "../framework/factory.h"
#include "atomic_queue_spec.h"
#include "mgark_spec.h"
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
    constexpr size_t N = 128;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_uint32";

    using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = uint32_t;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}