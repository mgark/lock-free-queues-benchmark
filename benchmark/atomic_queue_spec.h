#pragma once

#include <atomic_queue/atomic_queue.h>

template <class T, T NIL_VAL>
struct AQ_SPSCBoundedDynamicContext
{
  using QueueType = atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL, true, false, true>;
  QueueType q;

  AQ_SPSCBoundedDynamicContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage>
struct AtomicQueueProduceAll
{
  using message_creator = ProduceOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
  {
    size_t i = 0;
    ProduceOneMessage message_creator_;
    while (i < N)
    {
      ctx.q.push(message_creator_());
      ++i;
    }

    return i;
  }
};

template <class ConsumeOneMessage>
struct AtomicQueueConsumeAll
{
  using message_consumer = ConsumeOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, BenchmarkContext& ctx, ConsumeOneMessage& message_processor)
  {
    ConsumeOneMessage message_processor_;
    size_t i = 0;
    while (i < N)
    {
      message_processor_(ctx.q.pop());
      ++i;
    }

    return i;
  }
};
