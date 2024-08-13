#pragma once

#include "detail/single_bit_reuse.h"
#include <array>
#include <sys/types.h>

template <class SeqNumType>
struct OrderBookBase
{
  static constexpr std::size_t N = 20;

  SeqNumType seq_num;
  std::array<uint32_t, N> bid_price;
  std::array<uint32_t, N> ask_price;
  std::array<uint32_t, N> bid_size;
  std::array<uint32_t, N> ask_size;
};

struct OrderBook : OrderBookBase<uint32_t>
{
};

struct OrderBookOptimized : OrderBookBase<integral_msb_always_0<uint32_t>>
{
  void flip_version() { seq_num.flip_version(); }
  auto read_version() { return seq_num.read_version(); }
  void release_version() { seq_num.release_version(); }
};

template <class T>
struct ProduceFreshOrderBook
{
  T val;

  ProduceFreshOrderBook()
  {
    val.seq_num = 0;
    for (std::size_t i = 0; i < 20; ++i)
    {
      val.bid_price[i] = OrderBook::N - i;
      val.ask_price[i] = OrderBook::N + i;
      val.bid_size[i] = i + 1;
      val.ask_size[i] = i + 1;
    }
  }

  T operator()()
  {
    uint32_t new_seq_num = ++val.seq_num;

    val.bid_size[3] = new_seq_num;
    val.bid_size[17] = new_seq_num;
    val.ask_size[0] = new_seq_num;
    val.ask_size[19] = new_seq_num;

    return val;
  }
};
