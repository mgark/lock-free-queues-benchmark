#include "detail/common.h"
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mpmc.h>

#include "../../thirdparty/mgark/test/common_test_utils.h"
#include <new>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <x86intrin.h>

template <class T, size_t BATCH_NUM = 4>
class SPSC2
{
  struct Node
  {
    alignas(T) std::byte paylod[sizeof(T)];
  };

  static_assert((BATCH_NUM & (BATCH_NUM - 1)) == 0, "BATCH_NUM must be power of 2");
  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);

  Node* data_;
  size_t N_;
  size_t items_per_batch_;
  alignas(_CACHE_PREFETCH_SIZE_) size_t last_read_idx_{0};
  alignas(_CACHE_PREFETCH_SIZE_) std::atomic<size_t> write_idx_{0};

  alignas(_CACHE_PREFETCH_SIZE_) std::atomic<size_t> read_idx_{0};
  alignas(_CACHE_PREFETCH_SIZE_) size_t local_read_idx_{0};
  size_t next_checkpoint_idx_;
  size_t last_write_idx_{0};
  std::allocator<Node> alloc_;

  static constexpr size_t _batch_buffer_size_ = 256 / sizeof(Node);
  static constexpr size_t _items_per_cache_prefetch_num_ = _CACHE_PREFETCH_SIZE_ / sizeof(Node);
  alignas(_CACHE_PREFETCH_SIZE_) std::byte batch_buffer_[_batch_buffer_size_ * sizeof(Node)];
  size_t batch_buffer_idx_{std::numeric_limits<size_t>::max()};

public:
  SPSC2(size_t N) : N_(N), items_per_batch_(N / BATCH_NUM), next_checkpoint_idx_(items_per_batch_)
  {
    if ((N & (N - 1)) != 0)
    {
      throw std::runtime_error("queue size must be power of 2");
    }

    if ((items_per_batch_ & (items_per_batch_ - 1)) != 0)
    {
      throw std::runtime_error("items per batch must be power of 2");
    }

    data_ = alloc_.allocate(N_);
    std::uninitialized_default_construct(data_, data_ + N_);
  }

  size_t prev_result_{0};
  ~SPSC2() { alloc_.deallocate(data_, N_); }

  T* peek()
  {
    if (batch_buffer_idx_ < _batch_buffer_size_)
    {
      return std::launder(reinterpret_cast<T*>(&batch_buffer_[(batch_buffer_idx_++) * sizeof(Node)]));
    }

    bool first_time = true;
    while (last_write_idx_ <= local_read_idx_)
    {
      last_write_idx_ = write_idx_.load(std::memory_order_acquire);
      if (first_time == false)
      {
        unroll<0>([]() { _mm_pause(); });
      }

      first_time = false;
    }

    size_t items_ready_num = last_write_idx_ - local_read_idx_;
    if (items_ready_num >= _batch_buffer_size_)
    {
      std::memcpy(&batch_buffer_, &data_[(local_read_idx_ & (N_ - 1))], _batch_buffer_size_ * sizeof(Node));
      // local_read_idx_ += _batch_buffer_size_;
      batch_buffer_idx_ = 0;
      return std::launder(reinterpret_cast<T*>(&batch_buffer_[(batch_buffer_idx_++) * sizeof(Node)]));
    }
    else
    {
      Node* node = &data_[local_read_idx_ & (N_ - 1)];
      //++local_read_idx_;
      return std::launder(reinterpret_cast<T*>(node->paylod));
    }
  }

  void skip()
  {
    if (local_read_idx_++ >= next_checkpoint_idx_)
    {
      read_idx_.store(local_read_idx_, std::memory_order_release);
      size_t prev_checkpoint = next_checkpoint_idx_;
      next_checkpoint_idx_ += items_per_batch_; // - (local_read_idx_ & (items_per_batch_ - 1)) + items_per_batch_;
    }
  }

  template <class... Args>
  void emplace(Args&&... args)
  {
    bool first_time = true;
    size_t write_idx = write_idx_.load(std::memory_order_relaxed);
    while (write_idx >= last_read_idx_ + N_)
    {
      last_read_idx_ = read_idx_.load(std::memory_order_acquire);
      if (first_time == false)
      {
        unroll<0>([]() { _mm_pause(); });
      }

      first_time = false;
    }

    Node* node = &data_[write_idx & (N_ - 1)];
    ::new (node->paylod) T(std::forward<Args>(args)...);
    write_idx_.store(write_idx + 1, std::memory_order_release);
  }
};

template <class T, size_t BATCH_NUM>
struct Spsc2BenchmarkContext
{
  static constexpr const char* VENDOR = "spsc2";

  using QueueType = SPSC2<T, BATCH_NUM>;
  QueueType q;

  Spsc2BenchmarkContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage, class BenchmarkContext>
struct Spsc2SingleQueueProduceAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProduceOneMessage& message_creator_;

  using message_creator = ProduceOneMessage;
  Spsc2SingleQueueProduceAll(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
    : N_(N), ctx_(ctx), message_creator_(message_creator)
  {
  }

  size_t operator()()
  {
    size_t i = 0;
    while (i < N_)
    {
      ctx_.q.emplace(message_creator_());
      ++i;
    }

    return i;
  }
};

template <class ProcessOneMessage, class BenchmarkContext>
struct Spsc2QueueConsumeAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProcessOneMessage& p_;

  using message_processor = ProcessOneMessage;
  Spsc2QueueConsumeAll(size_t N, BenchmarkContext& ctx, ProcessOneMessage& p)
    : N_(N), ctx_(ctx), p_(p)
  {
  }

  size_t operator()()
  {
    size_t i = 0;
    while (i < N_)
    {
      const auto* v = ctx_.q.peek();
      if (v)
      {
        p_(*v);
        ctx_.q.skip();
        ++i;
      }
    }

    return i;
  }
};
