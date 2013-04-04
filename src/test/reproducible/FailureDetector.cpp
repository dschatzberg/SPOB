#include "test/reproducible/FailureDetector.hpp"

FailureDetector::FailureDetector()
{
}

void
FailureDetector::AddCallback(Callback cb, void* data)
{
  cb_ = cb;
  data_ = data;
}

const boost::icl::interval_set<uint32_t>&
FailureDetector::set() const {
  return set_;
}
