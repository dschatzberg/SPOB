#pragma once

#include <stdint.h>

#include <list>
#include <string>

#include <boost/icl/interval_set.hpp>

#include "spob/AckTree.pb.h"
#include "spob/ConstructTree.pb.h"
#include "spob/RecoverPropose.pb.h"

namespace spob {
  class CommunicatorInterface {
  public:
    virtual void Send(const ConstructTree& ct, uint32_t to) = 0;
    virtual void Send(const AckTree& at, uint32_t to) = 0;
    virtual void Send(const RecoverPropose& rp, uint32_t to) = 0;
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
    StateMachine(uint32_t rank, CommunicatorInterface& comm,
                 FailureDetectorInterface& fd);
    void Start();
    //uint64_t Propose(const std::string& message);

    void Receive(const spob::ConstructTree& ct, uint32_t from);
    void Receive(const spob::AckTree& at, uint32_t from);
    void Receive(const spob::RecoverPropose& rp, uint32_t from);
  private:
    void Recover();
    void ConstructTree(uint32_t max_rank);
    void AckTree();
    void FDCallback(uint32_t rank);

    static void FDCallbackStatic(void* data, uint32_t rank);

    uint32_t rank_;
    CommunicatorInterface& comm_;
    FailureDetectorInterface& fd_;
    uint32_t primary_;
    uint32_t parent_;
    uint64_t count_;
    uint32_t max_rank_;
    uint64_t last_proposed_zxid_;
    unsigned int tree_acks_;
    std::list<uint32_t> ancestors_;
    std::map<uint32_t, uint32_t> children_;
    std::map<uint64_t, std::string> log_;
    spob::ConstructTree ct_;
    spob::AckTree at_;
    spob::RecoverPropose rp_;
  };

}
