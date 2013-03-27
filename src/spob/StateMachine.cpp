#include "Spob.hpp"

using namespace Spob;

StateMachine::StateMachine(uint32_t rank, CommunicatorInterface& comm,
                           FailureDetectorInterface& fd)
  : rank_(rank), comm_(comm), fd_(fd)
{
  fd.AddCallback(FDCallbackStatic, reinterpret_cast<void*>(this));
}

void
StateMachine::Start()
{
  Recover();
}

void
StateMachine::Recover()
{
  // If the set of failed processes contains the set of processes
  // with lower rank than me, then I am the lowest ranked correct process
  using namespace boost::icl;
  interval_set<uint32_t> lower(interval<uint32_t>::closed(0, rank_));
  lower -= fd_.set();
  primary_ = first(lower);
  tree_count_ = 0;
  if (primary_ == rank_) {
    ConstructTree(comm_.size()-1);
  }
}

void
StateMachine::ConstructTree(uint32_t max_rank)
{
  if (max_rank <= rank_) {
    //no tree needs to be constructed
    AckTree();
    return;
  }
  using namespace boost::icl;
  interval_set<uint32_t> range(interval<uint32_t>::closed(rank_ + 1,
                                                          max_rank));
  range -= fd_.set();

  if (cardinality(range) == 0) {
    //no tree needs to be constructed
    AckTree();
    return;
  }
  //left child is next lowest rank above ours
  uint32_t left_child = lower(range);

  //right child is the median process in our subtree
  uint32_t right_child;
  uint32_t pos = cardinality(range) / 2;
  for (interval_set<uint32_t>::const_iterator it = range.begin();
       it != range.end(); ++it) {
    if (cardinality(*it) > pos) {
      right_child = first(*it) + pos;
      break;
    }
    pos -= cardinality(*it);
  }

  //tell left child to construct
  //send(CONSTRUCT_TREE(primary, count, std::max(left_child,
  //right_child - 1)))
  if (left_child == right_child) {
    //we only need one child
    return;
  }
  //send(CONSTRUCT_TREE(primary, count, max_rank))
}

void
StateMachine::AckTree()
{
}

void
StateMachine::FDCallback(uint32_t rank)
{
}

void
StateMachine::FDCallbackStatic(void* data, uint32_t rank)
{
  StateMachine* t = reinterpret_cast<StateMachine*>(data);
  t->FDCallback(rank);
}
