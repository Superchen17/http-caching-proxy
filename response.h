#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <iostream>
#include <vector>

class Response{
  private:
    std::vector<std::string> lines;
    std::string rawHeader;
    std::string date;
    int contentLength;
    std::string cacheControl;
    std::string lastModified;
    std::string eTag;
    std::string expires;
    std::string body;

    std::string parse_string_field(std::string lineHead, std::string lineEnd);
    void parse_rawHeader(std::string firstBatch);
    void compute_contentLength();
    void parse_date();
    void parse_cacheControl();
    void parse_lastModified();
    void parse_eTag();
    void parse_expires();

    void from_str_to_vec(std::string responseHeader);
    std::string get_line_with_header(std::string header);
    std::string remove_line_header_and_end(std::string line, std::string lineHead, std::string lineEnd);

  public:
    Response(): contentLength(-1){}
    Response(std::string firstBatch);
    ~Response();

    std::string get_rawHeader();
    size_t get_headerLen();
    int get_contentLength();
    std::string get_body();
    std::string get_response();
    std::string get_lastModified();
    std::string get_eTag();

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
    bool is_cacheable();
    bool need_revalidation();

    void print_response_headers();
};

#endif
