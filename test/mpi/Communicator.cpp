#include <boost/serialization/list.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/utility.hpp>

#include "Communicator.hpp"

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

Communicator::ReceiveVisitor::ReceiveVisitor(Communicator& comm) : comm_(comm) {}

template <typename T>
void
Communicator::ReceiveVisitor::operator()(T& t) const
{
  if (comm_.verbose_) {
    std::cout << comm_.rank_ << ": Received " << t << std::endl;
  }
  (*comm_.sm_)->Receive(t, opt_status_->source());
}

namespace mpi = boost::mpi;

Communicator::Communicator(spob::StateMachine** sm, bool verbose)
  : sm_(sm), rv_(*this), verbose_(verbose)
{
  mpi::communicator world;
  req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
  rank_ = world.rank();
}

template <typename T>
void
Communicator::DoSend(const T& t, uint32_t to)
{
  if (verbose_) {
    std::cout << rank_ << ": Sending " << t << " to " << to << std::endl;
  }
  mpi::communicator world;
  pending_.push(std::make_pair(mpi::request(), Message(t)));
  pending_.back().first = world.isend(to, 0, pending_.back().second);
}

void
Communicator::Send(const spob::ConstructTree& ct, uint32_t to)
{
  DoSend(ct, to);
}

void
Communicator::Send(const spob::AckTree& at, uint32_t to)
{
  DoSend(at, to);
}

void
Communicator::Send(const spob::NakTree& nt, uint32_t to)
{
  DoSend(nt, to);
}

void
Communicator::Send(const spob::RecoverPropose& rp, uint32_t to)
{
  DoSend(rp, to);
}

void
Communicator::Send(const spob::AckRecover& ar, uint32_t to)
{
  DoSend(ar, to);
}

void
Communicator::Send(const spob::RecoverReconnect& rr, uint32_t to)
{
  DoSend(rr, to);
}

void
Communicator::Send(const spob::RecoverCommit& rc, uint32_t to)
{
  DoSend(rc, to);
}

void
Communicator::Send(const spob::Propose& p, uint32_t to)
{
  DoSend(p, to);
}

void
Communicator::Send(const spob::Ack& a, uint32_t to)
{
  DoSend(a, to);
}

void
Communicator::Send(const spob::Commit& c, uint32_t to)
{
  DoSend(c, to);
}

void
Communicator::Send(const spob::Reconnect& r, uint32_t to)
{
  DoSend(r, to);
}

void
Communicator::Send(const spob::ReconnectResponse& recon_resp, uint32_t to)
{
  DoSend(recon_resp, to);
}

void
Communicator::Process()
{
  while (!pending_.empty() && pending_.front().first.test()) {
    pending_.pop();
  }
  rv_.opt_status_ = req_.test();
  if (rv_.opt_status_) {
    boost::apply_visitor(rv_, message_);
    mpi::communicator world;
    req_ = world.irecv(mpi::any_source, 0, message_);
  }
}

Communicator::~Communicator()
{
  req_.cancel();
}
