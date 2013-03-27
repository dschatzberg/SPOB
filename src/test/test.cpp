#include "Spob.hpp"

class Communicator : public Spob::CommunicatorInterface {
public:
  Communicator(uint32_t size) : size_(size) {}
  uint32_t size() const {return size_;}
private:
  uint32_t size_;
};

class FailureDetector : public Spob::FailureDetectorInterface {
public:
  FailureDetector() {}
  void AddCallback(Callback cb, void* data) {}
  const boost::icl::interval_set<uint32_t>& set() const {return set_;}
private:
  boost::icl::interval_set<uint32_t> set_;
};

int main()
{
  Communicator comm(3);
  FailureDetector fd;
  Spob::StateMachine sm(0, comm, fd);
  sm.Start();
  return 0;
}
