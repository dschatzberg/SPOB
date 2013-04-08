#pragma once

#include "Spob.h"

class FailureDetector : public spob::FailureDetectorInterface {
public:
  FailureDetector();
  void AddCallback(Callback cb, void* data);
  const boost::icl::interval_set<uint32_t>& set() const;

  Callback cb_;
  void* data_;
  boost::icl::interval_set<uint32_t> set_;
  std::set<uint32_t> unreported_;
};
