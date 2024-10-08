
/*
 * Copyright(c) 2024-present Mykola Garkusha.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../framework/benchmark_base.h"
#include "../framework/benchmark_round_trip_latency.h"
#include "../framework/benchmark_suite.h"
#include "../framework/benchmark_throughput.h"
#include "../framework/factory.h"
#include "detail/common.h"
#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"
#include "vendor_specs/spsc1_spec.h"
#include "vendor_specs/spsc2_spec.h"
#include <concepts>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

int main()
{

  std::cout << ThroughputBenchmarkStats::csv_header();
  constexpr size_t BATCH_NUM = 4;

  // SPSC Optmized  tests
  for (size_t RING_BUFFER_SIZE : {2048, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 512;
    constexpr size_t ITERATION_NUM = 120;
    constexpr const char* BENCH_NAME = "spsc_int";
    constexpr size_t CPU_PAUSE_N = 0;
    using MsgType0 = uint32_t;
    using MsgType = MsgType0;
    // using MsgType = integral_msb_always_0<uint32_t>;
    using BenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, CPU_PAUSE_N>;

    using AtomicQueueContext =
      AQ_SPSCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), true>;
    using Spsc1Context = Spsc1BenchmarkContext<uint32_t>;
    using Spsc2Context = Spsc2BenchmarkContext<uint32_t, BATCH_NUM>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType, BenchmarkContext, PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>, BenchmarkContext>,
                                                  MgarkSingleQueueNonBlockingConsumeAll<ConsumeAndStore<MsgType>, BenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType0, Spsc1Context, PRODUCER_N, CONSUMER_N, Spsc1SingleQueueProduceAll<ProduceIncremental<MsgType0>, Spsc1Context>,
                                                  Spsc1QueueConsumeAll<ConsumeAndStore<MsgType0>, Spsc1Context>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MsgType0, Spsc2Context, PRODUCER_N, CONSUMER_N, Spsc2SingleQueueProduceAll<ProduceIncremental<MsgType0>, Spsc2Context>,
                                                  Spsc2QueueConsumeAll<ConsumeAndStore<MsgType0>, Spsc2Context>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  /*  for (size_t RING_BUFFER_SIZE : {2048, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 512;
    constexpr size_t ITERATION_NUM = 120;
    constexpr const char* BENCH_NAME = "mpsc_uint32";
    constexpr size_t CPU_PAUSE_N = 20;
    using MsgType = uint32_t;
    using BenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, CPU_PAUSE_N>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, BenchmarkContext, PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>, BenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>, BenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }*/

  return 0;
}