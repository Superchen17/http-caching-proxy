#include "request.h"
#include "exception.h"

Request::Request(std::string requestStr){
  this->from_str_to_vec(requestStr);
  this->find_method();
  this->find_host_and_port();
}

Request::~Request(){

}

std::string Request::get_method(){
  return this->method;
}

std::string Request::get_host(){
  return this->host;
}

std::string Request::get_port(){
  return this->port;
}

std::string Request::get_line_with_header(std::string header){
  std::vector<std::string>::iterator linesIt = this->lines.begin();
  while(linesIt != this->lines.end()){
    size_t pos = 0;
    if((pos = (*linesIt).find(header)) != std::string::npos){
      return *linesIt;
    }
    ++linesIt;
  }
  throw CustomException("error: no host information found in header");
}

void Request::from_str_to_vec(std::string requestStr){
  std::string delimiter = "\r\n";
  size_t pos = 0;
  std::string line;
  std::vector<std::string> lines;

  while((pos = requestStr.find(delimiter)) != std::string::npos){
    line = requestStr.substr(0, pos);
    if(!line.empty()){
      this->lines.push_back(line);
    }
    requestStr = requestStr.substr(pos + delimiter.length());
  }
}

void Request::find_method(){
  std::string lineWithMethod = this->lines[0]; // method always on http header 1st line
  std::string delimiter = " ";
  size_t pos = lineWithMethod.find_first_of(delimiter);
  this->method = lineWithMethod.substr(0, pos);
}

void Request::find_host_and_port(){
  std::string lineEnd = "\r\n";
  std::string lineHead = "Host: ";
  std::string colon = ":";

  std::string lineWithHostPort = this->get_line_with_header(lineHead);

  // prune line head
  lineWithHostPort = lineWithHostPort.substr(lineHead.length());
  
  // prune line end
  size_t posLineEnd = lineWithHostPort.find_first_of(lineEnd);
  lineWithHostPort = lineWithHostPort.substr(0, posLineEnd);

  // find colon between host and port
  // if not found, host is the entire line, 
  size_t posColon;
  if((posColon = lineWithHostPort.find_last_of(colon)) == std::string::npos){
    this->host = lineWithHostPort;
    this->port = "80";
  }
  else{
    this->host = lineWithHostPort.substr(0, posColon);
    this->port = lineWithHostPort.substr(posColon + colon.length());
  }
}

void Request::print_whole_request(){
  std::vector<std::string>::iterator linesIt = this->lines.begin();

  while(linesIt != this->lines.end()){
    std::cout << " - " << *linesIt << std::endl;
    ++linesIt;
  }
}

void Request::print_request_fields(){
  std::cout << "method: " << this->method << std::endl;
  std::cout << "host: " << this->host << std::endl;
  std::cout << "port: " << this->port << std::endl;

}
