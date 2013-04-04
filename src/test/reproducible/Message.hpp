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
  Message(uint32_t from, spob::RecoverCommit rc);
  Message(uint32_t from, spob::Propose p);
  Message(uint32_t from, spob::Ack a);
  Message(uint32_t from, spob::Commit c);
  enum Type {
    ConstructTree,
    AckTree,
    NakTree,
    RecoverPropose,
    AckRecover,
    RecoverCommit,
    Propose,
    Ack,
    Commit
  };
  Type type_;
  uint32_t from_;
  union {
    spob::ConstructTree ct_;
    spob::AckTree at_;
    spob::NakTree nt_;
    spob::RecoverPropose rp_;
    spob::AckRecover ar_;
    spob::RecoverCommit rc_;
    spob::Propose p_;
    spob::Ack a_;
    spob::Commit c_;
  };
  ~Message();
};
