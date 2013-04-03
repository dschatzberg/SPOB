#pragma once

#include <queue>
#include <vector>

#include <boost/coroutine/all.hpp>

#include "Communicator.hpp"
#include "FailureDetector.hpp"
#include "Message.hpp"
#include "Spob.hpp"

class Coroutine {
public:
  Coroutine(std::vector<Coroutine*>& coroutines, std::set<Coroutine*>& runnable,
            uint32_t rank, uint32_t size);
  void operator()(boost::coroutines::coroutine<void()>::caller_type& ca);

  static void Deliver(void* data, uint64_t id, const std::string& message);
  static void Status(void* data, spob::StateMachine::Status status,
                     uint32_t primary);

  std::queue<Message> queue_;
  Communicator comm_;
  FailureDetector fd_;
  spob::StateMachine sm_;
  std::vector<Coroutine*>& coroutines_;
  std::set<Coroutine*>& runnable_;
  boost::coroutines::coroutine<void()>::caller_type* ca_;
  uint32_t rank_;
};
