
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

#include "detail/common.h"
#include "detail/consumer.h"
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

template <class T, size_t _PRODUCER_N_, size_t _CONSUMER_N_, size_t _BATCH_NUM_ = 4>
struct Mgark_Anycast2ReliableBoundedContext_SingleQueue
{
  static constexpr const char* VENDOR = "mgark_anycast_optimized";
  static constexpr bool _MULTICAST_ = false;

  using QueueType = SPMCMulticastQueueReliableBounded<T, _CONSUMER_N_, _PRODUCER_N_, _BATCH_NUM_, _MULTICAST_>;
  QueueType q;

  Mgark_Anycast2ReliableBoundedContext_SingleQueue(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage, class BenchmarkContext>
struct MgarkSingleQueueProduceAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProduceOneMessage& message_creator_;
  ProducerBlocking<typename BenchmarkContext::QueueType> p_;

  using message_creator = ProduceOneMessage;
  MgarkSingleQueueProduceAll(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
    : N_(N), ctx_(ctx), message_creator_(message_creator), p_(ctx.q)
  {
    ctx.q.start();
  }

  size_t operator()()
  {
    size_t i = 0;
    ProduceReturnCode ret_code;
    while (i < N_)
    {
      ret_code = p_.emplace(message_creator_());
      if (ProduceReturnCode::Published == ret_code)
        ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage, class BenchmarkContext>
struct MgarkSingleQueueConsumeAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProcessOneMessage& p_;
  ConsumerBlocking<typename BenchmarkContext::QueueType> c_;

  using message_processor = ProcessOneMessage;
  MgarkSingleQueueConsumeAll(size_t N, BenchmarkContext& ctx, ProcessOneMessage& p)
    : N_(N), ctx_(ctx), p_(p), c_(ctx_.q)
  {
  }

  size_t operator()()
  {
    // important to do this after creating a consumer since it needs first to join the queue before
    size_t i = 0;
    bool stop = false;
    while (i < N_)
    {
      // auto ret_code = c_.consume([&](const auto& m) mutable { p_(m); });
      const auto* v = c_.peek();
      if (v)
      {
        p_(*v);
        c_.skip();
      }

      // p_(c_.consume());
      //  if (ret_code == ConsumeReturnCode::Consumed)
      ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage, class BenchmarkContext>
struct MgarkSingleQueueAnycastConsumeAll
{

  size_t N_;
  BenchmarkContext& ctx_;
  ProcessOneMessage& p_;
  AnycastConsumerBlocking<typename BenchmarkContext::QueueType> c_;

  using message_processor = ProcessOneMessage;

  MgarkSingleQueueAnycastConsumeAll(size_t N, BenchmarkContext& ctx, ProcessOneMessage& p)
    : N_(N), ctx_(ctx), p_(p), c_(ctx_.consumer_group)
  {
  }

  size_t operator()()
  {
    // important to do this after creating a consumer since it needs first to join the queue before

    size_t i = 0;
    bool stop = false;
    while (i < N_)
    {
      auto ret_code = c_.consume([&](const auto& m) mutable { p_(m); });
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

  size_t operator()(size_t thread_idx, size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
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

      const typename BenchmarkContext::QueueType::type* val;
      do
      {
        val = consumer.peek();
        if (val)
        {
          mp(*val);
          consumer.skip();
        }
        // c_ret_code = consumer.consume([&](const auto& m) mutable { mp(m); });
      } while (val == nullptr);
      //} while (c_ret_code != ConsumeReturnCode::Consumed);
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

  size_t operator()(size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
  {
    // important to do this after creating a consumer since it needs first to join the queue before
    b_ctx.q.start();

    int i = 0;
    ProduceReturnCode p_ret_code;
    ConsumeReturnCode c_ret_code;

    while (i < N)
    {
      const typename BenchmarkContext::QueueType::type* val;
      do
      {
        val = consumer.peek();
        if (val)
        {
          mp(*val);
          consumer.skip();
        }
        // c_ret_code = consumer.consume([&](const auto& m) mutable { mp(m); });
        //} while (c_ret_code != ConsumeReturnCode::Consumed);
      } while (val == nullptr);

      do
      {
        p_ret_code = producer.emplace(mc());
      } while (p_ret_code != ProduceReturnCode::Published);

      ++i;
    }

    return i;
  }
};

template <class ProduceOneMessage, class ProcessOneMessage, class BenchmarkContext>
struct Mgark_Anycast_SingleQueueLatencyA
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  AnycastConsumerBlocking<typename BenchmarkContext::QueueType> consumer;
  ProducerBlocking<typename BenchmarkContext::QueueType> producer;

  Mgark_Anycast_SingleQueueLatencyA(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
    : consumer(b_ctx.consumer_group), producer(a_ctx.q)
  {
  }

  size_t operator()(size_t thread_idx, size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
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
struct Mgark_Anycast_SingleQueueLatencyB
{
  using message_creator = ProduceOneMessage;
  using message_processor = ProcessOneMessage;

  AnycastConsumerBlocking<typename BenchmarkContext::QueueType> consumer;
  ProducerBlocking<typename BenchmarkContext::QueueType> producer;

  Mgark_Anycast_SingleQueueLatencyB(BenchmarkContext& a_ctx, BenchmarkContext& b_ctx)
    : consumer(a_ctx.consumer_group), producer(b_ctx.q)
  {
  }

  size_t operator()(size_t N, BenchmarkContext& a_ctx, BenchmarkContext& b_ctx,
                    ProduceOneMessage& mc, ProcessOneMessage& mp)
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
