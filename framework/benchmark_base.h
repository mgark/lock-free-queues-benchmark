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

public:
  using single_run_result = SingleRunResult;

  BenchmarkBase(const std::string& name) : name_(name) {}
  virtual ~BenchmarkBase() = default;

  virtual SingleRunResult go(size_t N) = 0;
  std::string name() const { return name_; }
  virtual std::string msg_type_name() const = 0;
  virtual size_t producer_num() const = 0;
  virtual size_t consumer_num() const = 0;
};
