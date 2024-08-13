#pragma once

#include "detail/single_bit_reuse.h"
#include <atomic>
#include <atomic_queue/atomic_queue.h>

template <class T, T NIL_VAL>
struct AQ_SPSCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType = atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL, true, false, true>;
  QueueType q;

  AQ_SPSCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class T, T NIL_VAL>
struct AQ_MPMCBoundedDynamicContext
{
  static constexpr const char* VENDOR = "atomic_queue";

  using QueueType = atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL>;
  QueueType q;

  AQ_MPMCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage>
struct AtomicQueueProduceAll
{
  using message_creator = ProduceOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
  {
    size_t i = 0;
    while (i < N)
    {
      ctx.q.push(message_creator());
      ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage>
struct AtomicQueueConsumeAll
{
  using message_processor = ProcessOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, std::atomic_uint64_t& consumers_ready_num, BenchmarkContext& ctx,
                    ProcessOneMessage& p)
  {
    ++consumers_ready_num;

    size_t i = 0;
    while (i < N)
    {
      p(ctx.q.pop());
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
