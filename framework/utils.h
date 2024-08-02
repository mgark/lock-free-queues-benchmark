#pragma once

#include <ctime>

constexpr uint64_t NS_PER_MICRO = 1000ul;
constexpr uint64_t MICRO_PER_SECOND = 1000ul;
constexpr uint64_t NS_PER_SECOND = NS_PER_MICRO * MICRO_PER_SECOND;

template <class T>
struct just_type
{
  using type = T;
};
