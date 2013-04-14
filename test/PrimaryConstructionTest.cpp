#include "gtest/gtest.h"

#include "Spob.hpp"
#include "TestCommunicator.hpp"

namespace {
  class PrimaryWithChildrenConstructionTest : public ::testing::Test {
  protected:
    PrimaryWithChildrenConstructionTest()
      : sm_(&comm_, 0, 5)
    {
    }

    virtual void SetUp() {
      sm_.start();
    }

    TestCommunicator comm_;
    spob::StateMachine sm_;
  };

  TEST_F(PrimaryWithChildrenConstructionTest, PicksChildren) {
    ASSERT_EQ(2u, comm_.messages_.size());
    TestCommunicator::message_t m = comm_.messages_.front();
    ASSERT_EQ(1u, m.second);
    spob::ConstructTree* ct = boost::get<spob::ConstructTree>(&m.first);
    ASSERT_TRUE(ct != NULL);
    ASSERT_EQ(1u, ct->ancestors_.size());
    ASSERT_EQ(0u, ct->ancestors_.front());
    ASSERT_EQ(2u, ct->max_rank_);
    comm_.messages_.pop();
    m = comm_.messages_.front();
    ASSERT_EQ(3u, m.second);
    ct = boost::get<spob::ConstructTree>(&m.first);
    ASSERT_TRUE(ct != NULL);
    ASSERT_EQ(1u, ct->ancestors_.size());
    ASSERT_EQ(0u, ct->ancestors_.front());
    ASSERT_EQ(4u, ct->max_rank_);
  }

  TEST_F(PrimaryWithChildrenConstructionTest, ReconstructOnFailure) {
    comm_.messages_.pop();
    comm_.messages_.pop();
    sm_.process_event(spob::Failure(1));
    ASSERT_EQ(2u, comm_.messages_.size());
    TestCommunicator::message_t m = comm_.messages_.front();
    ASSERT_EQ(2u, m.second);
    spob::ConstructTree* ct = boost::get<spob::ConstructTree>(&m.first);
    ASSERT_TRUE(ct != NULL);
    ASSERT_EQ(1u, ct->ancestors_.size());
    ASSERT_EQ(0u, ct->ancestors_.front());
    ASSERT_EQ(2u, ct->max_rank_);
    comm_.messages_.pop();
    m = comm_.messages_.front();
    ASSERT_EQ(3u, m.second);
    ct = boost::get<spob::ConstructTree>(&m.first);
    ASSERT_TRUE(ct != NULL);
    ASSERT_EQ(1u, ct->ancestors_.size());
    ASSERT_EQ(0u, ct->ancestors_.front());
    ASSERT_EQ(4u, ct->max_rank_);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
