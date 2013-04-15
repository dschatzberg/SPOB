#pragma once

#include <random>
#include <vector>

class Coroutine;

namespace {
  std::vector<Coroutine*> coroutines;
  std::vector<Coroutine*> runnable_coroutines;
  uint32_t size;
  std::default_random_engine rng;
  double propose_probability;
  int num_proposals;
  int max_proposals;
  bool verbose;
}
