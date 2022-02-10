#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <iostream>
#include <vector>

class Response{
  private:
    std::string rawHeader;
    int contentLength;
    std::string body;

    void parse_rawHeader(std::string firstBatch);
    void compute_contentLength();

  public:
    Response(std::string firstBatch);
    ~Response();

    std::string get_rawHeader();
    size_t get_headerLen();
    std::string get_body();
    std::string get_response();

    void fetch_rest_body_from_remote(int remoteFd, std::string firstBatch);

    /**
     * @brief
     * 
     * @warning will not behave properly if "chunked" exist somewhere else in the body
     * 
     * @return true 
     * @return false 
     */
    bool is_chunked();
};

#endif
