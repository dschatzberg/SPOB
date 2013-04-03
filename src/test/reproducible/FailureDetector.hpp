#pragma once

#include "Spob.hpp"

class FailureDetector : public spob::FailureDetectorInterface {
public:
  FailureDetector();
  void AddCallback(Callback cb, void* data);
  const boost::icl::interval_set<uint32_t>& set() const;
private:
  boost::icl::interval_set<uint32_t> set_;
};
