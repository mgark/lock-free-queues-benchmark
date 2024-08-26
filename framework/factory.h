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

#include "benchmark_base.h"

template <class ConcreteBenchmark, class SingleRunResult, class... T>
static std::function<std::unique_ptr<BenchmarkBase<SingleRunResult>>()> benchmark_creator(
  T... params) requires(std::is_same_v<SingleRunResult, typename ConcreteBenchmark::single_run_result>)
{
  return [=]() { return std::make_unique<ConcreteBenchmark>(params...); };
}

template <class T>
struct ProduceIncremental
{
  T initial_val{};
  T operator()() { return ++initial_val; }
};

template <class T>
struct ProduceSameValue
{
  T initial_val{};
  T operator()() { return initial_val; }
};

template <class T>
struct ConsumeAndStore
{
  T last_val{};
  void operator()(const T& v)
  {
    last_val = v; // it returns a value to force a volatile read! }
  }
};
