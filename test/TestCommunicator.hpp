#include <queue>

#include <boost/variant.hpp>

#include "Spob.hpp"

class TestCommunicator : public spob::CommunicatorInterface {
public:
    void Send(const spob::ConstructTree& ct, uint32_t to);
    void Send(const spob::AckTree& at, uint32_t to);

    typedef boost::variant<spob::ConstructTree,
                           spob::AckTree
                           > event_t;
    typedef std::pair<event_t, uint32_t> message_t;
    std::queue<message_t> messages_;
};
