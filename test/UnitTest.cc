#include <queue>

#include <boost/variant.hpp>

#include "gtest/gtest.h"

#include "Spob.h"

namespace {
  class TreeConstructionTest : public ::testing::Test {
  protected:
    TreeConstructionTest()
      : root_(&comm_root_, 0, 5),
        parent_(&comm_parent_, 1, 5),
        leaf_(&comm_leaf_, 2, 5)
    {
    }

    virtual void SetUp() {
      root_.start();
      parent_.start();
      leaf_.start();
    }

    class Communicator : public spob::CommunicatorInterface {
    public:
      void Send(const spob::ConstructTree& ct, uint32_t to) {
        event_t e = ct;
        messages_.push(std::make_pair(ct, to));
      }
      void Send(const spob::AckTree& at, uint32_t to) {
        event_t e = at;
        messages_.push(std::make_pair(at, to));
      }

      typedef boost::variant<spob::ConstructTree,
                             spob::AckTree
                             > event_t;
      typedef std::pair<event_t, uint32_t> message_t;
      std::queue<message_t> messages_;
    };

    Communicator comm_root_;
    Communicator comm_parent_;
    Communicator comm_leaf_;
    spob::StateMachine root_;
    spob::StateMachine parent_;
    spob::StateMachine leaf_;
  };
}

TEST_F(TreeConstructionTest, RootPicksChildren) {
  ASSERT_EQ(2u, comm_root_.messages_.size());
  Communicator::message_t m = comm_root_.messages_.front();
  ASSERT_EQ(1u, m.second);
  spob::ConstructTree* ct = boost::get<spob::ConstructTree>(&m.first);
  ASSERT_TRUE(ct != NULL);
  ASSERT_EQ(1u, ct->ancestors_.size());
  ASSERT_EQ(0u, ct->ancestors_.front());
  ASSERT_EQ(2u, ct->max_rank_);
  comm_root_.messages_.pop();
  m = comm_root_.messages_.front();
  ASSERT_EQ(3u, m.second);
  ct = boost::get<spob::ConstructTree>(&m.first);
  ASSERT_TRUE(ct != NULL);
  ASSERT_EQ(1u, ct->ancestors_.size());
  ASSERT_EQ(0u, ct->ancestors_.front());
  ASSERT_EQ(4u, ct->max_rank_);
}

TEST_F(TreeConstructionTest, ParentIdles) {
  ASSERT_EQ(comm_parent_.messages_.size(), 0u);
}

TEST_F(TreeConstructionTest, ParentChoosesChildren) {
  spob::ConstructTree ct;
  ct.ancestors_.push_front(0);
  ct.max_rank_ = 2;

  parent_.process_event(ct);
  ASSERT_EQ(1u, comm_parent_.messages_.size());
  Communicator::message_t m = comm_parent_.messages_.front();
  ASSERT_EQ(2u, m.second);
  spob::ConstructTree* ctp = boost::get<spob::ConstructTree>(&m.first);
  ASSERT_TRUE(ctp != NULL);
  ASSERT_EQ(2u, ctp->ancestors_.size());
  ASSERT_EQ(1u, ctp->ancestors_.front());
  ctp->ancestors_.pop_front();
  ASSERT_EQ(0u, ctp->ancestors_.front());
  ASSERT_EQ(2u, ctp->max_rank_);
}

TEST_F(TreeConstructionTest, LeafChoosesChildren) {
  spob::ConstructTree ct;
  ct.ancestors_.push_front(0);
  ct.ancestors_.push_front(1);
  ct.max_rank_ = 2;

  leaf_.process_event(ct);
  ASSERT_EQ(1u, comm_leaf_.messages_.size());
  Communicator::message_t m = comm_leaf_.messages_.front();
  ASSERT_EQ(1u, m.second);
  spob::AckTree* at = boost::get<spob::AckTree>(&m.first);
  ASSERT_TRUE(at != NULL);
}

int main (int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
