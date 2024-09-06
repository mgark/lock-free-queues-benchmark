
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

#include "vendor_specs/atomic_queue_spec.h"
#include "vendor_specs/mgark_spec.h"

#include <cstdint>
#include <iostream>
#include <limits>

int main()
{

  constexpr bool _MAXIMIZE_THROUGHOUT_ = true;
  constexpr size_t BATCH_NUM = 32;
  std::cout << ThroughputBenchmarkStats::csv_header();

  // SPSC  tests
  for (size_t RING_BUFFER_SIZE : {1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 1;
    constexpr size_t N = 1024 * 1024;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "spsc_uint32";
    constexpr bool CPU_PAUSE_N = 0;
    using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;

    using MgarkBenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, CPU_PAUSE_N>;
    using AtomicQueueContext =
      AQ_SPSCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>, MgarkBenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPSC  multicast tests single consumer!
  for (size_t RING_BUFFER_SIZE : {1024, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 1;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpsc_uint32";
    constexpr bool _CPU_PAUSE_N_ = 30;
    using MgarkMsgType = uint32_t;
    // using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;
    using MgarkBenchmarkContext =
      Mgark_MulticastReliableBoundedContext<MgarkMsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, _CPU_PAUSE_N_>;
    using AtomicQueueContext =
      AQ_MPMCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>, MgarkBenchmarkContext>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)})
           .go(N);
  }

  // MPMC  anycast tests, multiple consumers!
  for (size_t RING_BUFFER_SIZE : {1024, 1024 * 64})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr const char* BENCH_NAME = "mpmc_uint32";
    constexpr bool _CPU_PAUSE_N_ = 30;
    // using MgarkMsgType = integral_msb_always_0<uint32_t>;
    using MgarkMsgType = uint32_t;
    using MsgType = uint32_t;

    using MgarkBenchmarkContext2 =
      Mgark_Anycast2ReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, _CPU_PAUSE_N_>;
    using MgarkBenchmarkContext1 =
      Mgark_AnycastReliableBoundedContext_SingleQueue<MgarkMsgType, PRODUCER_N, CONSUMER_N, BATCH_NUM, _CPU_PAUSE_N_>;
    using AtomicQueueContext =
      AQ_MPMCBoundedDynamicContext<MsgType, std::numeric_limits<MsgType>::max(), _MAXIMIZE_THROUGHOUT_>;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType, AtomicQueueContext, PRODUCER_N, CONSUMER_N, AtomicQueueProduceAll<ProduceIncremental<MsgType>, AtomicQueueContext>,
                                                  AtomicQueueConsumeAll<ConsumeAndStore<MsgType>, AtomicQueueContext>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE),
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext2, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>, MgarkBenchmarkContext2>,
                                                  MgarkSingleQueueConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext2>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)/*,
            benchmark_creator<ThroughputBenchmark<MgarkMsgType, MgarkBenchmarkContext1, PRODUCER_N, CONSUMER_N,
                                                  MgarkSingleQueueProduceAll<ProduceIncremental<MgarkMsgType>, MgarkBenchmarkContext1>,
                                                  MgarkSingleQueueAnycastConsumeAll<ConsumeAndStore<MgarkMsgType>, MgarkBenchmarkContext1>>,
                              ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)*/})
           .go(N);
  }

  // MPMC - Multicast consumers  tests
  /*for (size_t RING_BUFFER_SIZE : {64, 512, 1024, 1024 * 64, 1024 * 256})
  {
    constexpr size_t CONSUMER_N = 2;
    constexpr size_t PRODUCER_N = 2;
    constexpr size_t N = 1024 * 256;
    constexpr size_t ITERATION_NUM = 100;
    constexpr bool MULTICAST_CONSUMERS = true;
    constexpr const char* BENCH_NAME = "mpmc_int_multicast";
    using MsgType = int;

    std::cout
      << ThroughputBenchmarkSuite(
           ITERATION_NUM,
           {benchmark_creator<ThroughputBenchmark<MsgType,
  Mgark_MulticastReliableBoundedContext<MsgType, PRODUCER_N, CONSUMER_N>, PRODUCER_N, CONSUMER_N,
  MgarkSingleQueueProduceAll<ProduceIncremental<MsgType>>,
  MgarkSingleQueueConsumeAll<ConsumeAndStore<MsgType>>, MULTICAST_CONSUMERS>,
  ThroughputBenchmarkSuite::BenchmarkRunResult>(BENCH_NAME, RING_BUFFER_SIZE)}) .go(N);
  }*/

  return 0;
}