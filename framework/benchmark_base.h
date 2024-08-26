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

  std::string make_hidden_unique_key() const
  {
    return name_ + vendor_ + std::to_string(ring_buffer_sz_) + "-" + msg_type_name() +
      std::to_string(producer_num()) + "-" + std::to_string(consumer_num());
  }

public:
  using single_run_result = SingleRunResult;

  BenchmarkBase(const std::string& name, const std::string& vendor, size_t ring_buffer_sz)
    : name_(name), vendor_(vendor), ring_buffer_sz_(ring_buffer_sz)
  {
  }

  virtual ~BenchmarkBase() = default;

  virtual SingleRunResult go(size_t N) = 0;
  virtual std::string msg_type_name() const = 0;
  virtual size_t producer_num() const = 0;
  virtual size_t consumer_num() const = 0;

  std::string key() const { return make_hidden_unique_key(); }
  std::string name() const { return name_; }
  std::string vendor() const { return vendor_; }
  size_t ring_buffer_sz() const { return ring_buffer_sz_; }
};
