#pragma once

#include <list>
#include <map>

#include <boost/icl/interval_set.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>

namespace spob {
  //Events
  struct ConstructTree {
    std::list<uint32_t> ancestors_;
    uint32_t max_rank_;
  };

  struct AckTree {
  };

  struct Failure {
    Failure(uint32_t rank) : rank_(rank) {}
    uint32_t rank_;
  };

  class CommunicatorInterface {
  public:
    virtual void Send(const ConstructTree& ct, uint32_t to) = 0;
    virtual void Send(const AckTree& at, uint32_t to) = 0;
    virtual ~CommunicatorInterface() {}
  };

  namespace msm = boost::msm;
  namespace msmf = boost::msm::front;
  namespace mpl = boost::mpl;
  namespace icl = boost::icl;

  struct StateMachine_ : msmf::state_machine_def<StateMachine_> {

    struct SharedData {
      SharedData(CommunicatorInterface* comm, uint32_t rank, uint32_t size)
        : comm_(comm), rank_(rank), size_(size) {}
      CommunicatorInterface* comm_;
      typedef std::map<uint32_t, std::pair<uint32_t, uint64_t> > ChildrenMap;
      ChildrenMap children_;
      std::list<uint32_t> ancestors_;
      icl::interval_set<uint32_t> lower_correct_;
      icl::interval_set<uint32_t> higher_correct_;
      icl::interval_set<uint32_t> subtree_correct_;
      uint32_t rank_;
      uint32_t size_;
      uint32_t primary_;
    } shared_data_;

    StateMachine_(CommunicatorInterface* comm, uint32_t rank, uint32_t size)
      : shared_data_(comm, rank, size)
    {
      assert(rank < size);
      shared_data_.lower_correct_ +=
        icl::interval<uint32_t>::closed(0, rank);
      shared_data_.higher_correct_ +=
        icl::interval<uint32_t>::closed(rank + 1, size - 1);
    }

    // List of FSM states
    struct TreeConstruction_ : msmf::state_machine_def<TreeConstruction_> {
      SharedData* shared_data_;
      template <class Event, class FSM>
      void on_entry(const Event&, FSM& fsm)
      {
        shared_data_ = &fsm.shared_data_;
      }

      // List of FSM states
      struct Reset : public msmf::state<> {
        template <class Event, class FSM>
        void on_entry(const Event&, FSM& fsm)
        {
          fsm.shared_data_->children_.clear();
          fsm.shared_data_->ancestors_.clear();
          fsm.shared_data_->primary_ =
            icl::first(fsm.shared_data_->lower_correct_);
          fsm.shared_data_->subtree_correct_ = fsm.shared_data_->higher_correct_;
        }
      };

      struct WaitForParent : public msmf::state<> {
      };

      struct WaitForAck : public msmf::state<> {
      };

      struct Constructing : public msmf::state<> {
      };

      struct Exit : public msmf::exit_pseudo_state<msmf::none> {};

      typedef Reset initial_state;

      // Guards
      struct IsPrimary {
        template <class Event, class FSM, class SourceState, class TargetState>
        bool operator()(const Event&, FSM& fsm, SourceState&, TargetState&) const
        {
          return fsm.shared_data_->primary_ == fsm.shared_data_->rank_;
        }
      };

      struct IsLeaf {
        template <class Event, class FSM, class SourceState, class TargetState>
        bool operator()(const Event&, FSM& fsm, SourceState&, TargetState&) const
        {
          return fsm.shared_data_->subtree_correct_.empty();
        }
      };

      struct ChildFailed {
        template <class Event, class FSM, class SourceState, class TargetState>
        bool operator()(const Event& e, FSM& fsm, SourceState&, TargetState&) const
        {
          return fsm.shared_data_->children_.count(e.rank_);
        }
      };

