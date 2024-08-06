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
  volatile T initial_val{};
  T operator()() { return initial_val++; }
};

template <class T>
struct ProduceSameValue
{
  volatile T initial_val{};
  T operator()() { return initial_val; }
};

template <class T>
struct ConsumeAndStore
{
  volatile T last_val{};
  void operator()(const T& v) { last_val = v; }
};
