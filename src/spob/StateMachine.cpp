#include "Spob.hpp"

using namespace spob;

StateMachine::StateMachine(uint32_t rank, CommunicatorInterface& comm,
                           FailureDetectorInterface& fd, DeliverFunc df,
                           StatusFunc sf, void* cb_data)
: rank_(rank), comm_(comm), fd_(fd), df_(df), sf_(sf), cb_data_(cb_data)
{
  count_ = 0;
  last_proposed_mid_ = 0;
  last_acked_mid_ = 0;
  last_committed_mid_ = 0;
  fd.AddCallback(FDCallbackStatic, reinterpret_cast<void*>(this));
}

void
StateMachine::Start()
{
  Recover();
}

uint64_t
StateMachine::Propose(const std::string& message)
{
  p_.set_primary(primary_);
  p_.mutable_proposal()->set_mid(current_mid_);
  p_.mutable_proposal()->set_message(message);
  Propose(p_);
  current_mid_++;
  assert(current_mid_ >> 32 == p_.proposal().mid() >> 32);
  return p_.proposal().mid();
}

void
StateMachine::Propose(const spob::Propose& p)
{
  for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it =
         children_.begin(); it != children_.end(); ++it) {
    comm_.Send(p, it->first);
  }
  using namespace boost::icl;
  log_[p.proposal().mid()] = std::make_pair(p.proposal().message(), fd_.set());
  last_proposed_mid_ = p.proposal().mid();
}

void
StateMachine::Receive(const spob::ConstructTree& ct, uint32_t from)
{
  // only act if this message has a newer primary than we are aware of
  // or same primary, but higher count
  uint32_t new_primary = ct.ancestors(ct.ancestors_size() - 1);
  if (new_primary > primary_ ||
      (new_primary == primary_ && ct.count() >= count_)) {
    if (boost::icl::contains(fd_.set(), new_primary)) {
      Recover();
      return;
    }
    ancestors_.assign(ct.ancestors().begin(), ct.ancestors().end());
    primary_ = new_primary;
    count_ = ct.count();
    max_rank_ = ct.max_rank();
    ConstructTree(max_rank_);
  }
}

void
StateMachine::Receive(const spob::AckTree& at, uint32_t from)
{
  if (at.primary() == primary_ && at.count() == count_) {
    assert(children_.count(from) == 1);
    tree_acks_++;
    children_[from].second = at.last_mid();
    if (tree_acks_ == children_.size()) {
      AckTree();
    }
  }
}

void
StateMachine::Receive(const spob::NakTree& nt, uint32_t from)
{
  if (nt.primary() == primary_ && nt.count() == count_) {
    assert(children_.count(from) == 1);
    NakTree(&nt);
  }
}

void
StateMachine::NakTree(const spob::NakTree* nt)
{
  count_++;
  if (primary_ == rank_) {
    ancestors_.clear();
    ConstructTree(max_rank_);
  } else {
    children_.clear();
    if (nt == NULL) {
      nt_.set_primary(primary_);
      nt_.set_count(count_);
      nt = &nt_;
    }
    comm_.Send(*nt, ancestors_.front());
  }
}

void
StateMachine::Receive(const spob::RecoverPropose& rp, uint32_t from)
{
  if (rp.primary() == primary_ && from == ancestors_.front()) {
    got_propose_ = true;
    LogType::iterator hint;
    switch (rp.type()) {
    case spob::RecoverPropose::DIFF:
      if (rp.proposals().size() > 0) {
        hint = --log_.end();
        for (google::protobuf::RepeatedPtrField<spob::Entry>::const_iterator it =
               rp.proposals().begin(); it != rp.proposals().end(); ++it) {
          using namespace boost::icl;
          hint = log_.insert(hint, std::make_pair(it->mid(),
                                                  std::make_pair(it->message(),
                                                                 interval_set<uint32_t>()
                                                                 )));
        }
        last_proposed_mid_ = (--rp.proposals().end())->mid();
      }
      break;
    case spob::RecoverPropose::TRUNC:
      log_.erase(log_.upper_bound(rp.last_mid()), log_.end());
      last_proposed_mid_ = rp.last_mid();
      break;
    }
    if (leaf_) {
      AckRecover();
    } else {
      RecoverPropose();
    }
  }
}

