#include <cstdlib>

#include <random>

#include <boost/coroutine/all.hpp>

#include "test/reproducible/Coroutine.hpp"

typedef boost::coroutines::coroutine<void()> coroutine_t;

int main (int argc, char* argv[])
{
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " num_processes num_messages seed" <<
      std::endl;
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

  long int seed = strtol(argv[3], NULL, 10);
  if (seed < 0) {
    std::cerr << "Invalid seed" << std::endl;
    return -1;
  }
  std::vector<Coroutine*> coroutine_info(num_processes);
  std::set<Coroutine*> runnable;
  for (int i = 0; i < num_processes; ++i) {
    coroutine_info[i] = new Coroutine(coroutine_info, runnable, i, num_processes);
  }

  std::vector<coroutine_t> coroutines;
  for (int i = 0; i < num_processes; ++i) {
    coroutines.push_back(coroutine_t(*coroutine_info[i]));
  }

  std::default_random_engine rng(seed);
  while (1) {
    if (runnable.size() == 0) {
      break;
    }
    std::uniform_int_distribution<uint32_t> dist(0, runnable.size() - 1);
    uint32_t next = dist(rng);

    std::set<Coroutine*>::const_iterator it = runnable.begin();
    for (int i = 0; i < next; ++i) {
      ++it;
    }
    coroutines[(*it)->rank_]();
  }
  return EXIT_SUCCESS;
}
