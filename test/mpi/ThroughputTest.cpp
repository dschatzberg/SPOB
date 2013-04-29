#include "config.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <boost/random.hpp>
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
  bool no_comm = false;
  uint32_t outstanding;
  uint32_t string_size;
  uint32_t samples;
  double pfail;
  boost::random::mt19937 gen;
#if HAVE_PPC450_INLINES_H
  typedef uint64_t my_time_t;
  my_time_t GetTime() {
    return _bgp_GetTimeBase();
  }
  const std::string timer_unit("cycles");
  const double per_ms = 850000;
#elif HAVE_A2_INLINES_H
  typedef uint64_t my_time_t;
  my_time_t GetTime() {
    return GetTimeBase();
  }
  const std::string timer_unit("cycles");
  const double per_ms = 1600000;
#else
  boost::timer::cpu_timer timer;
  typedef boost::timer::nanosecond_type my_time_t;
  my_time_t GetTime() {
    return timer.elapsed().wall;
  }
  const std::string timer_unit("nanoseconds");
  const double per_ms = 1000000;
#endif
  my_time_t sample_time;
  my_time_t notify_time;

  std::string
  pair_to_string(const std::pair<double, uint64_t>& p)
  {
    std::ostringstream str;
    str << "(" << p.first << ", " << p.second << ")";
    return str.str();
  }
}

class Callback : public spob::StateMachine::Callback {
public:
  Callback(spob::StateMachine** sm)
    : sm_(sm), message_(string_size, '\0'), dist_(pfail)
  {
    mpi::communicator world;
    rank_ = world.rank();
    primary_ = 0;
    count_ = 0;
    last_count_ = 0;
    start_ = last_time_ = GetTime();
    tookover_ = 0;
    failed_ = 0;
    samples_ = 0;
    failed_nodes_ = new bool[world.size()];
  }

  ~Callback()
  {
    delete failed_nodes_;
  }

  void operator()(spob::StateMachine::Status status, uint32_t primary)
  {
    if (status == spob::StateMachine::kLeading) {
      tookover_ = GetTime();
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
    if ((GetTime() - last_time_) > (sample_time * per_ms)) {
      samples_++;
      if (samples_ == samples) {
        Dump();
        quit = true;
      } else {
        if (rank_ == primary_) {
          counts_.push_back(std::make_pair((GetTime() - start_) / per_ms,
                                           count_ - last_count_));
          last_count_ = count_;
        }
        last_time_ = GetTime();
        bool failed = !failed_ && dist_(gen);
        if (failed) {
          failed_ = GetTime();
          no_comm = true;
        }
        mpi::communicator world;
        mpi::all_gather(world, failed, failed_nodes_);
        for (int i = 0; i < world.size(); ++i) {
          if (failed_nodes_[i]) {
            failure_notify_.push(std::make_pair(i, static_cast<my_time_t>
                                                (last_time_ +
                                                notify_time * per_ms));
          }
        }
      }
    }
    while (!failure_notify_.empty() && failure_notify_.front().second <= GetTime()) {
      spob::Failure f;
      f.rank_ = failure_notify_.front().first;
      (*sm_)->Receive(f);
      failure_notify_.pop();
    }
  }
private:
  void Dump()
  {
    std::ostringstream ss;
    ss << rank_ << ": ";
    if (tookover_) {
      ss << "tookover at " << (tookover_ - start_) / per_ms
         << "ms ";
      if (!counts_.empty()) {
        std::transform(counts_.begin(), --(counts_.end()),
                       std::ostream_iterator<std::string>(ss, " "),
                       pair_to_string);
        ss << pair_to_string(counts_.back());
      }
    }
    if (failed_) {
      ss << "failed at " << (failed_ - start_) / per_ms << "ms";
    }
    std::cout << ss.str() << std::endl;
  }
  uint32_t primary_;
  uint32_t rank_;
  uint32_t samples_;
  uint64_t count_;
  uint64_t last_count_;
  spob::StateMachine** sm_;
  my_time_t start_;
  my_time_t last_time_;
  my_time_t tookover_;
  my_time_t failed_;
  std::list<std::pair<double, uint64_t> > counts_;
  std::string message_;
  boost::random::bernoulli_distribution<> dist_;
  boost::mpi::request req_;
  bool* failed_nodes_;
  std::queue<std::pair<uint32_t, my_time_t> > failure_notify_;
};

int main(int argc, char* argv[])
{
  po::options_description desc("Options");
  uint32_t seed;
  try {
    desc.add_options()
      ("help", "produce help message")
      ("v", po::value<bool>(&verbose)->default_value(false),
       "enable verbose output")
      ("samples", po::value<uint32_t>(&samples)->required(),
       "set number of samples")
      ("ts", po::value<my_time_t>(&sample_time)->required(),
       "set sample time (ms)")
      ("tn", po::value<my_time_t>(&notify_time)->required(),
       "set notify time (ms)")
      ("pfail", po::value<double>(&pfail)->required(),
       "set probability of failure at each sample")
      ("seed", po::value<uint32_t>(&seed)->default_value(0),
       "set random number generator seed")
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
  gen.seed(seed + world.rank());
  sm = new spob::StateMachine(world.rank(), world.size(), comm, cb);
  sm->Start();
  while (!quit) {
    if (!no_comm) {
      comm.Process();
    }
    cb.Process();
  }
  delete sm;
  return 0;
}
