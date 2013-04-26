#include "config.h"

#include <algorithm>
#include <iterator>
#include <sstream>

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
  uint32_t outstanding;
  uint32_t string_size;
#if HAVE_PPC450_INLINES_H
  typedef uint64_t my_time_t;
  my_time_t GetTime() {
    return _bgp_GetTimeBase();
  }
  const std::string timer_unit("cycles");
  double per_ms = 850000;
#else
  boost::timer::cpu_timer timer;
  typedef boost::timer::nanosecond_type my_time_t;
  my_time_t GetTime() {
    return timer.elapsed().wall;
  }
  const std::string timer_unit("nanoseconds");
  double per_ms = 1000000;
#endif
  my_time_t total_time;
  my_time_t sample_time;
}

class Callback : public spob::StateMachine::Callback {
public:
  Callback(spob::StateMachine** sm)
    : sm_(sm), message_(string_size, '\0')
  {
    mpi::communicator world;
    rank_ = world.rank();
    primary_ = 0;
    count_ = 0;
    last_count_ = 0;
    start_ = last_time_ = GetTime();
  }

  void operator()(spob::StateMachine::Status status, uint32_t primary)
  {
    if (status == spob::StateMachine::kLeading) {
      if (verbose) {
        std::cout << rank_ << ": Recovered and Leading" << std::endl;
      }
      primary_ = primary;
      for (uint32_t i = 0; i < outstanding; i++) {
        (*sm_)->Propose(message_);
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
    if (rank_ == primary_) {
      (*sm_)->Propose(message_);
    }
  }
  void Process()
  {
    if ((GetTime() - start_) > (total_time * per_ms)) {
      if (rank_ == primary_ && !counts_.empty()) {
        std::ostringstream ss;
        std::copy(counts_.begin(), --(counts_.end()),
                  std::ostream_iterator<uint64_t>(ss, " "));
        ss << counts_.back();
        std::cout << ss.str() << std::endl;
      }
      quit = true;
    } else if ((GetTime() - last_time_) > (sample_time * per_ms)) {
      if (rank_ == primary_) {
        counts_.push_back(count_ - last_count_);
        last_count_ = count_;
        last_time_ = GetTime();
      }
    }
  }
private:
  uint32_t primary_;
  uint32_t rank_;
  uint64_t count_;
  uint64_t last_count_;
  spob::StateMachine** sm_;
  my_time_t start_;
  my_time_t last_time_;
  std::list<uint64_t> counts_;
  std::string message_;
};

int main(int argc, char* argv[])
{
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false),
       "enable verbose output")
      ("t", po::value<my_time_t>(&total_time)->required(),
       "set total time (ms)")
      ("ts", po::value<my_time_t>(&sample_time)->required(),
       "set sample time (ms)")
      ("no", po::value<uint32_t>(&outstanding)->required(),
       "set max number of outstanding messages")
      ("ss", po::value<uint32_t>(&string_size)->required(),
       "set string size of message")
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
  Callback cb(&sm);
  Communicator comm(&sm, verbose);
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), world.size(), comm, cb);
  sm->Start();
  while (!quit) {
    comm.Process();
    cb.Process();
  }
  delete sm;
  return 0;
}
