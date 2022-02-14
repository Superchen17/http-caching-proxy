#include "cache.h"

size_t Cache::RequestKeyHash::operator()(const Request& r) const{
  return std::hash<std::string>{}(r.get_raw_request());
}
