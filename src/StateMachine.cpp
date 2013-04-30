#include "Spob.hpp"

using namespace spob;
namespace icl = boost::icl;

StateMachine::StateMachine(uint32_t rank, uint32_t size, uint32_t replicas,
                           CommunicatorInterface& comm,
                           Callback& cb)
  : rank_(rank), size_(size), replicas_(replicas), comm_(comm), cb_(cb)
{
  count_ = 0;
  last_proposed_mid_ = 0;
  last_acked_mid_ = 0;
  last_committed_mid_ = 0;
  lower_correct_ += icl::interval<uint32_t>::closed(0, rank);
  upper_correct_ += icl::interval<uint32_t>::closed(rank + 1, replicas - 1);
  listener_upper_correct_ +=
    icl::interval<uint32_t>::closed(std::max(rank + 1, replicas), size - 1);
}

void
StateMachine::Start()
{
  Recover();
}

uint64_t
StateMachine::Propose(const std::string& message)
{
  spob::Propose p;
  p.primary_ = primary_;
  uint64_t id = current_mid_;
  p.proposal_ = std::make_pair(id, message);
  Propose(p);
  current_mid_++;
  // Make sure we don't wrap around with the same primary
  assert(current_mid_ >> 32 == id >> 32);
  return id;
}

void
StateMachine::Propose(const spob::Propose& p)
{
  for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it =
         children_.begin(); it != children_.end(); ++it) {
    comm_.Send(p, it->first);
  }
  uint64_t id = p.proposal_.first;
  log_[id] = std::make_pair(p.proposal_.second, subtree_correct_);
  last_proposed_mid_ = id;
}

void
StateMachine::Receive(const spob::ConstructTree& ct, uint32_t from)
{
  // only act if this message has a newer primary than we are aware of
  // or same primary, but higher count and the primary in question has
  // not failed
  uint32_t new_primary = ct.ancestors_.back();
  if ((new_primary > primary_ ||
       (new_primary == primary_ && ct.count_ >= count_)) &&
      icl::contains(lower_correct_, new_primary)) {
    ancestors_ = ct.ancestors_;
    primary_ = new_primary;
    count_ = ct.count_;
    if (rank_ < replicas_) {
      subtree_correct_ = upper_correct_ -
        icl::interval<uint32_t>::closed(ct.max_rank_ + 1, replicas_ - 1);
    } else {
      listener_subtree_correct_ = listener_upper_correct_ -
        icl::interval<uint32_t>::closed(ct.max_rank_ + 1, size_ - 1);
    }
    ConstructTree();
  }
}

void
StateMachine::Receive(const spob::AckTree& at, uint32_t from)
{
  if (at.primary_ == primary_ && at.count_ == count_ &&
      (children_.count(from) || listener_children_.count(from))) {
    tree_acks_++;
    if (from < replicas_) {
      children_[from].second = at.last_mid_;
    } else {
      listener_children_[from] = at.last_mid_;
    }
    if (tree_acks_ == (children_.size() + listener_children_.size())) {
      AckTree();
    }
  }
}

void
StateMachine::Receive(const spob::NakTree& nt, uint32_t from)
{
  if (nt.primary_ == primary_ && nt.count_ == count_) {
    assert(children_.count(from) == 1);
    NakTree(nt);
  }
}

void
StateMachine::NakTree(const spob::NakTree& nt)
{
  count_++;
  if (primary_ == rank_) {
    ConstructTree();
  } else {
    comm_.Send(nt, ancestors_.front());
    children_.clear();
    ancestors_.clear();
  }
}

void
StateMachine::Receive(const spob::RecoverPropose& rp, uint32_t from)
{
  if (rp.primary_ == primary_ && from == ancestors_.front()) {
    got_propose_ = true;
    switch (rp.type_) {
    case RecoverPropose::kDiff:
      if (rp.proposals_.size() > 0) {
        for (std::list<std::pair<uint64_t, std::string> >::const_iterator it =
               rp.proposals_.begin(); it != rp.proposals_.end(); ++it) {
          log_.insert(log_.end(),
                      std::make_pair(it->first,
                                     std::make_pair(it->second,
                                                    subtree_correct_)));
        }
        last_proposed_mid_ = rp.proposals_.rbegin()->first;
      }
      break;
    case RecoverPropose::kTrunc:
      log_.erase(log_.upper_bound(rp.last_mid_), log_.end());
      last_proposed_mid_ = rp.last_mid_;
      break;
    }
    if (subtree_correct_.empty()) {
      AckRecover();
    } else {
      RecoverPropose();
    }
  }
}

