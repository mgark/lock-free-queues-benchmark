#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "utils.h"
#include <functional>
#include <iomanip>
#include <list>
#include <random>

template <class SingleRunResult>
class BenchmarkBase
{
  std::string name_;
  std::string vendor_;
  size_t ring_buffer_sz_;
  std::string key_;

public:
  using single_run_result = SingleRunResult;

  BenchmarkBase(const std::string& name, const std::string& vendor, size_t ring_buffer_sz)
    : name_(name), vendor_(vendor), ring_buffer_sz_(ring_buffer_sz)
  {
    key_ = name_ + vendor_ + std::to_string(ring_buffer_sz);
  }

  virtual ~BenchmarkBase() = default;

  virtual SingleRunResult go(size_t N) = 0;
  virtual std::string msg_type_name() const = 0;
  virtual size_t producer_num() const = 0;
  virtual size_t consumer_num() const = 0;

  std::string key() const { return key_; }
  std::string name() const { return name_; }
  std::string vendor() const { return vendor_; }
  size_t ring_buffer_sz() const { return ring_buffer_sz_; }
};
