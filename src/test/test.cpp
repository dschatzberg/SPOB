#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>

#include "Spob.hpp"

namespace mpi = boost::mpi;

namespace {
  bool quit = false;
}

class Callback {
public:
  Callback(spob::StateMachine** sm, long int num_messages)
    : sm_(sm), n_(num_messages)
  {
    mpi::communicator world;
    rank_ = world.rank();
    primary_ = 0;
    count_ = 0;
  }
  static void Status(void* data, spob::StateMachine::Status status,
                            uint32_t primary)
  {
    Callback* cb = reinterpret_cast<Callback*>(data);
    cb->Status(status, primary);
  }
  static void Deliver(void* data, uint64_t id, const std::string& message)
  {
    Callback* cb = reinterpret_cast<Callback*>(data);
    cb->Deliver(id, message);
  }
private:
  void Status(spob::StateMachine::Status status, uint32_t primary)
  {
    if (status == spob::StateMachine::LEADING) {
#ifdef LOG
      std::cout << rank_ << ": Recovered and Leading" << std::endl;
#endif
      primary_ = primary;
      (*sm_)->Propose("test");
    } else if (spob::StateMachine::FOLLOWING) {
      primary_ = primary;
#ifdef LOG
      std::cout << rank_ << ": Recovered and Following " << primary << std::endl;
#endif
    }
  }
  void Deliver(uint64_t id, const std::string& message)
  {
    count_++;
#ifdef LOG
    std::cout << rank_ << ": Delivered message: 0x" << std::hex << id <<
      std::dec << ", \"" << message << "\"" << std::endl;
#endif
    if (count_ == n_) {
      quit = true;
    } else if (rank_ == primary_) {
      (*sm_)->Propose("test");
    }
  }
  uint32_t primary_;
  uint32_t rank_;
  long int count_;
  long int n_;
  spob::StateMachine** sm_;
};

class Communicator : public spob::CommunicatorInterface {
public:
  Communicator(spob::StateMachine** sm) : sm_(sm)
  {

    mpi::communicator world;
    size_ = world.size();
    req_ = world.irecv(mpi::any_source, mpi::any_tag, message_);
    rank_ = world.rank();
  }

  enum MessageType {
    ConstructTree,
    AckTree,
    RecoverPropose,
    AckRecover,
    RecoverCommit,
    Propose,
    Ack,
    Commit
  };

  void Send(const spob::ConstructTree& ct, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending ConstructTree to " << to <<
      ": " << ct.ShortDebugString() << std::endl;
#endif
    std::string str;
    if (!ct.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a ConstructTree");
    }

    Send(to, ConstructTree, str);
  }

  void Send(const spob::AckTree& at, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending AckTree to " << to <<
      ": " << at.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!at.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a AckTree");
    }

    Send(to, AckTree, str);
  }

  void Send(const spob::RecoverPropose& rp, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending RecoverPropose to " << to <<
      ": " << rp.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!rp.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a RecoverPropose");
    }

    Send(to, RecoverPropose, str);
  }

  void Send(const spob::AckRecover& ar, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending AckRecover to " << to <<
      ": " << ar.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!ar.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a AckRecover");
    }

    Send(to, AckRecover, str);
  }

  void Send(const spob::RecoverCommit& rc, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending RecoverCommit to " << to <<
      ": " << rc.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!rc.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a RecoverCommit");
    }

    Send(to, RecoverCommit, str);
  }

  void Send(const spob::Propose& p, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending Propose to " << to <<
      ": " << p.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!p.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a Propose");
    }

    Send(to, Propose, str);
  }

  void Send(const spob::Ack& a, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending Ack to " << to <<
      ": " << a.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!a.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a Ack");
    }

    Send(to, Ack, str);
  }

  void Send(const spob::Commit& c, uint32_t to)
  {
#ifdef LOG
    std::cout << rank_ << ": Sending Commit to " << to <<
      ": " << c.ShortDebugString() << std::endl;
#endif

    std::string str;
    if (!c.SerializeToString(&str)) {
      throw std::runtime_error("Failed to serialized a Commit");
    }

    Send(to, Commit, str);
  }

  void Process()
  {
    boost::optional<mpi::status> opt_status = req_.test();
    if (opt_status) {
      spob::ConstructTree ct;
      spob::AckTree at;
      spob::RecoverPropose rp;
      spob::AckRecover ar;
      spob::RecoverCommit rc;
      spob::Propose p;
      spob::Ack a;
      spob::Commit c;
      switch (static_cast<MessageType>(opt_status->tag())) {
      case ConstructTree:
        if (!ct.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse ConstructTree");
        }
#ifdef LOG
        std::cout << rank_ << ": Received ConstructTree from " <<
          opt_status->source() << ": " << ct.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(ct, opt_status->source());
        break;
      case AckTree:
        if (!at.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse AckTree");
        }
#ifdef LOG
        std::cout << rank_ << ": Received AckTree from " <<
          opt_status->source() << ": " << at.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(at, opt_status->source());
        break;
      case RecoverPropose:
        if (!rp.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse RecoverPropose");
        }
#ifdef LOG
        std::cout << rank_ << ": Received RecoverPropose from " <<
          opt_status->source() << ": " << rp.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(rp, opt_status->source());
        break;
      case AckRecover:
        if (!ar.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse AckRecover");
        }
#ifdef LOG
        std::cout << rank_ << ": Received AckRecover from " <<
          opt_status->source() << ": " << ar.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(ar, opt_status->source());
        break;
      case RecoverCommit:
        if (!rc.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse RecoverCommit");
        }
#ifdef LOG
        std::cout << rank_ << ": Received RecoverCommit from " <<
          opt_status->source() << ": " << rc.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(rc, opt_status->source());
        break;
      case Propose:
        if (!p.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse Propose");
        }
#ifdef LOG
        std::cout << rank_ << ": Received Propose from " <<
          opt_status->source() << ": " << p.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(p, opt_status->source());
        break;
      case Ack:
        if (!a.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse Ack");
        }
#ifdef LOG
        std::cout << rank_ << ": Received Ack from " <<
          opt_status->source() << ": " << a.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(a, opt_status->source());
        break;
      case Commit:
        if (!c.ParseFromString(message_)) {
          throw std::runtime_error("Failed to parse Commit");
        }
#ifdef LOG
        std::cout << rank_ << ": Received Commit from " <<
          opt_status->source() << ": " << c.ShortDebugString() << std::endl;
#endif
        (*sm_)->Receive(c, opt_status->source());
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
  void Send(uint32_t to, MessageType tag, std::string str)
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " num_messages" << std::endl;
    return -1;
  }
  long int num_messages = strtol(argv[1], NULL, 10);
  if (num_messages <= 0 || num_messages == LONG_MAX ||
      num_messages == LONG_MIN) {
    std::cerr << "Invalid number of messages" << std::endl;
    return -1;
  }
  Callback cb(&sm, num_messages);
  Communicator comm(&sm);
  FailureDetector fd;
  mpi::communicator world;
  sm = new spob::StateMachine(world.rank(), comm, fd, Callback::Deliver,
                              Callback::Status, reinterpret_cast<void*>(&cb));
  sm->Start();
  while (!quit) {
    comm.Process();
  }
  delete sm;
  return 0;
}
