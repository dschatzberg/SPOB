#include "Coroutine.h"

Coroutine::Coroutine(std::vector<Coroutine*>& coroutines,
                     std::set<Coroutine*>& runnable,
                     uint32_t rank, uint32_t size,
                     std::default_random_engine& rng,
                     double p_propose, int& num_proposals, int max_proposals)
  : comm_(*this, rank, size),
    sm_(rank, comm_, fd_, Deliver, Status, reinterpret_cast<void*>(this)),
    coroutines_(coroutines), runnable_(runnable),
    rank_(rank), failed_(false), rng_(rng), dist_prop_(p_propose),
    max_proposals_(max_proposals), num_proposals_(num_proposals)
{
}

void
Coroutine::operator()(boost::coroutines::coroutine<void()>::caller_type& ca)
{
  ca_ = &ca;
  sm_.Start();
  std::bernoulli_distribution notify;
  while (1) {
    if (!queue_.empty() ||
        !fd_.unreported_.empty() ||
        (primary_ && num_proposals_ < max_proposals_) ){
      runnable_.insert(this);
    }
    ca();
    runnable_.erase(this);
    if ((!fd_.unreported_.empty() && notify(rng_)) || (queue_.empty() && !primary_)) {
      std::uniform_int_distribution<uint32_t> dist(0, fd_.unreported_.size() - 1);
      uint32_t report = dist(rng_);

      std::set<uint32_t>::iterator it = fd_.unreported_.begin();
      for (uint32_t i = 0; i < report; i++) {
        ++it;
      }
      uint32_t rank = *it;
      fd_.unreported_.erase(it);
      fd_.set_.insert(rank);
#ifdef LOG
      std::cout << rank_ << ": Detected " << rank << " failed" << std::endl;
#endif
      fd_.cb_(fd_.data_, rank);
    } else if ((primary_ && num_proposals_ < max_proposals_ && dist_prop_(rng_)) ||
               queue_.empty()) {
      num_proposals_++;
      sm_.Propose("message");
    } else {
      const Message& m = queue_.front();
      switch(m.type_) {
      case Message::ConstructTree:
#ifdef LOG
        std::cout << rank_ << ": Received ConstructTree from " <<
          m.from_ << ": " << m.ct_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.ct_, m.from_);
        break;
      case Message::AckTree:
#ifdef LOG
        std::cout << rank_ << ": Received AckTree from " <<
          m.from_ << ": " << m.at_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.at_, m.from_);
        break;
      case Message::NakTree:
#ifdef LOG
        std::cout << rank_ << ": Received NakTree from " <<
          m.from_ << ": " << m.nt_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.nt_, m.from_);
        break;
      case Message::RecoverPropose:
#ifdef LOG
        std::cout << rank_ << ": Received RecoverPropose from " <<
          m.from_ << ": " << m.rp_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.rp_, m.from_);
        break;
      case Message::AckRecover:
#ifdef LOG
        std::cout << rank_ << ": Received AckRecover from " <<
          m.from_ << ": " << m.ar_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.ar_, m.from_);
        break;
      case Message::RecoverReconnect:
#ifdef LOG
        std::cout << rank_ << ": Received RecoverReconnect from " <<
          m.from_ << ": " << m.rr_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.rr_, m.from_);
        break;
      case Message::RecoverCommit:
#ifdef LOG
        std::cout << rank_ << ": Received RecoverCommit from " <<
          m.from_ << ": " << m.rc_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.rc_, m.from_);
        break;
      case Message::Propose:
#ifdef LOG
        std::cout << rank_ << ": Received Propose from " <<
          m.from_ << ": " << m.p_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.p_, m.from_);
        break;
      case Message::Ack:
#ifdef LOG
        std::cout << rank_ << ": Received Ack from " <<
          m.from_ << ": " << m.a_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.a_, m.from_);
        break;
      case Message::Commit:
#ifdef LOG
        std::cout << rank_ << ": Received Commit from " <<
          m.from_ << ": " << m.c_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.c_, m.from_);
        break;
      case Message::Reconnect:
#ifdef LOG
        std::cout << rank_ << ": Received Reconnect from " <<
          m.from_ << ": " << m.r_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.r_, m.from_);
        break;
      case Message::ReconnectResponse:
#ifdef LOG
        std::cout << rank_ << ": Received ReconnectResponse from " <<
          m.from_ << ": " << m.recon_resp_.ShortDebugString() << std::endl;
#endif
        sm_.Receive(m.recon_resp_, m.from_);
        break;
      }
      queue_.pop();
    }
  }
}

void
Coroutine::Fail()
{
  failed_ = true;
  runnable_.erase(this);
}

void
Coroutine::Failure(uint32_t rank)
{
  fd_.unreported_.insert(rank);
  runnable_.insert(this);
}

void
Coroutine::Deliver(uint64_t id, const std::string& message)
{
#ifdef LOG
  std::cout << rank_ << ": Delivered: " << std::hex << id << std::dec <<
    ", " << message << std::endl;
#endif
  runnable_.insert(this);
  (*ca_)();
  if (primary_ && num_proposals_ < max_proposals_ && dist_prop_(rng_)) {
    num_proposals_++;
    sm_.Propose("message");
    runnable_.insert(this);
    (*ca_)();
  }
  runnable_.erase(this);
}

void
Coroutine::Status(spob::StateMachine::Status status, uint32_t primary)
{
  if (status == spob::StateMachine::LEADING) {
    primary_ = true;
  } else {
    primary_ = false;
  }
  runnable_.insert(this);
  (*ca_)();
  if (primary_ && num_proposals_ < max_proposals_ && dist_prop_(rng_)) {
    num_proposals_++;
    sm_.Propose("message");
    runnable_.insert(this);
    (*ca_)();
  }
  runnable_.erase(this);
}

void
Coroutine::Deliver(void* data, uint64_t id, const std::string& message)
{
  Coroutine* self = reinterpret_cast<Coroutine*>(data);
  self->Deliver(id, message);
}

void
Coroutine::Status(void* data, spob::StateMachine::Status status,
                  uint32_t primary)
{
  Coroutine* self = reinterpret_cast<Coroutine*>(data);
  self->Status(status, primary);
}