      // Actions
      struct ChooseChildren {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(const Event&, FSM& fsm, SourceState&, TargetState&) const
        {
          uint32_t left_child = icl::first(fsm.shared_data_->subtree_correct_);
          uint32_t right_child = 0;
          uint32_t pos = icl::cardinality(fsm.shared_data_->subtree_correct_) / 2;
          for (icl::interval_set<uint32_t>::const_iterator it =
                 fsm.shared_data_->subtree_correct_.begin();
               it != fsm.shared_data_->subtree_correct_.end(); ++it) {
            if (icl::cardinality(*it) > pos) {
              right_child = icl::first(*it) + pos;
              break;
            }
            pos -= icl::cardinality(*it);
          }
          //assert(right_child > 0);

          ConstructTree ct;
          ct.ancestors_.push_back(fsm.shared_data_->rank_);
          ct.ancestors_.insert(ct.ancestors_.end(),
                               fsm.shared_data_->ancestors_.begin(),
                               fsm.shared_data_->ancestors_.end());
          ct.max_rank_ = std::max(left_child, right_child - 1);

          fsm.shared_data_->comm_->Send(ct, left_child);
          fsm.shared_data_->children_[left_child] =
            std::make_pair(ct.max_rank_, 0);

          if (left_child != right_child) {
            ct.max_rank_ = icl::last(fsm.shared_data_->subtree_correct_);
            fsm.shared_data_->comm_->Send(ct, right_child);
            fsm.shared_data_->children_[right_child] =
              std::make_pair(ct.max_rank_, 0);
          }
        }
      };

      struct SetupSubtree {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(const Event& e, FSM& fsm, SourceState&, TargetState&) const
        {
          fsm.shared_data_->ancestors_.assign(e.ancestors_.begin(),
                                              e.ancestors_.end());
          fsm.shared_data_->subtree_correct_ -=
            icl::interval<uint32_t>::closed(e.max_rank_ + 1,
                                            fsm.shared_data_->size_ - 1);

          fsm.shared_data_->primary_ = e.ancestors_.back();
        }
      };

      struct Acknowledge {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(const Event& e, FSM& fsm, SourceState&, TargetState&) const
        {
          AckTree at;
          fsm.shared_data_->comm_->Send(at, fsm.shared_data_->ancestors_.front());
        }
      };

      struct RecordFailure {
        template <class Event, class FSM, class SourceState, class TargetState>
        void operator()(const Event& e, FSM& fsm, SourceState&, TargetState&) const
        {
          if (e.rank_ > fsm.shared_data_->rank_) {
            fsm.shared_data_->higher_correct_ -= e.rank_;
          } else {
            fsm.shared_data_->lower_correct_ -= e.rank_;
          }
        }
      };

      // Transition table
      struct transition_table : mpl::vector<
        msmf::Row<Reset, msmf::none, WaitForParent, msmf::none, msmf::none>,
        msmf::Row<Reset, msmf::none, Constructing, msmf::none, IsPrimary>,
        msmf::Row<WaitForParent, ConstructTree, Constructing, SetupSubtree, msmf::none>,
        msmf::Row<Constructing, msmf::none, WaitForAck, ChooseChildren, msmf::none>,
        msmf::Row<Constructing, msmf::none, Exit, Acknowledge, IsLeaf>,
        msmf::Row<WaitForAck, Failure, Reset, RecordFailure, msmf::euml::And_<
                                                               IsPrimary,
                                                               ChildFailed> >
        > {};

      template <class FSM, class Event>
      void no_transition(const Event& e, FSM&, int state)
      {
        std::cout << "TreeConstruction: no transition from state " << state
                  << " on event " << typeid(e).name() << std::endl;
        assert(0);
      }
    };

    typedef msm::back::state_machine<TreeConstruction_> TreeConstruction;

    struct Recovering : public msmf::state<> {
    };

    typedef TreeConstruction initial_state;

    struct transition_table : mpl::vector<
      msmf::Row<TreeConstruction::exit_pt<TreeConstruction_::Exit>,
                msmf::none, Recovering, msmf::none, msmf::none>
      > {};

    template <class FSM, class Event>
    void no_transition(const Event& e, FSM&, int state)
    {
      std::cout << "StateMachine: no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
      assert(0);
    }
  };

  typedef msm::back::state_machine<StateMachine_> StateMachine;
}
