#pragma once

#include <stdint.h>

#include <list>
#include <string>

#include <boost/icl/interval_set.hpp>

#include "spob/Ack.pb.h"
#include "spob/AckRecover.pb.h"
#include "spob/AckTree.pb.h"
#include "spob/Commit.pb.h"
#include "spob/ConstructTree.pb.h"
#include "spob/NakTree.pb.h"
#include "spob/Propose.pb.h"
#include "spob/RecoverCommit.pb.h"
#include "spob/RecoverPropose.pb.h"

namespace spob {
  class CommunicatorInterface {
  public:
    virtual void Send(const ConstructTree& ct, uint32_t to) = 0;
    virtual void Send(const AckTree& at, uint32_t to) = 0;
    virtual void Send(const NakTree& nt, uint32_t to) = 0;
    virtual void Send(const RecoverPropose& rp, uint32_t to) = 0;
    virtual void Send(const AckRecover& ar, uint32_t to) = 0;
    virtual void Send(const RecoverCommit& rc, uint32_t to) = 0;
    virtual void Send(const Propose& p, uint32_t to) = 0;
    virtual void Send(const Ack& a, uint32_t to) = 0;
    virtual void Send(const Commit& c, uint32_t to) = 0;
    virtual uint32_t size() const = 0;

    virtual ~CommunicatorInterface() {}
  };

  class FailureDetectorInterface {
  public:
    typedef void (*Callback)(void*, uint32_t);
    virtual void AddCallback(Callback cb, void* data) = 0;
    virtual const boost::icl::interval_set<uint32_t>& set() const = 0;

    virtual ~FailureDetectorInterface() {}
  };

  class StateMachine {
  public:
    enum Status {
      RECOVERING,
      FOLLOWING,
      LEADING
    };
    typedef void (*DeliverFunc)(void*, uint64_t id, const std::string& message);
    typedef void (*StatusFunc)(void*, Status status, uint32_t primary);
    StateMachine(uint32_t rank, CommunicatorInterface& comm,
                 FailureDetectorInterface& fd, DeliverFunc df, StatusFunc sf,
                 void* cb_data);
    void Start();
    uint64_t Propose(const std::string& message);

    void Receive(const spob::ConstructTree& ct, uint32_t from);
    void Receive(const spob::AckTree& at, uint32_t from);
    void Receive(const spob::NakTree& at, uint32_t from);
    void Receive(const spob::RecoverPropose& rp, uint32_t from);
    void Receive(const spob::AckRecover& ar, uint32_t from);
    void Receive(const spob::RecoverCommit& rc, uint32_t from);
    void Receive(const spob::Propose& p, uint32_t from);
    void Receive(const spob::Ack& a, uint32_t from);
    void Receive(const spob::Commit& c, uint32_t from);
  private:
    void Recover();
    void ConstructTree(uint32_t max_rank);
    void AckTree();
    void RecoverPropose();
    void AckRecover();
    void RecoverCommit();
    void Ack(uint64_t id);
    void FDCallback(uint32_t rank);

    static void FDCallbackStatic(void* data, uint32_t rank);

    uint32_t rank_;
    CommunicatorInterface& comm_;
    FailureDetectorInterface& fd_;
    DeliverFunc df_;
    StatusFunc sf_;
    void* cb_data_;
    uint32_t primary_;
    uint32_t parent_;
    uint64_t count_;
    uint32_t max_rank_;
    uint64_t last_proposed_mid_;
    uint64_t current_mid_;
    bool constructing_;
    bool recovering_;
    unsigned int tree_acks_;
    std::list<uint32_t> ancestors_;
    std::map<uint32_t, std::pair<uint32_t, uint64_t> > children_;
    typedef std::map<uint64_t,
                     std::pair<std::string,
                               boost::icl::interval_set<uint32_t> > > LogType;
    LogType log_;
    boost::icl::interval_set<uint32_t> recover_ack_;
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

}
