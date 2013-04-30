#include "Messages.hpp"

#include <algorithm>
#include <iterator>
#include <sstream>

namespace {
  std::string
  TransactionToString(const spob::Transaction &t)
  {
    std::ostringstream str;
    str << "(0x" << std::hex << t.first << std::dec <<
      ", \"" << t.second << "\")";
    return str.str();
  }
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::ConstructTree& ct)
{
  strm << "ConstructTree {" <<
    "max_rank: " << ct.max_rank_ <<
    ", count: " << ct.count_ <<
    ", ancestors: {";
  if (!ct.ancestors_.empty()) {
    std::copy(ct.ancestors_.begin(), --(ct.ancestors_.end()),
              std::ostream_iterator<uint32_t>(strm, ", "));
    strm << ct.ancestors_.back();
  }
  return strm << "}}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::AckTree& at)
{
  return strm << "AckTree {"  <<
    "primary: " << at.primary_ <<
    ", count: " << at.count_ <<
    ", last_mid: 0x" << std::hex << at.last_mid_ << std::dec <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::NakTree& nt)
{
  return strm << "NakTree {" <<
    "primary: " << nt.primary_ <<
    ", count: " << nt.count_ <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::RecoverPropose& rp)
{
  strm << "RecoverPropose {" <<
    "primary: " << rp.primary_ <<
    ", ";
  switch (rp.type_) {
  case spob::RecoverPropose::kDiff:
    strm << "DIFF: {";
    if (!rp.proposals_.empty()) {
      std::transform(rp.proposals_.begin(), --(rp.proposals_.end()),
                     std::ostream_iterator<std::string>(strm, ", "),
                     TransactionToString);
      strm << TransactionToString(rp.proposals_.back());
    }
    strm << "}";
    break;
  case spob::RecoverPropose::kTrunc:
    strm << "TRUNC: {" <<
      "last_mid: 0x" << std::hex <<rp.last_mid_ << std::dec <<
      "}";
    break;
  }
  return strm << "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::AckRecover& ar)
{
  return strm << "AckRecover {" <<
    "primary: " << ar.primary_ <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::RecoverCommit& rc)
{
  return strm << "RecoverCommit {" <<
    "primary: " << rc.primary_ <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::RecoverInform& ri)
{
  return strm << "RecoverInform {" <<
    "primary: " << ri.primary_ <<
    ", snapshot: " << ri.snapshot_ <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::RecoverReconnect& rr)
{
  return strm << "RecoverReconnect {" <<
    "primary: " << rr.primary_ <<
    ", max_rank: " << rr.max_rank_ <<
    ", last_proposed: 0x" << std::hex << rr.last_proposed_ << std::dec <<
    ", got_propose: " << rr.got_propose_ <<
    ", acked: " << rr.acked_ <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::Propose& p)
{
  return strm << "Propose {" <<
    "primary: " << p.primary_ <<
    ", proposal: " << TransactionToString(p.proposal_) <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::Ack& a)
{
  return strm << "Ack {" <<
    "primary: " << a.primary_ <<
    ", mid: 0x" << std::hex << a.mid_ << std::dec <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::Commit& c)
{
  return strm << "Commit {" <<
    "primary: " << c.primary_ <<
    ", mid: 0x" << std::hex << c.mid_ << std::dec <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::Inform& i)
{
  return strm << "Inform {" <<
    "primary: " << i.primary_ <<
    ", message: " << TransactionToString(i.t_) <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::Reconnect& r)
{
  return strm << "Reconnect {" <<
    "primary: " << r.primary_ <<
    ", max_rank: " << r.max_rank_ <<
    ", last_proposed: 0x" << std::hex << r.last_proposed_ <<
    ", last_acked: 0x" << r.last_acked_ << std::dec <<
    "}";
}

std::ostream&
spob::operator<<(std::ostream& strm, const spob::ReconnectResponse& rr)
{
  strm << "ReconnectResponse {" <<
    "primary: " << rr.primary_ <<
    ", last_committed: 0x" << std::hex << rr.last_committed_ << std::dec <<
    ", proposals: {";
  if (!rr.proposals_.empty()) {
    std::transform(rr.proposals_.begin(), --(rr.proposals_.end()),
                   std::ostream_iterator<std::string>(strm, ", "),
                   TransactionToString);
    strm << TransactionToString(rr.proposals_.back());
  }
  return strm << "}}";
}
