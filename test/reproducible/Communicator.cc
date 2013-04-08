#include "Communicator.h"
#include "Coroutine.h"
#include "Message.h"

Communicator::Communicator(Coroutine& c, uint32_t rank, uint32_t size)
  : c_(c), rank_(rank), size_(size)
{
}

void
Communicator::Send(const spob::ConstructTree& ct, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending ConstructTree to " << to <<
    ": " << ct.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, ct);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::AckTree& at, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending AckTree to " << to <<
    ": " << at.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, at);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::NakTree& nt, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending NakTree to " << to <<
    ": " << nt.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, nt);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::RecoverPropose& rp, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending RecoverPropose to " << to <<
    ": " << rp.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, rp);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::AckRecover& ar, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending AckRecover to " << to <<
    ": " << ar.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, ar);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::RecoverReconnect& rr, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending RecoverReconnect to " << to <<
    ": " << rr.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, rr);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::RecoverCommit& rc, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending RecoverCommit to " << to <<
    ": " << rc.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, rc);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::Propose& p, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending Propose to " << to <<
    ": " << p.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, p);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::Ack& a, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending Ack to " << to <<
    ": " << a.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, a);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::Commit& c, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending Commit to " << to <<
    ": " << c.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, c);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::Reconnect& r, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending Reconnect to " << to <<
    ": " << r.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, r);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}
void
Communicator::Send(const spob::ReconnectResponse& recon_resp, uint32_t to)
{
#ifdef LOG
  std::cout << rank_ << ": Sending ReconnectResponse to " << to <<
    ": " << recon_resp.ShortDebugString() << std::endl;
#endif
  if (!c_.coroutines_[to]->failed_) {
    c_.coroutines_[to]->queue_.emplace(rank_, recon_resp);
    c_.runnable_.insert(c_.coroutines_[to]);
  }
  c_.runnable_.insert(&c_);
  (*(c_.ca_))();
  c_.runnable_.erase(&c_);
}

uint32_t
Communicator::size() const
{
  return size_;
}
