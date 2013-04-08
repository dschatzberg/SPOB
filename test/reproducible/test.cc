#include <cstdlib>

#include <random>

#include <boost/coroutine/all.hpp>

#include "Coroutine.h"

typedef boost::coroutines::coroutine<void()> coroutine_t;

int main (int argc, char* argv[])
{
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0] << " num_processes num_messages"
      " propose_probability failure_probability seed" << std::endl;
    return -1;
  }

  long int num_processes = strtol(argv[1], NULL, 10);
  if (num_processes <= 0) {
    std::cerr << "Invalid number of processes" << std::endl;
    return -1;
  }

  long int num_messages = strtol(argv[2], NULL, 10);
  if (num_messages <= 0) {
    std::cerr << "Invalid number of messages" << std::endl;
    return -1;
  }

  double propose_probability = strtod(argv[3], NULL);
  if (propose_probability < 0 || propose_probability > 1) {
    std::cerr << "Invalid propose probability" << std::endl;
    return -1;
  }

  double failure_probability = strtod(argv[4], NULL);
  if (failure_probability < 0 || failure_probability > 1) {
    std::cerr << "Invalid failure probability" << std::endl;
    return -1;
  }

  long int seed = strtol(argv[5], NULL, 10);
  if (seed < 0) {
    std::cerr << "Invalid seed" << std::endl;
    return -1;
  }
  std::default_random_engine rng(seed);
  std::vector<Coroutine*> coroutine_info(num_processes);
  std::set<Coroutine*> runnable;
  int num_proposals = 0;
  for (int i = 0; i < num_processes; ++i) {
    coroutine_info[i] = new Coroutine(coroutine_info, runnable, i, num_processes, rng,
                                      propose_probability, num_proposals, num_messages);
  }

  std::vector<Coroutine*> alive(coroutine_info);

  std::vector<coroutine_t> coroutines;
  for (int i = 0; i < num_processes; ++i) {
    coroutines.push_back(coroutine_t(*coroutine_info[i]));
  }

  std::bernoulli_distribution failure(failure_probability);
  while (1) {
    if (runnable.size() == 0) {
      break;
    }

    if (failure(rng)) {
      std::uniform_int_distribution<uint32_t> dist(0, alive.size() - 1);
      uint32_t next = dist(rng);
      uint32_t rank = alive[next]->rank_;
#ifdef LOG
      std::cout << rank << " failed" << std::endl;
#endif
      alive[next]->Fail();
      alive.erase(alive.begin()+next);
      for (unsigned i = 0; i < alive.size(); i++) {
        alive[i]->Failure(rank);
      }
      runnable.insert(alive.begin(), alive.end());
    } else {
      std::uniform_int_distribution<uint32_t> dist(0, runnable.size() - 1);
      uint32_t next = dist(rng);

      std::set<Coroutine*>::const_iterator it = runnable.begin();
      for (uint32_t i = 0; i < next; ++i) {
        ++it;
      }
      coroutines[(*it)->rank_]();
    }
  }
  return EXIT_SUCCESS;
}
