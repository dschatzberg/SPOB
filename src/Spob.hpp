#pragma once

#include <stdint.h>

#include <string>

#include <boost/icl/interval_set.hpp>

namespace Spob {
  class CommunicatorInterface {
  public:
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
    uint32_t tree_count_;
  };

}