void
StateMachine::Receive(const spob::AckRecover& ar, uint32_t from)
{
  if (ar.primary_ == primary_ && children_.count(from)) {
    recover_ack_ -= icl::interval<uint32_t>::closed(from, children_[from].first);
    if (recover_ack_.empty()) {
      AckRecover();
    }
  }
}

void
StateMachine::Receive(const spob::RecoverReconnect& rr, uint32_t from)
{
  if (rr.primary_ == primary_) {
    if (!rr.got_propose_) {
      //The child never received the propose
      spob::RecoverPropose rp;
      rp.primary_ = primary_;
      if (rr.last_proposed_ <= last_proposed_mid_) {
        rp.type_ = RecoverPropose::kDiff;
        for (LogType::const_iterator it = log_.upper_bound(rr.last_proposed_);
             it != log_.end(); ++it) {
          rp.proposals_.push_back(std::make_pair(it->first, it->second.first));
        }
      } else {
        rp.type_ = RecoverPropose::kTrunc;
        rp.last_mid_ = last_proposed_mid_;
      }
      comm_.Send(rp, from);
    } else if (rr.acked_ && !acked_) {
      //The child acknowledged the recovery already and we haven't acked
      recover_ack_ -= icl::interval<uint32_t>::closed(from, rr.max_rank_);
      if (recover_ack_.empty()) {
        AckRecover();
      }
    }
    children_[from] = std::make_pair(rr.max_rank_, rr.last_proposed_);
  }
}

void
StateMachine::Receive(const spob::RecoverCommit& rc, uint32_t from)
{
  if (rc.primary_ == primary_ && from == ancestors_.front()) {
    RecoverCommit();
    cb_(kFollowing, primary_);
  }
}

void
StateMachine::Receive(const spob::RecoverInform& ri, uint32_t from)
{
  if (ri.primary_ == primary_ && from == ancestors_.front()) {
    RecoverInform();
    cb_(kFollowing, primary_);
  }
}

void
StateMachine::Receive(const spob::Propose& p, uint32_t from)
{
  if (p.primary_ == primary_ && from == ancestors_.front()) {
    Propose(p);
    if (subtree_correct_.empty()) {
      spob::Ack a;
      a.primary_ = primary_;
      a.mid_ = p.proposal_.first;
      Ack(a);
    }
  }
}

void
StateMachine::Ack(const spob::Ack& a)
{
  comm_.Send(a, ancestors_.front());
  last_acked_mid_ = a.mid_;
}

void
StateMachine::Receive(const spob::Ack& a, uint32_t from)
{
  if (a.primary_ == primary_ && children_.count(from) && log_.count(a.mid_)) {
    uint64_t id = a.mid_;
    log_[id].second -=
      icl::interval<uint32_t>::closed(from, children_[from].first);
    if (log_[id].second.empty()) {
      if (rank_ == primary_) {
        spob::Commit c;
        c.primary_ = primary_;
        c.mid_ = id;
        Commit(c);
        spob::Inform i;
        i.primary_ = primary_;
        i.t_.first = id;
        i.t_.second = log_[id].first;
        Inform(i);
      } else {
        Ack(a);
      }
    }
  }
}

void
StateMachine::Commit(const spob::Commit& c)
{
  for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it =
         children_.begin(); it != children_.end(); ++it) {
    comm_.Send(c, it->first);
  }
  uint64_t id = c.mid_;
  cb_(id, log_[id].first);
  log_.erase(log_.begin());
  last_committed_mid_ = id;
  last_acked_mid_ = std::max(last_acked_mid_, last_committed_mid_);
}

void
StateMachine::Inform(const spob::Inform& i)
{
  for (std::map<uint32_t, uint64_t>::const_iterator it =
         listener_children_.begin(); it != listener_children_.end(); ++it) {
    comm_.Send(i, it->first);
  }
  if (rank_ != primary_) {
    cb_(i.t_.first, i.t_.second);
    last_committed_mid_ = i.t_.first;
  }
}

void
StateMachine::Receive(const spob::Commit& c, uint32_t from)
{
  if (c.primary_ == primary_ && from == ancestors_.front()) {
    Commit(c);
  }
}

