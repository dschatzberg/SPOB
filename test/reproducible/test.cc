#include <cstdlib>

#include <random>

#include <boost/coroutine/all.hpp>
#include <boost/program_options.hpp>

#include "Coroutine.h"

typedef boost::coroutines::coroutine<void()> coroutine_t;

namespace po = boost::program_options;

int main (int argc, char* argv[])
{
  bool verbose;
  int num_processes;
  int num_messages;
  double propose_probability;
  double failure_probability;
  int seed;

  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false), "enable verbose output")
      ("np", po::value<int>(&num_processes)->required(), "set number of processes")
      ("nm", po::value<int>(&num_messages)->required(), "set number of messages")
      ("p-prop", po::value<double>(&propose_probability)->required(),
       "set proposal probability")
      ("p-fail", po::value<double>(&failure_probability)->required(),
       "set failure probability")
      ("seed", po::value<int>(&seed)->required(), "set random number seed")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    po::notify(vm);
  } catch (std::exception& e) {
    std::cout << desc << std::endl;
    std::cout << e.what() << std::endl;
    return 1;
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
