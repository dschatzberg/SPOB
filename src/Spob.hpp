#pragma once

#include <stdint.h>

#include <list>
#include <map>
#include <string>

#include <boost/icl/interval_set.hpp>

#include "Messages.hpp"

namespace spob {
  class CommunicatorInterface {
  public:
    virtual void Send(const ConstructTree& ct, uint32_t to) = 0;
    virtual void Send(const AckTree& at, uint32_t to) = 0;
    virtual void Send(const NakTree& nt, uint32_t to) = 0;
    virtual void Send(const RecoverPropose& rp, uint32_t to) = 0;
    virtual void Send(const AckRecover& ar, uint32_t to) = 0;
    virtual void Send(const RecoverCommit& rc, uint32_t to) = 0;
    virtual void Send(const RecoverReconnect& rr, uint32_t to) = 0;
    virtual void Send(const Propose& p, uint32_t to) = 0;
    virtual void Send(const Ack& a, uint32_t to) = 0;
    virtual void Send(const Commit& c, uint32_t to) = 0;
    virtual void Send(const Reconnect& r, uint32_t to) = 0;
    virtual void Send(const ReconnectResponse& recon_resp, uint32_t to) = 0;

    virtual ~CommunicatorInterface() {}
  };

  class StateMachine {
  public:
    enum Status {
      kRecovering,
      kFollowing,
      kLeading
    };
    struct Callback {
      virtual void operator()(uint64_t id, const std::string& message) = 0;
      virtual void operator()(Status status, uint32_t primary) = 0;
      virtual ~Callback() {}
    };
    StateMachine(uint32_t rank, uint32_t size,
                 CommunicatorInterface& comm, Callback& cb);

    void Start();
    uint64_t Propose(const std::string& message);

    void Receive(const spob::ConstructTree& ct, uint32_t from);
    void Receive(const spob::AckTree& at, uint32_t from);
    void Receive(const spob::NakTree& at, uint32_t from);
    void Receive(const spob::RecoverPropose& rp, uint32_t from);
    void Receive(const spob::AckRecover& ar, uint32_t from);
    void Receive(const spob::RecoverCommit& rc, uint32_t from);
    void Receive(const spob::RecoverReconnect& rr, uint32_t from);
    void Receive(const spob::Propose& p, uint32_t from);
    void Receive(const spob::Ack& a, uint32_t from);
    void Receive(const spob::Commit& c, uint32_t from);
    void Receive(const spob::Reconnect& r, uint32_t from);
    void Receive(const spob::ReconnectResponse& recon_resp, uint32_t from);
    void Receive(const spob::Failure& failure);
  private:
    void Recover();
    void ConstructTree();
    void AckTree();
    void NakTree(const spob::NakTree& nt);
    void RecoverPropose();
    void AckRecover();
    void RecoverCommit();
    void Propose(const spob::Propose& p);
    void Ack(const spob::Ack& a);
    void Commit(const spob::Commit& c);

    uint32_t rank_;
    uint32_t size_;
    CommunicatorInterface& comm_;
    Callback& cb_;
    uint32_t primary_;
    uint64_t count_;
    uint64_t last_proposed_mid_;
    uint64_t last_acked_mid_;
    uint64_t last_committed_mid_;
    uint64_t current_mid_;
    bool constructing_;
    bool recovering_;
    bool got_propose_;
    bool acked_;
    unsigned int tree_acks_;
    std::list<uint32_t> ancestors_;
    std::map<uint32_t, std::pair<uint32_t, uint64_t> > children_;
    typedef std::map<uint64_t,
                     std::pair<std::string,
                               boost::icl::interval_set<uint32_t> > > LogType;
    LogType log_;
    boost::icl::interval_set<uint32_t> recover_ack_;
    boost::icl::interval_set<uint32_t> lower_correct_;
    boost::icl::interval_set<uint32_t> upper_correct_;
    boost::icl::interval_set<uint32_t> subtree_correct_;
  };
}
