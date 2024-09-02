
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
#include "../framework/benchmark_suite.h"
#include "../framework/benchmark_throughput.h"
#include "../framework/factory.h"
#include <cstdint>
#include <iostream>
#include <limits>

#include "types/order_book.h"
#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"

int main()
{

  std::cout << ThroughputBenchmarkStats::csv_header();

  // SPSC  tests
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 512;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_orderbook";
    using MgarkMsgType = OrderBook;
    using MsgType = OrderBook;

    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using AtomicQueueContext = AQ_NonAtomic_SPSCBoundedDynamicContext<MsgType>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceFreshOrderBook<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceFreshOrderBook<MgarkMsgType>, MgarkBenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC  multicast tests single consumer!
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 3;
    constexpr size_t N = 1024 * 512;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpsc_orderbook";
    using MgarkMsgType = OrderBook;
    using MsgType = OrderBook;

    using MgarkBenchmarkContext = Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using AtomicQueueContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceFreshOrderBook<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N>,
                                                  PRODUCER_N, CONSUMER_N, MgarkSingleQueueProduceAll<ProduceFreshOrderBook<MgarkMsgType>, MgarkBenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPMC  anycast tests, multiple consumers!
  for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 512;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpmc_orderbook";
    using MgarkMsgType = OrderBook;
    using MsgType = OrderBook;

    using MgarkBenchmarkContext =
      Mgark_Anycast2ReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N>;
    using AtomicQueueContext = AQ_NonAtomic_MPMCBoundedDynamicContext<MsgType>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceFreshOrderBook<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceFreshOrderBook<MgarkMsgType>, MgarkBenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  return 0;
}