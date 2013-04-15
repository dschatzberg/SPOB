#pragma once

#include <boost/coroutine/all.hpp>

#include "ReproducibleTest.hpp"

#include "Communicator.h"
#include "Spob.h"

class Coroutine : public Callback {
public:
  Coroutine(uint32_t rank);
  void operator()(boost::coroutines::coroutine<void()>::caller_type& ca);

  void operator()(uint64_t id, const std::string& message);
  void operator()(spob::StateMachine::Status status, uint32_t primary);

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
  std::vector<std::queue<Message> > queues_;
  Communicator comm_;
  spob::StateMachine sm_;
  boost::coroutines::coroutine<void()>::caller_type* ca_;
  uint32_t rank_;
  bool failed_;
  bool primary_;
  std::bernoulli_distribution dist_prop_;
};
