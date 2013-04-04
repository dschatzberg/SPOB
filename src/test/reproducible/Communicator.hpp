#pragma once

#include <stdint.h>

#include "Spob.hpp"

class Coroutine;

class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(Coroutine& c, uint32_t rank, uint32_t size);
  void Send(const spob::ConstructTree& ct, uint32_t to);
  void Send(const spob::AckTree& at, uint32_t to);
  void Send(const spob::NakTree& nt, uint32_t to);
  void Send(const spob::RecoverPropose& rp, uint32_t to);
  void Send(const spob::AckRecover& ar, uint32_t to);
  void Send(const spob::RecoverCommit& rc, uint32_t to);
  void Send(const spob::Propose& p, uint32_t to);
  void Send(const spob::Ack& a, uint32_t to);
  void Send(const spob::Commit& c, uint32_t to);
  uint32_t size() const;
private:
  Coroutine& c_;
  uint32_t rank_;
  uint32_t size_;
};
