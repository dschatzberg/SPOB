#include "TestCommunicator.hpp"

void
TestCommunicator::Send(const spob::ConstructTree& ct, uint32_t to)
{
    event_t e = ct;
    messages_.push(std::make_pair(ct, to));
}

void
TestCommunicator::Send(const spob::AckTree& at, uint32_t to)
{
    event_t e = at;
    messages_.push(std::make_pair(at, to));
}
