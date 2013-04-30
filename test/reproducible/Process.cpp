#include "Process.hpp"

Process::MessageHandler::MessageHandler(Process& p) : p_(p) {}

template <typename T>
void Process::MessageHandler::operator()(T& message) const
{
  if (verbose) {
    std::cout << p_.rank_ << ": Received " << message << " from " <<
      from_ << std::endl;
  }
  p_.sm_.Receive(message, from_);
}

Process::Process(uint32_t rank)
  : comm_(*this), sm_(rank, size, replicas, comm_, *this), queues_(size),
    mh_(*this), rank_(rank),
    pending_messages_(0), active_(false), failed_(false)
{
}

void
Process::operator()(boost::coroutines::coroutine<void()>::caller_type& ca)
{
  ca_ = &ca;
  sm_.Start();
  while (1) {
    active_ = false;
    ca();
    if (Receive* r = boost::get<Receive>(&command_)) {
      uint32_t index = r->from;
      mh_.from_ = index;
      boost::apply_visitor(mh_, queues_[index].front());
      queues_[index].pop();
      if (queues_[index].empty()) {
        pending_queues_.erase(index);
      }
      pending_messages_--;
      if (pending_messages_ == 0 && unreported_.empty()) {
        runnable_processes.erase(this);
      }
    } else if (boost::get<Propose>(&command_)) {
      num_proposals++;
      sm_.Propose("test");
    } else if (Notify* n = boost::get<Notify>(&command_)) {
      uint32_t failed = n->failed;
      unreported_.erase(failed);
      if (pending_messages_ == 0 && unreported_.empty()) {
        runnable_processes.erase(this);
      }
      if (verbose) {
        std::cout << rank_ << ": Detected " << failed << " failed" << std::endl;
      }
      spob::Failure f;
      f.rank_ = failed;
      sm_.Receive(f);
    }
  }
}

void
Process::Deliver(uint64_t id, const std::string& message)
{
}

void
Process::StatusChange(spob::StateMachine::Status status, uint32_t p)
{
  if (status == spob::StateMachine::kLeading) {
    primary = rank_;
    can_propose_ = true;
    active_ = true;
    runnable_processes.insert(this);
    (*ca_)();
    if (pending_messages_ == 0 && unreported_.empty()) {
      runnable_processes.erase(this);
    }
    if (boost::get<Propose>(&command_)) {
      num_proposals++;
      sm_.Propose("test");
    }
  }
}

void
Process::TakeSnapshot(std::string& snapshot)
{
}
void
Process::ApplySnapshot(const std::string& snapshot)
{
}
