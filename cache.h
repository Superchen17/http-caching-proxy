#ifndef __CACHE_H__
#define __CACHE_H__

#include "request.h"
#include "response.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <bits/stdc++.h>

class Cache{
  class RequestKeyHash{
    public:
      size_t operator()(const Request& r) const;
  };

  private:
    std::unordered_map<Request, Response, RequestKeyHash> store;

  public:
    Cache(){}
    ~Cache(){}

    void add_entry_to_store(const Request& request, const Response& response);
    void evict_from_store(const Request& request);
    bool exist_in_store(const Request& request) const;
    Response get_cached_response(const Request& request) const;
    void print_store() const;
};

#endif