void
StateMachine::Receive(const spob::AckRecover& ar, uint32_t from)
{
  if (ar.primary() == primary_ && children_.count(from)) {
    using namespace boost::icl;
    recover_ack_ += interval<uint32_t>::closed(from, children_[from].first);
    interval_set<uint32_t> set = recover_ack_;
    set += fd_.set();
    if (contains(set, interval<uint32_t>::closed(rank_ + 1, max_rank_))) {
      AckRecover();
    }
  }
}

void
StateMachine::Receive(const spob::RecoverReconnect& rr, uint32_t from)
{
  if (rr.primary() == primary_) {
    if (!rr.got_propose()) {
      rp_.set_primary(primary_);
      rp_.clear_proposals();
      if (rr.last_proposed() <= last_proposed_mid_) {
        rp_.set_type(spob::RecoverPropose::DIFF);
        for (LogType::const_iterator it = log_.upper_bound(rr.last_proposed());
             it != log_.end(); ++it) {
          spob::Entry* ent = rp_.add_proposals();
          ent->set_mid(it->first);
          ent->set_message(it->second.first);
        }
      } else {
        rp_.set_type(spob::RecoverPropose::TRUNC);
        rp_.set_last_mid(last_proposed_mid_);
      }
      comm_.Send(rp_, from);
    } else if (rr.acked()) {
      using namespace boost::icl;
      recover_ack_ += interval<uint32_t>::closed(from, rr.max_rank());
      interval_set<uint32_t> set = recover_ack_;
      set += fd_.set();
      if (contains(set, interval<uint32_t>::closed(rank_ + 1, max_rank_))) {
        AckRecover();
      }
    }
    children_[from] = std::make_pair(rr.max_rank(), rr.last_proposed());
  }
}

void
StateMachine::Receive(const spob::RecoverCommit& rc, uint32_t from)
{
  if (rc.primary() == primary_ && from == ancestors_.front()) {
    RecoverCommit();
    sf_(cb_data_, FOLLOWING, primary_);
  }
}

void
StateMachine::Receive(const spob::Propose& p, uint32_t from)
{
  if (p.primary() == primary_ && from == ancestors_.front()) {
    Propose(p);
    if (leaf_) {
      a_.set_primary(primary_);
      a_.set_mid(p.proposal().mid());
      Ack(a_);
    }
  }
}

void
StateMachine::Ack(const spob::Ack& a)
{
  comm_.Send(a, ancestors_.front());
  last_acked_mid_ = a.mid();
}

