#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <iostream>
#include <vector>

class Request{
  private:
    std::string method;
    std::string host;
    std::string port;
    std::vector<std::string> lines;

    std::string get_line_with_header(std::string header);

  public:
    Request(std::string requestStr);
    ~Request();

    std::string get_method();
    std::string get_host();
    std::string get_port();

    void from_str_to_vec(std::string requestStr);
    void find_method();
    void find_host_and_port();

    void print_whole_request();
    void print_request_fields();
};

#endif