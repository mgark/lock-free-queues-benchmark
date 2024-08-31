
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
#include "types/order_book.h"
#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"
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

    using MgarkMsgType = OrderBook;
    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
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

    using MgarkMsgType = OrderBook;
    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
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

    constexpr const char* BENCH_NAME = "mpmc_orderbook";

    using MgarkMsgType = OrderBook;
    using MgarkBenchmarkContext =
      Mgark_Anycast2ReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using MsgType = OrderBook;
    using AQBenchmarkContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << LatencyBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<LatencyBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               MgarkSingleQueueLatencyA<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>,
                                               MgarkSingleQueueLatencyB<ProduceFreshOrderBook<MgarkMsgType>, ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<LatencyBenchmark<MsgType, AQBenchmarkContext, PRODUCER_N, CONSUMER_N, THREAD_NUM,
                                               AQLatencyA<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>,
                                               AQLatencyB<ProduceFreshOrderBook<MsgType>, ConsumeAndStore<MsgType>>>,
                              LatencyBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}