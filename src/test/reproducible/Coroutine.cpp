#include "test/reproducible/Coroutine.hpp"

Coroutine::Coroutine(std::vector<Coroutine*>& coroutines,
                     std::set<Coroutine*>& runnable,
                     uint32_t rank, uint32_t size)
  : comm_(*this, rank, size),
    sm_(rank, comm_, fd_, Deliver, Status, reinterpret_cast<void*>(this)),
    coroutines_(coroutines), runnable_(runnable),
    rank_(rank)
{
}

void
Coroutine::operator()(boost::coroutines::coroutine<void()>::caller_type& ca)
{
  ca_ = &ca;
  sm_.Start();
  while (1) {
    if (queue_.size() > 0) {
      runnable_.insert(this);
    }
    ca();
    runnable_.erase(this);
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
    }
    queue_.pop();
  }
}

void
Coroutine::Deliver(void* data, uint64_t id, const std::string& message)
{
}

void
Coroutine::Status(void* data, spob::StateMachine::Status status,
                  uint32_t primary)
{
}