void
StateMachine::Receive(const spob::Ack& a, uint32_t from)
{
  if (a.primary() == primary_ && children_.count(from)) {
    assert(log_.count(a.mid()) == 1);
    using namespace boost::icl;
    log_[a.mid()].second += interval<uint32_t>::closed(from, children_[from].first);
    if (contains(log_[a.mid()].second,
                 interval<uint32_t>::closed(rank_ + 1, max_rank_))) {
      if (rank_ == primary_) {
        //commit
        c_.set_primary(primary_);
        c_.set_mid(a.mid());
        Commit(c_);
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
  df_(cb_data_, c.mid(), log_[c.mid()].first);
  log_.erase(log_.begin());
  last_committed_mid_ = c.mid();
}

void
StateMachine::Receive(const spob::Commit& c, uint32_t from)
{
  if (c.primary() == primary_ && from == ancestors_.front()) {
    Commit(c);
  }
}

void
StateMachine::Receive(const spob::Reconnect& r, uint32_t from)
{
  if (r.primary() == primary_) {
    assert(rank_ < from && from <= max_rank_);
    recon_resp_.set_primary(primary_);
    recon_resp_.set_last_committed(last_committed_mid_);
    for (LogType::const_iterator it = log_.upper_bound(r.last_proposed());
         it != log_.end(); ++it) {
      spob::Entry* ent = recon_resp_.add_proposals();
      ent->set_mid(it->first);
      ent->set_message(it->second.first);
    }
    comm_.Send(recon_resp_, from);
    for (LogType::iterator it = log_.upper_bound(last_acked_mid_);
         it != log_.upper_bound(r.last_acked()); ++it) {
      using namespace boost::icl;
      it->second.second += interval<uint32_t>::closed(from, r.max_rank());
      if (contains(it->second.second,
                   interval<uint32_t>::closed(rank_ + 1, max_rank_))) {
        if (rank_ == primary_) {
          c_.set_primary(primary_);
          c_.set_mid(it->first);
          Commit(c_);
        } else {
          a_.set_primary(primary_);
          a_.set_mid(it->first);
          Ack(a_);
        }
      }
    }
    children_[from] = std::make_pair(r.max_rank(), r.last_proposed());
  }
}

void
StateMachine::Receive(const spob::ReconnectResponse& recon_resp, uint32_t from)
{
  if (recon_resp.primary() == primary_ && from == ancestors_.front()) {
    for (std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
           = children_.begin();
         it != children_.end(); ++it) {
      comm_.Send(recon_resp, it->first);
    }
    for (LogType::const_iterator it = log_.begin();
         it != log_.upper_bound(recon_resp.last_committed()); ++it) {
      df_(cb_data_, it->first, it->second.first);
    }
    log_.erase(log_.begin(), log_.upper_bound(recon_resp.last_committed()));
    last_committed_mid_ = recon_resp.last_committed();
    if (recon_resp.proposals().size() > 0) {
      LogType::iterator hint;
      hint = --log_.end();
      for (google::protobuf::RepeatedPtrField<spob::Entry>::const_iterator it =
             recon_resp.proposals().begin();
           it != recon_resp.proposals().end(); ++it) {
        using namespace boost::icl;
        hint = log_.insert(hint, std::make_pair(it->mid(),
                                                std::make_pair(it->message(),
                                                               interval_set<uint32_t>()
                                                               )));
      }
      last_proposed_mid_ = (--recon_resp.proposals().end())->mid();
    }
  }
}

void
StateMachine::Recover()
{
  children_.clear();
  count_ = 0;
  // If the set of failed processes contains the set of processes
  // with lower rank than me, then I am the lowest ranked correct process
  using namespace boost::icl;
  interval_set<uint32_t> lower(interval<uint32_t>::closed(0, rank_));
  lower -= fd_.set();
  primary_ = first(lower);
  if (primary_ == rank_) {
    max_rank_ = comm_.size() - 1;
    ancestors_.clear();
    ConstructTree(max_rank_);
  }
  sf_(cb_data_, RECOVERING, 0);
}

void
StateMachine::ConstructTree(uint32_t max_rank)
{
  got_propose_ = false;
  acked_ = false;
  tree_acks_ = 0;
  children_.clear();
  if (max_rank <= rank_) {
    //no tree needs to be constructed
    leaf_ = true;
    AckTree();
    return;
  }
  using namespace boost::icl;
  interval_set<uint32_t> range(interval<uint32_t>::closed(rank_ + 1,
                                                          max_rank));
  range -= fd_.set();
  if (cardinality(range) == 0) {
    //no tree needs to be constructed
    leaf_ = true;
    AckTree();
    return;
  }
  leaf_ = false;
  //left child is next lowest rank above ours
  uint32_t left_child = first(range);

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

  ct_.clear_ancestors();
  ct_.add_ancestors(rank_);
  for (std::list<uint32_t>::const_iterator it = ancestors_.begin();
       it != ancestors_.end(); ++it) {
    ct_.add_ancestors(*it);
  }
  ct_.set_count(count_);

  //tell left child to construct
  uint32_t left_child_max_rank = std::max(left_child, right_child - 1);
  ct_.set_max_rank(left_child_max_rank);
  comm_.Send(ct_, left_child);
  children_[left_child] = std::make_pair(left_child_max_rank, 0);

  if (left_child == right_child) {
    //we only need one child
    return;
  }
  //tell right child to construct
  ct_.set_max_rank(max_rank);
  comm_.Send(ct_, right_child);
  children_[right_child] = std::make_pair(max_rank, 0);
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
    at_.set_primary(primary_);
    at_.set_count(count_);
    at_.set_last_mid(last_proposed_mid_);
    comm_.Send(at_, ancestors_.front());
  }
}

void
StateMachine::RecoverPropose()
{
  rp_.set_primary(primary_);
  // For each child, send a RECOVER_PROPOSE to get them up to date
  for(std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
        = children_.begin();
      it != children_.end(); ++it) {
    rp_.clear_proposals();
    if (it->second.second <= last_proposed_mid_) {
      rp_.set_type(spob::RecoverPropose::DIFF);
      for(LogType::const_iterator it2 = log_.upper_bound(it->second.second);
          it2 != log_.end(); ++it2) {
        spob::Entry* ent = rp_.add_proposals();
        ent->set_mid(it2->first);
        ent->set_message(it2->second.first);
      }
    } else {
      rp_.set_type(spob::RecoverPropose::TRUNC);
      rp_.set_last_mid(last_proposed_mid_);
    }
    comm_.Send(rp_, it->first);
  }
  recover_ack_.clear();
}

void
StateMachine::AckRecover()
{
  acked_ = true;
  last_acked_mid_ = last_proposed_mid_;
  if (rank_ == primary_) {
    RecoverCommit();
    current_mid_ = ((last_proposed_mid_ >> 32) + 1) << 32;
    assert(current_mid_ > last_proposed_mid_);
    sf_(cb_data_, LEADING, primary_);
  } else {
    ar_.set_primary(primary_);
    comm_.Send(ar_, ancestors_.front());
  }
}

void
StateMachine::RecoverCommit()
{
  rc_.set_primary(primary_);
  for(std::map<uint32_t, std::pair<uint32_t, uint64_t> >::const_iterator it
        = children_.begin();
      it != children_.end(); ++it) {
    comm_.Send(rc_, it->first);
  }
  for(LogType::const_iterator it = log_.begin(); it != log_.end(); ++it) {
    df_(cb_data_, it->first, it->second.first);
  }
  log_.clear();
  recovering_ = false;
}

void
StateMachine::FDCallback(uint32_t rank)
{
  if (rank == primary_) {
    Recover();
  } else if (constructing_ && children_.count(rank)) {
    NakTree(NULL);
  } else if (!constructing_) {
    if (rank_ < rank && rank <= max_rank_) {
      children_.erase(rank);
      if (contains(fd_.set(),
                   boost::icl::interval<uint32_t>::closed(rank_ + 1,
                                                          max_rank_))) {
        leaf_ = true;
      }
      if (recovering_) {
        boost::icl::interval_set<uint32_t> set = recover_ack_;
        set += fd_.set();
        if (contains(set,
                     boost::icl::interval<uint32_t>::closed(rank_ + 1,
                                                            max_rank_))) {
          AckRecover();
        }
      } else {
        for (LogType::iterator it = log_.upper_bound(last_acked_mid_);
             it != log_.end(); ++it) {
          it->second.second += rank;
          if (contains(it->second.second,
                       boost::icl::interval<uint32_t>::closed(rank_ + 1,
                                                              max_rank_))) {
            if (rank_ == primary_) {
              c_.set_primary(primary_);
              c_.set_mid(it->first);
              Commit(c_);
            } else {
              a_.set_primary(primary_);
              a_.set_mid(it->first);
              Ack(a_);
            }
          }
        }
      }
    } else if (ancestors_.front() == rank) {
      for (std::list<uint32_t>::iterator it = ++(ancestors_.begin());
           it != ancestors_.end(); ++it) {
        if (!boost::icl::contains(fd_.set(), *it)) {
          if (recovering_) {
            rr_.set_primary(primary_);
            rr_.set_max_rank(max_rank_);
            rr_.set_got_propose(got_propose_);
            rr_.set_acked(acked_);
            comm_.Send(rr_, *it);
          } else {
            r_.set_primary(primary_);
            r_.set_max_rank(max_rank_);
            r_.set_last_proposed(last_proposed_mid_);
            r_.set_last_acked(last_acked_mid_);
            comm_.Send(r_, *it);
          }
          ancestors_.erase(ancestors_.begin(), it);
          break;
        }
      }
    }
  }
}

void
StateMachine::FDCallbackStatic(void* data, uint32_t rank)
{
  StateMachine* t = reinterpret_cast<StateMachine*>(data);
  t->FDCallback(rank);
}
