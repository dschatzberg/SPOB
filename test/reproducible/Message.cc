#include "Message.h"

Message::Message() : type_(ConstructTree), ct_()
{
}

Message::Message(uint32_t from, spob::ConstructTree ct) :
  type_(ConstructTree), from_(from), ct_(ct)
{
}
Message::Message(uint32_t from, spob::AckTree at) :
  type_(AckTree), from_(from), at_(at)
{
}
Message::Message(uint32_t from, spob::NakTree nt) :
  type_(NakTree), from_(from), nt_(nt)
{
}
Message::Message(uint32_t from, spob::RecoverPropose rp) :
  type_(RecoverPropose), from_(from), rp_(rp)
{
}
Message::Message(uint32_t from, spob::AckRecover ar) :
  type_(AckRecover), from_(from), ar_(ar)
{
}
Message::Message(uint32_t from, spob::RecoverReconnect rr) :
  type_(RecoverReconnect), from_(from), rr_(rr)
{
}
Message::Message(uint32_t from, spob::RecoverCommit rc) :
  type_(RecoverCommit), from_(from), rc_(rc)
{
}
Message::Message(uint32_t from, spob::Propose p) :
  type_(Propose), from_(from), p_(p)
{
}
Message::Message(uint32_t from, spob::Ack a) :
  type_(Ack), from_(from), a_(a)
{
}
Message::Message(uint32_t from, spob::Commit c) :
  type_(Commit), from_(from), c_(c)
{
}
Message::Message(uint32_t from, spob::Reconnect r) :
  type_(Reconnect), from_(from), r_(r)
{
}
Message::Message(uint32_t from, spob::ReconnectResponse recon_resp) :
  type_(ReconnectResponse), from_(from), recon_resp_(recon_resp)
{
}

Message::~Message()
{
  switch (type_) {
  case ConstructTree:
    ct_.~ConstructTree();
    break;
  case AckTree:
    at_.~AckTree();
    break;
  case NakTree:
    nt_.~NakTree();
    break;
  case RecoverPropose:
    rp_.~RecoverPropose();
    break;
  case AckRecover:
    ar_.~AckRecover();
    break;
  case RecoverReconnect:
    rr_.~RecoverReconnect();
    break;
  case RecoverCommit:
    rc_.~RecoverCommit();
    break;
  case Propose:
    p_.~Propose();
    break;
  case Ack:
    a_.~Ack();
    break;
  case Commit:
    c_.~Commit();
    break;
  case Reconnect:
    r_.~Reconnect();
    break;
  case ReconnectResponse:
    recon_resp_.~ReconnectResponse();
    break;
  }
}
