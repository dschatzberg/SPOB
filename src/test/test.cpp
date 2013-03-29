#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>

#include "Spob.hpp"

namespace mpi = boost::mpi;

class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(spob::StateMachine** sm) : sm_(sm)
  {

    mpi::communicator world;
    size_ = world.size();
    req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
    rank_ = world.rank();
  }

  enum {
    ConstructTree,
    AckTree
  };

  void Send(const spob::ConstructTree& ct, uint32_t to)
  {
    std::cout << rank_ << ": Sending ConstructTree to " << to <<
      ": " << ct.ShortDebugString() << std::endl;

    std::string str;
    if (!ct.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a ConstructTree");
    }

    Send(to, ConstructTree, str);
  }

  void Send(const spob::AckTree& at, uint32_t to)
  {
    std::cout << rank_ << ": Sending AckTree to " << to <<
      ": " << at.ShortDebugString() << std::endl;

    std::string str;
    if (!at.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a AckTree");
    }

    Send(to, AckTree, str);
  }

  void Send(const spob::RecoverPropose& rp, uint32_t to)
  {
  }

  void Process()
  {
    boost::optional<mpi::status> opt_status = req_.test();
    if (opt_status) {
      spob::ConstructTree ct;
      spob::AckTree at;
      switch (opt_status->tag()) {
      case ConstructTree:
        if (!ct.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse ConstructTree");
        }
        std::cout << rank_ << ": Received ConstructTree from " <<
          opt_status->source() << ": " << ct.ShortDebugString() << std::endl;
        (*sm_)->Receive(ct, opt_status->source());
        break;
      case AckTree:
        if (!at.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse ConstructTree");
        }
        std::cout << rank_ << ": Received AckTree from " <<
          opt_status->source() << ": " << at.ShortDebugString() << std::endl;
        (*sm_)->Receive(at, opt_status->source());
        break;
      }
      mpi::communicator world;
      req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
    }
  }

  uint32_t size() const {return size_;}
  ~Communicator()
  {
    req_.cancel();
  }
private:
  void Send(uint32_t to, int tag, std::string str)
  {
    mpi::communicator world;
    world.send(to, tag, str);
  }
  spob::StateMachine** sm_;
  uint32_t size_;
  mpi::request req_;
  std::string message_;
  uint32_t rank_;
};

class FailureDetector : public spob::FailureDetectorInterface {
public:
  FailureDetector() {}
  void AddCallback(Callback cb, void* data) {}
  const boost::icl::interval_set<uint32_t>& set() const {return set_;}
private:
  boost::icl::interval_set<uint32_t> set_;
};

int main(int argc, char* argv[])
{
  mpi::environment env(argc, argv);
  spob::StateMachine* sm;
  Communicator comm(&sm);
  FailureDetector fd;
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), comm, fd);
  sm->Start();
  while (1) {
    comm.Process();
  }
  delete sm;
  return 0;
}
