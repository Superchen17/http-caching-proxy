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

};

#endif