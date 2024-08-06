#pragma once

#include "detail/common.h"
#include <atomic>
#include <mpmc.h>

template <class T, size_t _PRODUCER_N_, size_t _CONSUMER_N_, size_t _BATCH_NUM_ = 4>
struct Mgark_MulticastReliableBoundedContext
{
  static constexpr const char* VENDOR = "mgark";

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_, _BATCH_NUM_>;
  QueueType q;

  Mgark_MulticastReliableBoundedContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage>
struct MgarkSingleQueueProduceAll
{
  using message_creator = ProduceOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
  {
    ProducerBlocking<typename BenchmarkContext::QueueType> p(ctx.q);
    ctx.q.start();

    size_t i = 0;
    ProduceReturnCode ret_code;
    while (i < N)
    {
      ret_code = p.emplace(message_creator());
      if (ProduceReturnCode::Published == ret_code)
        ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage>
struct MgarkSingleQueueConsumeAll
{
  using message_processor = ProcessOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, std::atomic_uint64_t& consumers_ready_num, BenchmarkContext& ctx,
                    ProcessOneMessage& p)
  {
    ConsumerBlocking<typename BenchmarkContext::QueueType> c(ctx.q);
    // important to do this after creating a consumer since it needs first to join the queue before
    ++consumers_ready_num;

    size_t i = 0;
    bool stop = false;
    while (i < N)
    {
      auto ret_code = c.consume([&](const auto& m) mutable { p(m); });
      if (ret_code == ConsumeReturnCode::Consumed)
        ++i;
    }

    return i;
  }
};
