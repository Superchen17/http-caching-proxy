#ifndef __CACHE_H__
#define __CACHE_H__

#include "request.h"
#include "response.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <bits/stdc++.h>
#include <pthread.h>

#define MAX_CACHE_SIZE 100

/**
 * @brief cache class 
 * @note implemented with hashmap ordered by time of insertion
 * @note using FIFO policy to evict entries when full 
 */
class Cache{
  
  /**
   * @brief inner class: hash function for map with custom class key
   * 
   */
  class RequestKeyHash{
    public:
      size_t operator()(const Request& r) const;
  };

  private:
    int maxCacheSize;
    std::unordered_map<Request, Response, RequestKeyHash> store;
    std::queue<Request> fifoQueue;

    /**
     * @brief replace cache using fifo policy when full
     * 
     */
    void replace_using_fifo();

  public:
    Cache(): maxCacheSize(MAX_CACHE_SIZE){}
    ~Cache(){}

    /**
     * @brief add entry to store
     * @note replace cache when full
     * 
     * @param request 
     * @param response 
     */
    void add_entry_to_store(const Request& request, const Response& response);
    
    /**
     * @brief remove a certain entry from cache base on request
     * 
     * @param request key used for look up
     */
    void evict_from_store(const Request& request);

    /**
     * @brief check if a entry exist in store
     * 
     * @param request key used for look up
     * @return true when exists
     * @return false when not exists
     */
    bool exist_in_store(const Request& request) const;

    /**
     * @brief Get the cached response object 
     * 
     * @param request key used for look up
     * @return Response 
     */
    Response get_cached_response(const Request& request) const;

    /**
     * @brief print all entries in store
     * 
     */
    void print_store() const;
};

#endif