#pragma once

#include <stdint.h>

#include <list>
#include <ostream>
#include <string>

namespace spob {
  struct ConstructTree {
    uint32_t max_rank_;
    uint64_t count_;
    std::list<uint32_t> ancestors_;
  };
  std::ostream& operator<<(std::ostream& strm, const ConstructTree& ct);

  struct AckTree {
    uint32_t primary_;
    uint64_t count_;
    uint64_t last_mid_;
  };
  std::ostream& operator<<(std::ostream& strm, const AckTree& at);

  struct NakTree {
    uint32_t primary_;
    uint64_t count_;
  };
  std::ostream& operator<<(std::ostream& strm, const NakTree& nt);

  typedef std::pair<uint64_t, std::string> Transaction;
  std::ostream& operator<<(std::ostream& strm, const Transaction& t);

  struct RecoverPropose {
    enum RecoverType {
      kDiff,
      kTrunc
    };
    uint32_t primary_;
    RecoverType type_;
    std::list<Transaction> proposals_; // for diff
    uint64_t last_mid_; // for trunc
  };
  std::ostream& operator<<(std::ostream& strm, const RecoverPropose& rp);

  struct AckRecover {
    uint32_t primary_;
  };
  std::ostream& operator<<(std::ostream& strm, const AckRecover& ar);

  struct RecoverCommit {
    uint32_t primary_;
  };
  std::ostream& operator<<(std::ostream& strm, const RecoverCommit& rc);

  struct RecoverInform {
    uint32_t primary_;
    uint64_t last_committed_;
    std::string snapshot_;
  };
  std::ostream& operator<<(std::ostream& strm, const RecoverInform& ri);

  struct RecoverReconnect {
    uint32_t primary_;
    uint32_t max_rank_;
    uint64_t last_proposed_;
    bool got_propose_;
    bool acked_;
  };
  std::ostream& operator<<(std::ostream& strm, const RecoverReconnect& rr);

  struct ListenerRecoverReconnect {
    uint32_t primary_;
    uint64_t last_committed_;
  };
  std::ostream& operator<<(std::ostream& strm, const ListenerRecoverReconnect& lrr);

  struct Propose {
    uint32_t primary_;
    Transaction proposal_;
  };
  std::ostream& operator<<(std::ostream& strm, const Propose& p);

  struct Ack {
    uint32_t primary_;
    uint64_t mid_;
  };
  std::ostream& operator<<(std::ostream& strm, const Ack& a);

  struct Commit {
    uint32_t primary_;
    uint64_t mid_;
  };
  std::ostream& operator<<(std::ostream& strm, const Commit& c);

  struct Inform {
    uint32_t primary_;
    Transaction t_;
  };
  std::ostream& operator<<(std::ostream& strm, const Inform& i);

  struct Reconnect {
    uint32_t primary_;
    uint32_t max_rank_;
    uint64_t last_proposed_;
    uint64_t last_acked_;
  };
  std::ostream& operator<<(std::ostream& strm, const Reconnect& r);

  struct ListenerReconnect {
    uint32_t primary_;
    uint64_t last_committed_;
  };
  std::ostream& operator<<(std::ostream& strm, const ListenerReconnect& lr);

  struct ReconnectResponse {
    uint32_t primary_;
    uint64_t last_committed_;
    std::list<Transaction> proposals_;
  };
  std::ostream& operator<<(std::ostream& strm, const ReconnectResponse& rr);

  struct ListenerReconnectResponse {
    uint32_t primary_;
    uint64_t last_committed_;
    std::string snapshot_;
  };
  std::ostream& operator<<(std::ostream& strm, const ListenerReconnectResponse& lrr);

  struct Failure {
    uint32_t rank_;
  };
}
