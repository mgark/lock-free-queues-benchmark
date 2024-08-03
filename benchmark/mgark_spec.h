#pragma once

#include <mpmc.h>

template <class T, size_t _PRODUCER_N_, size_t _CONSUMER_N_>
struct Mgark_MulticastReliableBoundedBenchmarkContext
{
  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_>;
  QueueType q;

  Mgark_MulticastReliableBoundedBenchmarkContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage>
struct MgarkProduceAll
{
  using message_creator = ProduceOneMessage;

  template <class BenchmarkContext>
  void operator()(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
  {
    ProducerBlocking<typename BenchmarkContext::QueueType> p(ctx.q);
    ctx.q.start();
    size_t i = 0;
    while (i < N)
    {
      p.emplace(message_creator());
      ++i;
    }
  }
};

template <class ConsumeOneMessage>
struct MgarkConsumeAll
{
  using message_consumer = ConsumeOneMessage;

  template <class BenchmarkContext>
  void operator()(size_t N, BenchmarkContext& ctx, ConsumeOneMessage& message_processor)
  {
    ConsumerBlocking<typename BenchmarkContext::QueueType> c(ctx.q);
    size_t i = 0;
    while (i < N)
    {
      if (ConsumeReturnCode::Consumed == c.consume([&](const auto& m) { message_processor(m); }))
        ++i;
    }
  }
};
