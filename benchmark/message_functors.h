#pragma once

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
