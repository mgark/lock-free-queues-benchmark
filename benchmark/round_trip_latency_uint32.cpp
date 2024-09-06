
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
#include "../framework/factory.h"
#include <cstdint>
#include <iostream>
#include <limits>

#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"

int main()
{

  constexpr bool _MAXIMIZE_THROUGHOUT_ = false;
  std::cout << LatencyBenchmarkStats::csv_header();

  // SPSC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {512})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t THREAD_NUM = 1;
    constexpr size_t N = 1024 * 64;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_uint32";

    // using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;
    //    integral_msb_always_0<uint32_t>;
    using MgarkBenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N, 4>;
    using AQBenchmarkContext =
      AQ_SPSCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {512})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t THREAD_NUM = 1;
    constexpr size_t N = 1024 * 64;
    constexpr size_t ITERATION_NUM = 100;

    constexpr const char* BENCH_NAME = "mpsc_uint32";

    using MsgType = uint32_t;
    using MgarkMsgType = uint32_t; // integral_msb_always_0<uint32_t>;

    using MgarkBenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N, 4>;
    using AQBenchmarkContext =
      AQ_MPMCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPMC round-trip latency  tests
  for (size_t RING_BUFFER_SIZE : {512})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t THREAD_NUM = 2;
    constexpr size_t N = 1024 * 32;
    constexpr size_t ITERATION_NUM = 100;

    constexpr const char* BENCH_NAME = "mpmc_uint32";

    using MsgType = uint32_t;
    // using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkMsgType = MsgType;

    using MgarkBenchmarkContext =
      Mgark_AnycastReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using Mgark2BenchmarkContext =
      Mgark_Anycast2ReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using AQBenchmarkContext =
      AQ_MPMCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               Mgark_Anycast_SingleQueueLatencyA<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               Mgark_Anycast_SingleQueueLatencyB<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),

            benchmark_creator<LatencyBenchmark<MgarkMsgType, Mgark2BenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, Mgark2BenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceIncremental<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, Mgark2BenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),

            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceIncremental<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}