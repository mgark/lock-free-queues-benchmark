#pragma once

#include <atomic_queue/atomic_queue.h>

template <class T, T NIL_VAL>
struct AQ_SPSCBoundedDynamicBenchmarkContext
{
  using QueueType = atomic_queue::AtomicQueueB<T, std::allocator<T>, NIL_VAL, true, false, true>;
  QueueType q;

  AQ_SPSCBoundedDynamicBenchmarkContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage>
struct AtomicQueueProduceAll
{
  using message_creator = ProduceOneMessage;

  template <class BenchmarkContext>
  void operator()(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
  {
    size_t i = 0;
    ProduceOneMessage message_creator_;
    while (i < N)
    {
      ctx.q.push(message_creator_());
      ++i;
    }
  }
};

template <class ConsumeOneMessage>
struct AtomicQueueConsumeAll
{
  using message_consumer = ConsumeOneMessage;

  template <class BenchmarkContext>
  void operator()(size_t N, BenchmarkContext& ctx, ConsumeOneMessage& message_processor)
  {
    ConsumeOneMessage message_processor_;
    size_t i = 0;
    while (i < N)
    {
      message_processor_(ctx.q.pop());
      ++i;
    }
  }
};
