#include "Communicator.hpp"
#include "Process.hpp"
#include "ReproducibleTest.hpp"


Communicator::Communicator(Process& p)
  : p_(p)
{
}

template <typename T>
void
Communicator::DoSend(const T& t, uint32_t to)
{
  if (p_.active_) {
    bool temp = p_.can_propose_;
    p_.can_propose_ = false;
    runnable_processes.insert(&p_);
    (*p_.ca_)();
    if (p_.pending_messages_ == 0 && p_.unreported_.empty()) {
      runnable_processes.erase(&p_);
    }
    p_.can_propose_ = temp;
  }
  if (verbose) {
    std::cout << p_.rank_ << ": Sending " << t << " to " << to << std::endl;
  }
  if (!processes[to]->failed_) {
    processes[to]->queues_[p_.rank_].push(t);
    processes[to]->pending_messages_++;
    processes[to]->pending_queues_.insert(p_.rank_);
    runnable_processes.insert(processes[to]);
  }
  p_.active_ = true;
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
Communicator::Send(const spob::RecoverInform& ri, uint32_t to)
{
  DoSend(ri, to);
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
Communicator::Send(const spob::Inform& i, uint32_t to)
{
  DoSend(i, to);
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
