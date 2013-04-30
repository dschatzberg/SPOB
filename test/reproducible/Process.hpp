#pragma once
#include <queue>
#include <vector>

#include <boost/coroutine/all.hpp>
#include <boost/variant.hpp>

#include "Communicator.hpp"
#include "ReproducibleTest.hpp"
#include "Spob.hpp"


struct Receive {
  uint32_t from;
};

struct Propose {
};

struct Continue {
};

struct Notify {
  uint32_t failed;
};

class Process : public spob::StateMachine::Callback {
public:
  class MessageHandler : public boost::static_visitor<> {
  public:
    MessageHandler(Process& p);
    template <typename T>
    void operator()(T& message) const;
    uint32_t from_;
  private:
    Process& p_;
  };
  Process(uint32_t rank);
  void operator()(boost::coroutines::coroutine<void()>::caller_type& ca);
  void Deliver(uint64_t id, const std::string& message);
  void StatusChange(spob::StateMachine::Status status, uint32_t primary);
  void TakeSnapshot(std::string& snapshot);
  void ApplySnapshot(const std::string& snapshot);
  Communicator comm_;
  spob::StateMachine sm_;
  typedef boost::variant<
    spob::ConstructTree,
    spob::AckTree,
    spob::NakTree,
    spob::RecoverPropose,
    spob::AckRecover,
    spob::RecoverCommit,
    spob::RecoverInform,
    spob::RecoverReconnect,
    spob::ListenerRecoverReconnect,
    spob::Propose,
    spob::Ack,
    spob::Commit,
    spob::Inform,
    spob::Reconnect,
    spob::ListenerReconnect,
    spob::ReconnectResponse,
    spob::ListenerReconnectResponse> Message;
  std::vector<std::queue<Message> > queues_;
  std::set<uint32_t> pending_queues_;
  std::set<uint32_t> unreported_;
  boost::coroutines::coroutine<void()>::caller_type* ca_;
  boost::variant<Receive, Propose, Continue, Notify> command_;
  MessageHandler mh_;
  uint32_t rank_;
  uint32_t pending_messages_;
  bool active_;
  bool failed_;
  bool can_propose_;
};
