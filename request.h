#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <iostream>
#include <vector>

class Request{
  private:
    std::string rawRequest;
    std::string method;
    std::string host;
    std::string port;
    std::vector<std::string> acceptedEncoding;
    std::vector<std::string> lines;

    std::string get_line_with_header(std::string header);
    std::string remove_line_header_and_end(std::string line, std::string lineHead, std::string lineEnd);

  public:
    Request(){}
    Request(std::string requestStr);
    bool operator==(const Request& rhs) const;
    ~Request();

    std::string get_raw_request() const;
    std::string get_header_first_line() const;
    std::string get_method() const;
    std::string get_host() const;
    std::string get_port() const;
    std::vector<std::string> get_acceptedEncoding() const;

    void from_str_to_vec(std::string requestStr);
    void find_method();
    void find_host_and_port();
    void find_acceptedEncoding();

    void print_whole_request() const;
    void print_request_fields() const;
};

#endif