#include "detail/common.h"
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mpmc.h>

#include <new>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <x86intrin.h>

template <class T>
class SPSC1
{
  struct Node
  {
    alignas(T) std::byte paylod[sizeof(T)];
    std::atomic_bool busy{false};
  };

  static_assert(std::is_trivially_copyable_v<T>);
  static_assert(std::is_trivially_destructible_v<T>);

  Node* data_;
  alignas(64) size_t read_idx_{0};
  alignas(64) size_t write_idx_{0};
  std::allocator<Node> alloc_;
  size_t N_;

public:
  SPSC1(size_t N) : N_(N)
  {
    if ((N & (N - 1)) != 0)
    {
      throw std::runtime_error("queue size must be power of 2");
    }

    data_ = alloc_.allocate(N_);
    std::uninitialized_default_construct(data_, data_ + N_);
  }

  ~SPSC1() { alloc_.deallocate(data_, N_); }

  T* peek()
  {
    Node* node = &data_[read_idx_];
    while (!node->busy.load(std::memory_order_acquire))
      ;

    return std::launder(reinterpret_cast<T*>(node->paylod));
  }

  void skip()
  {
    Node* node = &data_[read_idx_];
    read_idx_ = (read_idx_ + 1) & (N_ - 1);
    node->busy.store(false, std::memory_order_release);
  }

  template <class... Args>
  void emplace(Args&&... args)
  {
    Node* node = &data_[write_idx_];
    while (node->busy.load(std::memory_order_acquire))
      ;

    ::new (node->paylod) T(std::forward<Args>(args)...);
    write_idx_ = (write_idx_ + 1) & (N_ - 1);
    node->busy.store(true, std::memory_order_release);
  }
};

template <class T>
struct Spsc1BenchmarkContext
{
  static constexpr const char* VENDOR = "spsc1";

  using QueueType = SPSC1<T>;
  QueueType q;

  Spsc1BenchmarkContext(size_t ring_buffer_sz) : q(ring_buffer_sz) {}
};

template <class ProduceOneMessage, class BenchmarkContext>
struct Spsc1SingleQueueProduceAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProduceOneMessage& message_creator_;

  using message_creator = ProduceOneMessage;
  Spsc1SingleQueueProduceAll(size_t N, BenchmarkContext& ctx, ProduceOneMessage& message_creator)
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
struct Spsc1QueueConsumeAll
{
  size_t N_;
  BenchmarkContext& ctx_;
  ProcessOneMessage& p_;

  using message_processor = ProcessOneMessage;
  Spsc1QueueConsumeAll(size_t N, BenchmarkContext& ctx, ProcessOneMessage& p)
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