void
StateMachine::Receive(const spob::Inform& i, uint32_t from)
{
  if (i.primary_ == primary_ && from == ancestors_.front()) {
    Inform(i);
  }
}

void
StateMachine::Receive(const spob::Reconnect& r, uint32_t from)
{
  if (r.primary_ == primary_ && icl::contains(subtree_correct_, from)) {
    ReconnectResponse rr;
    rr.primary_ = primary_;
    rr.last_committed_ = last_committed_mid_;
    for (LogType::const_iterator it = log_.upper_bound(r.last_proposed_);
         it != log_.end(); ++it) {
      rr.proposals_.push_back(std::make_pair(it->first, it->second.first));
    }
    comm_.Send(rr, from);
    children_[from] = std::make_pair(r.max_rank_, r.last_proposed_);
    // We look in the reverse order of all the outstanding
    // proposals. If we see one that we are allowed to acknowledge
    // then all preceding proposals can be acknowledged
    for (LogType::reverse_iterator it =
           static_cast<LogType::reverse_iterator>(log_.upper_bound(r.last_acked_));
         it != log_.rend(); ++it) {
      it->second.second -= icl::interval<uint32_t>::closed(from, r.max_rank_);
      if (it->second.second.empty()) {
        LogType::iterator cmp = it.base();
        for (LogType::iterator it2 = log_.begin();
             it2 != cmp; ++it2) {
          if (rank_ == primary_) {
            spob::Commit c;
            c.primary_ = primary_;
            c.mid_ = it2->first;
            Commit(c);
          } else {
            spob::Ack a;
            a.primary_ = primary_;
            a.mid_ = it2->first;
            Ack(a);
          }
        }
        break;
      }
    }
  }
}

void
StateMachine::Receive(const spob::ReconnectResponse& recon_resp, uint32_t from)
{
  if (recon_resp.primary_ == primary_ && from == ancestors_.front()) {
    for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
           = children_.begin();
         it != children_.end(); ++it) {
      comm_.Send(recon_resp, it->first);
    }
    for (LogType::const_iterator it = log_.begin();
         it != log_.upper_bound(recon_resp.last_committed_); ++it) {
      cb_(it->first, it->second.first);
    }
    log_.erase(log_.begin(), log_.upper_bound(recon_resp.last_committed_));
    last_committed_mid_ = recon_resp.last_committed_;
    if (!recon_resp.proposals_.empty()) {
      for (std::list<std::pair<uint64_t, std::string> >::const_iterator it =
             recon_resp.proposals_.begin();
           it != recon_resp.proposals_.end(); ++it) {
        log_.insert(log_.end(),
                    std::make_pair(it->first,
                                   std::make_pair(it->second,
                                                  subtree_correct_)));
        if (subtree_correct_.empty()) {
          spob::Ack a;
          a.primary_ = primary_;
          a.mid_ = it->first;
          Ack(a);
        }
      }
      last_proposed_mid_ = recon_resp.proposals_.rbegin()->first;
    }
  }
}

void
StateMachine::Recover()
{
  children_.clear();
  listener_children_.clear();
  ancestors_.clear();
  count_ = 0;
  // If the set of failed processes contains the set of processes
  // with lower rank than me, then I am the lowest ranked correct
  // process
  primary_ = icl::first(lower_correct_);
  if (primary_ == rank_) {
    subtree_correct_ = upper_correct_;
    listener_subtree_correct_ = listener_upper_correct_;
    ConstructTree();
  }
  cb_(kRecovering, 0);
}

