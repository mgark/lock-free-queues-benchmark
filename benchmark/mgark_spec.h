#pragma once

#include "detail/common.h"
#include <atomic>
#include <chrono>
#include <mpmc.h>

template <class T, size_t _PRODUCER_N_, size_t _CONSUMER_N_, size_t _BATCH_NUM_ = 4>
struct Mgark_MulticastReliableBoundedContext
{
  static constexpr const char* VENDOR = "mgark";

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_, _BATCH_NUM_>;
  QueueType q;

  Mgark_MulticastReliableBoundedContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class T, size_t _PRODUCER_N_, size_t _CONSUMER_N_, size_t _BATCH_NUM_ = 4>
struct Mgark_AnycastReliableBoundedContext_SingleQueue
{
  static constexpr const char* VENDOR = "mgark";

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_, _BATCH_NUM_>;
  QueueType q;
  AnycastConsumerGroup<QueueType> consumer_group;

  Mgark_AnycastReliableBoundedContext_SingleQueue(size_t ring_buffer_sz)
    : q(ring_buffer_sz), consumer_group({&q})
  {
  }
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

template <class ProcessOneMessage>
struct MgarkSingleQueueAnycastConsumeAll
{
  using message_processor = ProcessOneMessage;

  template <class BenchmarkContext>
  size_t operator()(size_t N, std::atomic_uint64_t& consumers_ready_num, BenchmarkContext& ctx,
                    ProcessOneMessage& p)
  {
    AnycastConsumerBlocking<typename BenchmarkContext::QueueType> c(ctx.consumer_group);
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

template <class ProduceOneMessage, class ProcessOneMessage, class BenchmarkContext>
struct MgarkSingleQueueLatencyA
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  ConsumerBlocking<typename BenchmarkContext::QueueType> consumer;
  ProducerBlocking<typename BenchmarkContext::QueueType> producer;

  MgarkSingleQueueLatencyA(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
    : consumer(b_ctx.q), producer(a_ctx.q)
  {
  }

  size_t operator()(size_t thread_idx, size_t N, BenchmarkContext& a_ctx, ProduceOneMessage& mc,
                    ProcessOneMessage& mp)
  {
    // important to do this after creating a consumer since it needs first to join the queue before
    a_ctx.q.start();

    int i = 0;
    ProduceReturnCode p_ret_code;
    ConsumeReturnCode c_ret_code;

    while (i <= N)
    {
      // if there are multiple producers, we just allow the first
      // one to publish the very first bootstrap message
      if (i > 0 || thread_idx == 0)
      {
        do
        {
          p_ret_code = producer.emplace(mc());
        } while (p_ret_code != ProduceReturnCode::Published);
      }

      if (i == N)
        break;

      do
      {
        c_ret_code = consumer.consume([&](const auto& m) mutable { mp(m); });
      } while (c_ret_code != ConsumeReturnCode::Consumed);

      ++i;
    }

    return i;
  }
};

template <class ProduceOneMessage, class ProcessOneMessage, class BenchmarkContext>
struct MgarkSingleQueueLatencyB
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  ConsumerBlocking<typename BenchmarkContext::QueueType> consumer;
  ProducerBlocking<typename BenchmarkContext::QueueType> producer;

  MgarkSingleQueueLatencyB(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
    : consumer(a_ctx.q), producer(b_ctx.q)
  {
  }

  size_t operator()(size_t N, BenchmarkContext& b_ctx, ProduceOneMessage& mc, ProcessOneMessage& mp)
  {
    // important to do this after creating a consumer since it needs first to join the queue before
    b_ctx.q.start();

    int i = 0;
    ProduceReturnCode p_ret_code;
    ConsumeReturnCode c_ret_code;

    while (i < N)
    {
      do
      {
        c_ret_code = consumer.consume([&](const auto& m) mutable { mp(m); });
      } while (c_ret_code != ConsumeReturnCode::Consumed);

      do
      {
        p_ret_code = producer.emplace(mc());
      } while (p_ret_code != ProduceReturnCode::Published);

      ++i;
    }

    return i;
  }
};
