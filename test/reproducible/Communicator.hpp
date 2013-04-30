#pragma once

#include <stdint.h>

#include "Spob.hpp"

class Process;

class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(Process& p);
  void Send(const spob::ConstructTree& ct, uint32_t to);
  void Send(const spob::AckTree& at, uint32_t to);
  void Send(const spob::NakTree& nt, uint32_t to);
  void Send(const spob::RecoverPropose& rp, uint32_t to);
  void Send(const spob::AckRecover& ar, uint32_t to);
  void Send(const spob::RecoverReconnect& rr, uint32_t to);
  void Send(const spob::ListenerRecoverReconnect& lrr, uint32_t to);
  void Send(const spob::RecoverCommit& rc, uint32_t to);
  void Send(const spob::RecoverInform& ri, uint32_t to);
  void Send(const spob::Propose& p, uint32_t to);
  void Send(const spob::Ack& a, uint32_t to);
  void Send(const spob::Commit& c, uint32_t to);
  void Send(const spob::Inform& i, uint32_t to);
  void Send(const spob::Reconnect& r, uint32_t to);
  void Send(const spob::ListenerReconnect& lr, uint32_t to);
  void Send(const spob::ReconnectResponse& recon_resp, uint32_t to);
  void Send(const spob::ListenerReconnectResponse& lrecon_resp, uint32_t to);
private:
  template <typename T>
  void DoSend(const T& t, uint32_t to);
  Process& p_;
};
