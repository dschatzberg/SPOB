#include "config.h"

#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#if HAVE_PPC450_INLINES_H
#include <bpcore/ppc450_inlines.h>
#else
#include <boost/timer/timer.hpp>
#endif

#include "Communicator.hpp"
#include "Spob.hpp"

namespace po = boost::program_options;
namespace mpi = boost::mpi;

namespace {
  bool quit = false;
  bool verbose;
  int outstanding;
#if HAVE_PPC450_INLINES_H
  typedef uint64_t time_t;
  time_t GetTime() {
    return _bgp_GetTimeBase();
  }
  const std::string timer_unit("cycles");
#else
  boost::timer::cpu_timer timer;
  typedef boost::timer::nanosecond_type time_t;
  time_t GetTime() {
    return timer.elapsed().wall;
  }
  const std::string timer_unit("nanoseconds");
#endif
}

class Callback : public spob::StateMachine::Callback {
public:
  Callback(spob::StateMachine** sm, long int num_messages)
    : n_(num_messages), sm_(sm)
  {
    mpi::communicator world;
    rank_ = world.rank();
    primary_ = 0;
    count_ = 0;
  }

  void operator()(spob::StateMachine::Status status, uint32_t primary)
  {
    if (status == spob::StateMachine::kLeading) {
      if (verbose) {
        std::cout << rank_ << ": Recovered and Leading" << std::endl;
      }
      primary_ = primary;
      start_ = GetTime();
      for (int i = 0; i < outstanding; i++) {
        (*sm_)->Propose("test");
      }
    } else if (spob::StateMachine::kFollowing) {
      primary_ = primary;
      if (verbose) {
        std::cout << rank_ << ": Recovered and Following " << primary << std::endl;
      }
    }
  }
  void operator()(uint64_t id, const std::string& message)
  {
    count_++;
    if (verbose) {
      std::cout << rank_ << ": Delivered message: 0x" << std::hex << id <<
        std::dec << ", \"" << message << "\"" << std::endl;
    }
    if (count_ == n_) {
      quit = true;
      if (rank_ == primary_) {
        time_t delta = GetTime() - start_;
        std::cout << "Output " << n_ << " messages in " << delta << " "
                  << timer_unit << std::endl;
      }
    } else if (rank_ == primary_) {
      (*sm_)->Propose("test");
    }
  }
private:
  uint32_t primary_;
  uint32_t rank_;
  long int count_;
  long int n_;
  spob::StateMachine** sm_;
  time_t start_;
};

int main(int argc, char* argv[])
{
  int num_messages;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false),
       "enable verbose output")
      ("nm", po::value<int>(&num_messages)->required(),
       "set number of messages")
      ("no", po::value<int>(&outstanding)->required(),
       "set max number of outstanding messages")
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

  mpi::environment env(argc, argv);
  spob::StateMachine* sm;
  Callback cb(&sm, num_messages);
  Communicator comm(&sm, verbose);
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), world.size(), comm, cb);
  sm->Start();
  while (!quit) {
    comm.Process();
  }
  delete sm;
  return 0;
}
