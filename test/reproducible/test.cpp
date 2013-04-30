#include <cstdlib>

#include <boost/coroutine/all.hpp>
#include <boost/program_options.hpp>

#include "Process.hpp"
#include "ReproducibleTest.hpp"

typedef boost::coroutines::coroutine<void()> coroutine_t;

std::vector<Process*> processes;
uint32_t size;
bool verbose;
std::set<Process*> runnable_processes;
int primary = -1;
int max_proposals;
int num_proposals = 0;
uint32_t replicas;

namespace po = boost::program_options;

int main (int argc, char* argv[])
{
  double propose_probability;
  double failure_probability;
  int seed;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false), "enable verbose output")
      ("r", po::value<uint32_t>(&replicas)->required(),
       "set number of replicas")
      ("np", po::value<uint32_t>(&size)->required(), "set number of processes")
      ("nm", po::value<int>(&max_proposals)->required(), "set number of messages")
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

  processes.resize(size);
  for (uint32_t i = 0; i < size; ++i) {
    processes[i] = new Process(i);
  }

  std::set<Process*> alive_processes(processes.begin(), processes.end());

  std::vector<coroutine_t> coroutines;
  for (uint32_t i = 0; i < size; ++i) {
    coroutines.push_back(coroutine_t(*processes[i]));
  }

  std::default_random_engine rng(seed);
  std::bernoulli_distribution propose(propose_probability);
  std::bernoulli_distribution fail(failure_probability);
  while (1) {
    if ((runnable_processes.size() == 0 &&
         num_proposals == max_proposals) ||
        alive_processes.size() == 1) {
      break;
    }

    if (fail(rng)) {
      std::uniform_int_distribution<uint32_t> dist(0, alive_processes.size() - 1);
      uint32_t next = dist(rng);
      std::set<Process*>::iterator it = alive_processes.begin();
      std::advance(it, next);
      alive_processes.erase(it);
      Process* p = *it;
      if (verbose) {
        std::cout << "Process " << p->rank_ << " failed" << std::endl;
      }
      p->failed_ = true;
      runnable_processes.erase(p);
      if ((int)p->rank_ == primary) {
        primary = -1;
      }
      for (std::set<Process*>::iterator it = alive_processes.begin();
           it != alive_processes.end(); ++it) {
        (*it)->unreported_.insert(p->rank_);
      }
      runnable_processes.insert(alive_processes.begin(), alive_processes.end());
    } else if (primary != -1 && num_proposals < max_proposals &&
        processes[primary]->can_propose_ &&
        (runnable_processes.size() == 0 || propose(rng))) {
      processes[primary]->command_ = Propose();
      coroutines[primary]();
    } else if (runnable_processes.size() > 0) {
      // Choose a process to run
      std::uniform_int_distribution<uint32_t> dist(0, runnable_processes.size() - 1);
      uint32_t next = dist(rng);
      std::set<Process*>::iterator it = runnable_processes.begin();
      std::advance(it, next);
      Process* p = *it;

      if (p->active_) {
        p->command_ = Continue();
        coroutines[p->rank_]();
      } else {
        std::bernoulli_distribution notify;
        if (p->pending_messages_ == 0 ||
            (!p->unreported_.empty() && notify(rng))) {
          std::uniform_int_distribution<uint32_t> dist(0, p->unreported_.size() - 1);
          uint32_t next = dist(rng);
          std::set<uint32_t>::iterator it = p->unreported_.begin();
          std::advance(it, next);
          Notify n;
          n.failed = *it;
          p->command_ = n;
          coroutines[p->rank_]();
        } else {
          std::uniform_int_distribution<uint32_t> dist(0, p->pending_queues_.size() - 1);
          uint32_t next = dist(rng);
          std::set<uint32_t>::iterator it = p->pending_queues_.begin();
          std::advance(it, next);
          Receive r;
          r.from = *it;
          p->command_ = r;
          coroutines[p->rank_]();
        }
      }
    }
  }
  return EXIT_SUCCESS;
}