void
StateMachine::ConstructTree()
{
  constructing_ = true;
  got_propose_ = false;
  acked_ = false;
  tree_acks_ = 0;
  children_.clear();
  listener_children_.clear();
  if ((rank_ < replicas_ && subtree_correct_.empty()) ||
      (rank_ >= replicas_ && listener_subtree_correct_.empty())) {
    AckTree();
  } else {

    if (rank_ < replicas_ && !subtree_correct_.empty()) {
      //left child is next lowest rank above ours
      uint32_t left_child = icl::first(subtree_correct_);

      //right child is the median process in our subtree
      uint32_t right_child = 0;
      uint32_t pos = icl::cardinality(subtree_correct_) / 2;
      for (icl::interval_set<uint32_t>::const_iterator it =
             subtree_correct_.begin();
           it != subtree_correct_.end(); ++it) {
        if (icl::cardinality(*it) > pos) {
          right_child = icl::first(*it) + pos;
          break;
        }
        pos -= icl::cardinality(*it);
      }
      assert(right_child > 0);

      spob::ConstructTree ct;
      ct.ancestors_.push_back(rank_);
      ct.ancestors_.insert(ct.ancestors_.end(), ancestors_.begin(),
                           ancestors_.end());
      ct.count_ = count_;

      //tell left child to construct
      uint32_t left_child_max_rank = std::max(left_child, right_child - 1);
      ct.max_rank_ = left_child_max_rank;
      comm_.Send(ct, left_child);
      children_[left_child] = std::make_pair(left_child_max_rank, 0);

      if (left_child != right_child) {
        //tell right child to construct
        ct.max_rank_ = icl::last(subtree_correct_);
        comm_.Send(ct, right_child);
        children_[right_child] = std::make_pair(ct.max_rank_, 0);
      }
    }

    if ((rank_ >= replicas_ || rank_ == primary_) &&
        !listener_subtree_correct_.empty()) {
      uint32_t left_child = icl::first(listener_subtree_correct_);

      uint32_t right_child = 0;
      uint32_t pos = icl::cardinality(listener_subtree_correct_) / 2;
      for (icl::interval_set<uint32_t>::const_iterator it =
             listener_subtree_correct_.begin();
           it != listener_subtree_correct_.end(); ++it) {
        if (icl::cardinality(*it) > pos) {
          right_child = icl::first(*it) + pos;
          break;
        }
        pos -= icl::cardinality(*it);
      }
      assert(right_child > 0);

      spob::ConstructTree ct;
      ct.ancestors_.push_back(rank_);
      ct.ancestors_.insert(ct.ancestors_.end(), ancestors_.begin(),
                           ancestors_.end());
      ct.count_ = count_;

      //tell left child to construct
      uint32_t left_child_max_rank = std::max(left_child, right_child - 1);
      ct.max_rank_ = left_child_max_rank;
      comm_.Send(ct, left_child);
      listener_children_[left_child] = left_child_max_rank;

      if (left_child != right_child) {
        //tell right child to construct
        ct.max_rank_ = icl::last(listener_subtree_correct_);
        comm_.Send(ct, right_child);
        listener_children_[right_child] = ct.max_rank_;
      }
    }
  }
}

void
StateMachine::AckTree()
{
  constructing_ = false;
  if (rank_ == primary_) {
    // Tree construction succeeded, send outstanding proposals down
    // the tree
    RecoverPropose();
  } else {
    spob::AckTree at;
    at.primary_ = primary_;
    at.count_ = count_;
    if (rank_ < replicas_) {
      at.last_mid_ = last_proposed_mid_;
    } else {
      at.last_mid_ = last_committed_mid_;
    }
    comm_.Send(at, ancestors_.front());
  }
}

void
StateMachine::RecoverPropose()
{
  spob::RecoverPropose rp;
  rp.primary_ = primary_;
  // For each child, send a RECOVER_PROPOSE to get them up to date
  if (rank_ < replicas_) {
    for(std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
          = children_.begin();
        it != children_.end(); ++it) {
      if (it->second.second <= last_proposed_mid_) {
        rp.type_ = RecoverPropose::kDiff;
        for(LogType::const_iterator it2 = log_.upper_bound(it->second.second);
            it2 != log_.end(); ++it2) {
          rp.proposals_.push_back(std::make_pair(it2->first, it2->second.first));
        }
      } else {
        rp.type_ = RecoverPropose::kTrunc;
        rp.last_mid_ = last_proposed_mid_;
      }
      comm_.Send(rp, it->first);
    }
  }
  recover_ack_ = subtree_correct_;
}

void
StateMachine::AckRecover()
{
  acked_ = true;
  last_acked_mid_ = last_proposed_mid_;
  if (rank_ == primary_) {
    RecoverCommit();
    RecoverInform();
    current_mid_ = ((last_proposed_mid_ >> 32) + 1) << 32;
    // Make sure we don't wrap around the epoch
    assert(current_mid_ > last_proposed_mid_);
    cb_(kLeading, primary_);
  } else {
    spob::AckRecover ar;
    ar.primary_ = primary_;
    comm_.Send(ar, ancestors_.front());
  }
}

