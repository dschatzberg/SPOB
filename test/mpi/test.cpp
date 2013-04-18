#include "config.h"

#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/variant.hpp>
#if HAVE_PPC450_INLINES_H
#include <bpcore/ppc450_inlines.h>
#else
#include <boost/timer/timer.hpp>
#endif

#include "Spob.hpp"

namespace mpi = boost::mpi;
namespace po = boost::program_options;

namespace {
  bool quit = false;
  bool verbose;
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
#if HAVE_PPC450_INLINES_H
      start_ = _bgp_GetTimeBase();
#else
      start_ = timer_.elapsed().wall;
#endif
      (*sm_)->Propose("test");
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
        std::cout << "Mean = " << new_mean_ << " ";
#if HAVE_PPC450_INLINES_H
        std::cout << "cycles";
#else
        std::cout << "nanoseconds";
#endif
        std::cout << ", StdDev = " << std::sqrt(new_square_/(count_ - 1)) << " ";
#if HAVE_PPC450_INLINES_H
        std::cout << "cycles";
#else
        std::cout << "nanoseconds";
#endif
        std::cout << std::endl;
      }
    } else if (rank_ == primary_) {
#if HAVE_PPC450_INLINES_H
      uint64_t sample = _bgp_GetTimeBase() - start_;
#else
      boost::timer::nanosecond_type sample = timer_.elapsed().wall - start_;
#endif
      if (count_ == 1) {
        old_mean_ = new_mean_ = sample;
        old_square_ = 0.0;
      } else {
        new_mean_ = old_mean_ + (sample - old_mean_)/count_;
        new_square_ = old_square_ + (sample - old_mean_)*(sample - new_mean_);

        old_mean_ = new_mean_;
        old_square_ = new_square_;
      }
#if HAVE_PPC450_INLINES_H
      start_ = _bgp_GetTimeBase();
#else
      start_ = timer_.elapsed().wall;
#endif
      (*sm_)->Propose("test");
    }
  }
private:
  uint32_t primary_;
  uint32_t rank_;
  long int count_;
  long int n_;
  spob::StateMachine** sm_;
#if HAVE_PPC450_INLINES_H
  uint64_t start_;
#else
  boost::timer::cpu_timer timer_;
  boost::timer::nanosecond_type start_;
#endif
  double old_mean_;
  double new_mean_;
  double old_square_;
  double new_square_;
};

class receive_visitor : public boost::static_visitor<>
{
public:
  template <typename T>
  void operator()(T& t) const
  {
    if (verbose) {
      std::cout << rank_ << ": Received " << t << std::endl;
    }
    (*sm_)->Receive(t, opt_status_->source());
  }
  boost::optional<mpi::status> opt_status_;
  spob::StateMachine** sm_;
  uint32_t rank_;
};

namespace boost {
  namespace serialization {
    template<class Archive>
    inline void
    serialize(Archive &ar, spob::ConstructTree &ct, const unsigned int file_version)
    {
      ar & ct.max_rank_;
      ar & ct.count_;
      ar & ct.ancestors_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::AckTree &at, const unsigned int file_version)
    {
      ar & at.primary_;
      ar & at.count_;
      ar & at.last_mid_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::NakTree &nt, const unsigned int file_version)
    {
      ar & nt.primary_;
      ar & nt.count_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::RecoverPropose &rp, const unsigned int file_version)
    {
      ar & rp.primary_;
      ar & rp.type_;
      ar & rp.proposals_;
      ar & rp.last_mid_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::AckRecover &are, const unsigned int file_version)
    {
      ar & are.primary_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::RecoverCommit &rc, const unsigned int file_version)
    {
      ar & rc.primary_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::RecoverReconnect &rr, const unsigned int file_version)
    {
      ar & rr.primary_;
      ar & rr.max_rank_;
      ar & rr.last_proposed_;
      ar & rr.got_propose_;
      ar & rr.acked_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::Propose &p, const unsigned int file_version)
    {
      ar & p.primary_;
      ar & p.proposal_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::Ack &a, const unsigned int file_version)
    {
      ar & a.primary_;
      ar & a.mid_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::Commit &c, const unsigned int file_version)
    {
      ar & c.primary_;
      ar & c.mid_;
    }

    template<class Archive>
    inline void
    serialize(Archive &ar, spob::Reconnect &r, const unsigned int file_version)
    {
      ar & r.primary_;
      ar & r.max_rank_;
      ar & r.last_proposed_;
      ar & r.last_acked_;
    }

    template<class Archive>
    inline void
    serialize(Archive& ar, spob::ReconnectResponse& rr, const unsigned int file_version)
    {
      ar & rr.primary_;
      ar & rr.last_committed_;
      ar & rr.proposals_;
    }
  }
}
class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(spob::StateMachine** sm) : sm_(sm)
  {
    mpi::communicator world;
    req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
    rank_ = world.rank();
    rv_.sm_ = sm_;
    rv_.rank_ = rank_;
  }

  typedef boost::variant<
    spob::ConstructTree,
    spob::AckTree,
    spob::NakTree,
    spob::RecoverPropose,
    spob::AckRecover,
    spob::RecoverCommit,
    spob::RecoverReconnect,
    spob::Propose,
    spob::Ack,
    spob::Commit,
    spob::Reconnect,
    spob::ReconnectResponse> Message;

  void
  Send(const spob::ConstructTree& ct, uint32_t to)
  {
    DoSend(ct, to);
  }
  void
  Send(const spob::AckTree& at, uint32_t to)
  {
    DoSend(at, to);
  }
  void
  Send(const spob::NakTree& nt, uint32_t to)
  {
    DoSend(nt, to);
  }
  void
  Send(const spob::RecoverPropose& rp, uint32_t to)
  {
    DoSend(rp, to);
  }
  void
  Send(const spob::AckRecover& ar, uint32_t to)
  {
    DoSend(ar, to);
  }
  void
  Send(const spob::RecoverReconnect& rr, uint32_t to)
  {
    DoSend(rr, to);
  }
  void
  Send(const spob::RecoverCommit& rc, uint32_t to)
  {
    DoSend(rc, to);
  }
  void
  Send(const spob::Propose& p, uint32_t to)
  {
    DoSend(p, to);
  }
  void
  Send(const spob::Ack& a, uint32_t to)
  {
    DoSend(a, to);
  }
  void
  Send(const spob::Commit& c, uint32_t to)
  {
    DoSend(c, to);
  }
  void
  Send(const spob::Reconnect& r, uint32_t to)
  {
    DoSend(r, to);
  }
  void
  Send(const spob::ReconnectResponse& recon_resp, uint32_t to)
  {
    DoSend(recon_resp, to);
  }

  void Process()
  {
    rv_.opt_status_ = req_.test();
    if (rv_.opt_status_) {
      boost::apply_visitor(rv_, message_);
      mpi::communicator world;
      req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
    }
  }

  ~Communicator()
  {
    req_.cancel();
  }
private:
  template <typename T>
  void DoSend(const T& t, uint32_t to)
  {
    if (verbose) {
      std::cout << rank_ << ": Sending " << t << " to " << to << std::endl;
    }
    mpi::communicator world;
    world.send(to, 0, Message(t));;
  }

  spob::StateMachine** sm_;
  receive_visitor rv_;
  mpi::request req_;
  Message message_;
  uint32_t rank_;
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
  Communicator comm(&sm);
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), world.size(), comm, cb);
  sm->Start();
  while (!quit) {
    comm.Process();
  }
  delete sm;
  return 0;
}
