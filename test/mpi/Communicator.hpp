#pragma once

#include <queue>

#include <boost/mpi.hpp>
#include <boost/variant.hpp>

#include "Spob.hpp"

class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(spob::StateMachine** sm, bool verbose);

  void Send(const spob::ConstructTree& ct, uint32_t to);
  void Send(const spob::AckTree& at, uint32_t to);
  void Send(const spob::NakTree& nt, uint32_t to);
  void Send(const spob::RecoverPropose& rp, uint32_t to);
  void Send(const spob::AckRecover& ar, uint32_t to);
  void Send(const spob::RecoverCommit& rc, uint32_t to);
  void Send(const spob::RecoverInform& ri, uint32_t to);
  void Send(const spob::RecoverReconnect& rr, uint32_t to);
  void Send(const spob::Propose& p, uint32_t to);
  void Send(const spob::Ack& a, uint32_t to);
  void Send(const spob::Commit& c, uint32_t to);
  void Send(const spob::Inform& i, uint32_t to);
  void Send(const spob::Reconnect& r, uint32_t to);
  void Send(const spob::ReconnectResponse& recon_resp, uint32_t to);
  void Process();
  ~Communicator();
private:
  template <typename T>
  void DoSend(const T& t, uint32_t to);

  spob::StateMachine** sm_;
  class ReceiveVisitor : public boost::static_visitor<> {
  public:
    ReceiveVisitor(Communicator& comm_);
    template <typename T>
    void operator()(T& t) const;
    boost::optional<boost::mpi::status> opt_status_;
  private:
    Communicator& comm_;
  };
  ReceiveVisitor rv_;
  boost::mpi::request req_;
  typedef boost::variant<
    spob::ConstructTree,
    spob::AckTree,
    spob::NakTree,
    spob::RecoverPropose,
    spob::AckRecover,
    spob::RecoverCommit,
    spob::RecoverInform,
    spob::RecoverReconnect,
    spob::Propose,
    spob::Ack,
    spob::Commit,
    spob::Inform,
    spob::Reconnect,
    spob::ReconnectResponse> Message;
  Message message_;
  uint32_t rank_;
  bool verbose_;
  std::queue<std::pair<boost::mpi::request, Message> > pending_;
};