void
StateMachine::RecoverCommit()
{
  spob::RecoverCommit rc;
  rc.primary_ = primary_;
  for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
         = children_.begin();
       it != children_.end(); ++it) {
    comm_.Send(rc, it->first);
  }
  for (LogType::const_iterator it = log_.begin(); it != log_.end(); ++it) {
    cb_(it->first, it->second.first);
  }
  log_.clear();
  recovering_ = false;
}

void
StateMachine::RecoverInform()
{
  spob::RecoverInform ri;
  ri.primary_ = primary_;
  //FIXME: get snapshot
  for (std::map<uint32_t, uint64_t>::const_iterator it
       = listener_children_.begin();
       it != listener_children_.end(); ++it) {
    comm_.Send(ri, it->first);
  }
  for (LogType::const_iterator it = log_.begin(); it != log_.end(); ++it) {
    cb_(it->first, it->second.first);
  }
  recovering_ = false;
}
void
StateMachine::Receive(const spob::Failure& f)
{
  // Remove from the set of correct nodes
  if (f.rank_ <= rank_) {
    lower_correct_ -= f.rank_;
  } else {
    upper_correct_ -= f.rank_;
  }

  if (f.rank_ == primary_) {
    Recover();
  } else if (constructing_ && children_.count(f.rank_)) {
    // failure of child during tree construction
    spob::NakTree nt;
    nt.primary_ = primary_;
    nt.count_ = count_;
    NakTree(nt);
  } else if (!constructing_) {
    if (icl::contains(subtree_correct_, f.rank_)) {
      // failure in our subtree
      subtree_correct_ -= f.rank_;
      children_.erase(f.rank_);
      if (recovering_) {
        // failure doing recovery, check if we can ack
        recover_ack_ -= f.rank_;
        if (recover_ack_.empty()) {
          AckRecover();
        }
      } else {
        // failure during broadcast phase, check outstanding proposals
        // to see if we can ack any of them
        for (LogType::reverse_iterator it = log_.rbegin();
             it != static_cast<LogType::reverse_iterator>
               (log_.upper_bound(last_acked_mid_)); ++it) {
          it->second.second -= f.rank_;
          if (it->second.second.empty()) {
            for (LogType::iterator it2 = log_.upper_bound(last_acked_mid_);
                 it2 != it.base()++; it2++) {
              if (rank_ == primary_) {
                spob::Commit c;
                c.primary_ = primary_;
                c.mid_ = it2->first;
                Commit(c);
              } else {
                spob::Ack a;
                a.primary_ = primary_;
                a.mid_ = it2->first;
                Ack(a);
              }
            }
            break;
          }
        }
      }
    } else if (f.rank_ == ancestors_.front()) {
      // our parent failed, find the closest ancestor and reconnect
      for (std::list<uint32_t>::iterator it = ++(ancestors_.begin());
           it != ancestors_.end(); ++it) {
        if (icl::contains(lower_correct_, *it)) {
          if (recovering_) {
            RecoverReconnect rr;
            rr.primary_ = primary_;
            if (subtree_correct_.empty()) {
              rr.max_rank_ = rank_;
            } else {
              rr.max_rank_ = icl::last(subtree_correct_);
            }
            rr.got_propose_ = got_propose_;
            rr.acked_ = acked_;
            comm_.Send(rr, *it);
          } else {
            Reconnect r;
            r.primary_ = primary_;
            if (subtree_correct_.empty()) {
              r.max_rank_ = rank_;
            } else {
              r.max_rank_ = icl::last(subtree_correct_);
            }
            r.last_proposed_ = last_proposed_mid_;
            r.last_acked_ = last_acked_mid_;
            comm_.Send(r, *it);
          }
          ancestors_.erase(ancestors_.begin(), it);
          break;
        }
      }
    }
  }
}

void
StateMachine::PrintState() {
  std::cout << "Ancestors: {";
  if (!ancestors_.empty()) {
    std::copy(ancestors_.begin(), --(ancestors_.end()),
              std::ostream_iterator<uint32_t>(std::cout, ", "));
    std::cout << ancestors_.back();
  }
  std::cout << "}" << std::endl;

  std::cout << "Children:" << std::endl;
  for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::iterator it =
         children_.begin(); it != children_.end(); ++it) {
    std::cout << "Key: " << it->first << std::endl;
    std::cout << "Val: " << it->second.first << ", " <<
      it->second.second << std::endl;
  }
}
