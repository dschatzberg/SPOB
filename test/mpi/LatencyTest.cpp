#include "config.h"

#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#if HAVE_PPC450_INLINES_H
#include <bpcore/ppc450_inlines.h>
#elif HAVE_A2_INLINES_H
#include <hwi/include/bqc/A2_inlines.h>
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
  uint32_t string_size;
#if HAVE_PPC450_INLINES_H
  typedef uint64_t my_time_t;
  my_time_t GetTime() {
    return _bgp_GetTimeBase();
  }
  const std::string timer_unit("cycles");
#elif HAVE_A2_INLINES_H
  typedef uint64_t my_time_t;
  my_time_t GetTime() {
    return GetTimeBase();
  }
  const std::string timer_unit("cycles");
#else
  boost::timer::cpu_timer timer;
  typedef boost::timer::nanosecond_type my_time_t;
  my_time_t GetTime() {
    return timer.elapsed().wall;
  }
  const std::string timer_unit("nanoseconds");
#endif
}

class Callback : public spob::StateMachine::Callback {
public:
  Callback(spob::StateMachine** sm, long int num_messages)
    : n_(num_messages), sm_(sm), message_(string_size, '\0')
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
      (*sm_)->Propose(std::string(string_size, ' '));
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
        std::cout << "Mean = " << new_mean_ << " " << timer_unit
                  << ", StdDev = " << std::sqrt(new_square_/(count_ - 1))
                  << " " << timer_unit << std::endl;
      }
    } else if (rank_ == primary_) {
      my_time_t sample = GetTime() - start_;
      if (count_ == 1) {
        old_mean_ = new_mean_ = sample;
        old_square_ = 0.0;
      } else {
        new_mean_ = old_mean_ + (sample - old_mean_)/count_;
        new_square_ = old_square_ + (sample - old_mean_)*(sample - new_mean_);

        old_mean_ = new_mean_;
        old_square_ = new_square_;
      }
      start_ = GetTime();
      (*sm_)->Propose(message_);
    }
  }
private:
  uint32_t primary_;
  uint32_t rank_;
  long int count_;
  long int n_;
  spob::StateMachine** sm_;
  my_time_t start_;
  double old_mean_;
  double new_mean_;
  double old_square_;
  double new_square_;
  std::string message_;
};

int main(int argc, char* argv[])
{
  int num_messages;
  uint32_t replicas;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false),
       "enable verbose output")
      ("r", po::value<uint32_t>(&replicas)->required(),
       "set number of replicas")
      ("nm", po::value<int>(&num_messages)->required(),
       "set number of messages")
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
  Callback cb(&sm, num_messages);
  Communicator comm(&sm, verbose);
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), world.size(), replicas, comm, cb);
  sm->Start();
  while (!quit) {
    comm.Process();
  }
  delete sm;
  return 0;
}
