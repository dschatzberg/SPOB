#pragma once

#include "Spob.hpp"

class Message {
public:
  Message();
  Message(uint32_t from, spob::ConstructTree ct);
  Message(uint32_t from, spob::AckTree at);
  Message(uint32_t from, spob::NakTree nt);
  Message(uint32_t from, spob::RecoverPropose rp);
  Message(uint32_t from, spob::AckRecover ar);
  Message(uint32_t from, spob::RecoverReconnect rr);
  Message(uint32_t from, spob::RecoverCommit rc);
  Message(uint32_t from, spob::Propose p);
  Message(uint32_t from, spob::Ack a);
  Message(uint32_t from, spob::Commit c);
  Message(uint32_t from, spob::Reconnect r);
  Message(uint32_t from, spob::ReconnectResponse recon_resp);
  enum Type {
    ConstructTree,
    AckTree,
    NakTree,
    RecoverPropose,
    AckRecover,
    RecoverReconnect,
    RecoverCommit,
    Propose,
    Ack,
    Commit,
    Reconnect,
    ReconnectResponse
  };
  Type type_;
  uint32_t from_;
  union {
    spob::ConstructTree ct_;
    spob::AckTree at_;
    spob::NakTree nt_;
    spob::RecoverPropose rp_;
    spob::AckRecover ar_;
    spob::RecoverReconnect rr_;
    spob::RecoverCommit rc_;
    spob::Propose p_;
    spob::Ack a_;
    spob::Commit c_;
    spob::Reconnect r_;
    spob::ReconnectResponse recon_resp_;
  };
  ~Message();
};
