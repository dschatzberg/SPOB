#include "Spob.hpp"

using namespace spob;

StateMachine::StateMachine(uint32_t rank, CommunicatorInterface& comm,
                           FailureDetectorInterface& fd)
  : rank_(rank), comm_(comm), fd_(fd)
{
  count_ = 0;
  last_proposed_zxid_ = 0;
  fd.AddCallback(FDCallbackStatic, reinterpret_cast<void*>(this));
}

void
StateMachine::Start()
{
  Recover();
}

void
StateMachine::Receive(const spob::ConstructTree& ct, uint32_t from)
{
  uint32_t new_primary = ct.ancestors(ct.ancestors_size() - 1);
  if (new_primary > primary_ ||
      (new_primary == primary_ && ct.count() > count_)) {
    ancestors_.assign(ct.ancestors().begin(), ct.ancestors().end());
    primary_ = ancestors_.back();
    count_ = ct.count();
    max_rank_ = ct.max_rank();
    ConstructTree(max_rank_);
  }
}

void
StateMachine::Receive(const spob::AckTree& at, uint32_t from)
{
  if (at.primary() == primary_ && at.count() == count_) {
    tree_acks_++;
    if (tree_acks_ == children_.size()) {
      AckTree();
    }
  }
}

void
StateMachine::Receive(const spob::RecoverPropose& rp, uint32_t from)
{

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
  if (primary_ == rank_) {
    count_ = 0;
    max_rank_ = comm_.size() - 1;
    ancestors_.clear();
    ConstructTree(max_rank_);
  }
}

void
StateMachine::ConstructTree(uint32_t max_rank)
{
  tree_acks_ = 0;
  children_.clear();
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
  ct_.clear_ancestors();
  ct_.add_ancestors(rank_);
  for (std::list<uint32_t>::const_iterator it = ancestors_.begin();
       it != ancestors_.end(); ++it) {
    ct_.add_ancestors(*it);
  }
  ct_.set_count(count_);
  uint32_t left_child_max_rank = std::max(left_child, right_child - 1);
  ct_.set_max_rank(left_child_max_rank);
  comm_.Send(ct_, left_child);
  children_[left_child] = left_child_max_rank;

  if (left_child == right_child) {
    //we only need one child
    return;
  }
  //tell right child to construct
  ct_.set_max_rank(max_rank);
  comm_.Send(ct_, right_child);
  children_[right_child] = max_rank;
}

void
StateMachine::AckTree()
{
  if (rank_ == primary_) {
    // Tree construction succeeded, send outstanding proposals down
    // the tree
    rp_.set_primary(primary_);
    for(std::map<uint64_t, std::string>::const_iterator it = log_.begin();
        it != log_.end(); ++it) {
      spob::Entry* ep = rp_.add_propose();
      ep->set_zxid(it->first);
      ep->set_message(it->second);
    }
    for(std::map<uint32_t, uint32_t>::const_iterator it = children_.begin();
        it != children_.end(); ++it) {
      comm_.Send(rp_, it->first);
    }
  } else {
    at_.set_primary(primary_);
    at_.set_count(count_);
    comm_.Send(at_, parent_);
  }
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
