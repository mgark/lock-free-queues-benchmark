
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

#pragma once

#include <atomic>
#include <atomic_queue/atomic_queue.h>

template <class T, T NIL_VAL, bool _MAXIMIZE_THROUGHPUT_>
struct AQ_SPSCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType =
    atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL, _MAXIMIZE_THROUGHPUT_, false, true>;
  QueueType q;

  AQ_SPSCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class T, T NIL_VAL, bool _MAXIMIZE_THROUGHPUT_>
struct AQ_MPMCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType = atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL, _MAXIMIZE_THROUGHPUT_>;
  QueueType q;

  AQ_MPMCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class T, bool _MAXIMIZE_THROUGHPUT_>
struct AQ_NonAtomic_SPSCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType = atomic_queue::AtomicQueueB2<T, std::allocator<T>, _MAXIMIZE_THROUGHPUT_, false, true>;
  QueueType q;

  AQ_NonAtomic_SPSCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class T, bool _MAXIMIZE_THROUGHPUT_>
struct AQ_NonAtomic_MPMCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType = atomic_queue::AtomicQueueB2<T, std::allocator<T>, _MAXIMIZE_THROUGHPUT_>;
  QueueType q;

  AQ_NonAtomic_MPMCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage, class BenchmarkContext>
struct AtomicQueueProduceAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProduceOneMessage& message_creator_;
  using message_creator = ProduceOneMessage;
  AtomicQueueProduceAll(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
    : N_(N), ctx_(ctx), message_creator_(message_creator)
  {
  }
  size_t operator()()
  {
    size_t i = 0;
    while (i < N_)
    {
      ctx_.q.push(message_creator_());
      ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage, class BenchmarkContext>
struct AtomicQueueConsumeAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProcessOneMessage& p_;
  using message_processor = ProcessOneMessage;

  AtomicQueueConsumeAll(size_t N, BenchmarkContext& ctx, ProcessOneMessage& p)
    : N_(N), ctx_(ctx), p_(p)
  {
  }

  size_t operator()()
  {
    size_t i = 0;
    while (i < N_)
    {
      p_(ctx_.q.pop());
      ++i;
    }

    return i;
  }
};

template <class ProduceOneMessage, class ProcessOneMessage>
struct AQLatencyA
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  template <class BenchmarkContext>
  AQLatencyA(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
  {
  }

  template <class BenchmarkContext>
  size_t operator()(size_t thread_idx, size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
  {
    int i = 0;
    while (i <= N)
    {
      // if there are multiple producers, we just allow the first
      // one to publish the very first bootstrap message
      if (i > 0 || thread_idx == 0)
        a_ctx.q.push(mc());

      if (i == N)
        break;

      mp(b_ctx.q.pop());
      ++i;
    }

    return i;
  }
};

template <class ProduceOneMessage, class ProcessOneMessage>
struct AQLatencyB
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  template <class BenchmarkContext>
  AQLatencyB(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
  {
  }

  template <class BenchmarkContext>
  size_t operator()(size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
  {
    int i = 0;
    while (i < N)
    {
      mp(a_ctx.q.pop());
      b_ctx.q.push(mc());
      ++i;
    }

    return i;
  }
};
