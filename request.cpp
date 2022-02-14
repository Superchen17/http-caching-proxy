#include "request.h"
#include "exception.h"

Request::Request(std::string requestStr){
  this->rawRequest = requestStr;
  this->from_str_to_vec(requestStr);
  this->find_method();
  this->find_host_and_port();
  this->find_acceptedEncoding();
}

Request::~Request(){

}

std::string Request::get_raw_request() const{
  return this->rawRequest;
}

std::string Request::get_method() const{
  return this->method;
}

std::string Request::get_host() const{
  return this->host;
}

std::string Request::get_port() const{
  return this->port;
}

std::vector<std::string> Request::get_acceptedEncoding() const{
  return this->acceptedEncoding;
}

bool Request::operator==(const Request& rhs) const{
  if(this->method != rhs.method){
    return false;
  }
  if(this->host != rhs.host){
    return false;
  }
  if(this->port != rhs.port){
    return false;
  }
  if(this->acceptedEncoding != rhs.acceptedEncoding){
    return false;
  }
  return true;
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

  throw CustomException("error: no information found in header");
}

std::string Request::remove_line_header_and_end(std::string line, std::string lineHead, std::string lineEnd){
  
  // prune line header
  line = line.substr(lineHead.length());
  
  // prune line end
  size_t posLineEnd = line.find_first_of(lineEnd);
  line = line.substr(0, posLineEnd);

  return line;  
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
  if(this->lines.empty()){
    throw CustomException("error parsing raw request to lines");
  }
  std::string lineWithMethod = this->lines[0]; // method always on http header 1st line
  std::string delimiter = " ";
  size_t pos;
  if((pos=lineWithMethod.find(delimiter)) != std::string::npos){
    this->method = lineWithMethod.substr(0, pos);
    return;
  }
  throw CustomException("error: no method found in request");
}

void Request::find_host_and_port(){
  std::string lineEnd = "\r\n";
  std::string lineHead = "Host: ";
  std::string colon = ":";

  std::string lineWithHostPort = this->get_line_with_header(lineHead);
  lineWithHostPort = this->remove_line_header_and_end(lineWithHostPort, lineHead, lineEnd);

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

void Request::find_acceptedEncoding(){
  std::string lineEnd = "\r\n";
  std::string lineHead = "Accept-Encoding: ";
  std::string comma = ", ";

  try{
    std::string lineWithEncoding = this->get_line_with_header(lineHead);
    lineWithEncoding = this->remove_line_header_and_end(lineWithEncoding, lineHead, lineEnd);

    int posComma;
    while((posComma=lineWithEncoding.find(comma)) != std::string::npos){
      this->acceptedEncoding.push_back(lineWithEncoding.substr(0, posComma));
      lineWithEncoding = lineWithEncoding.substr(posComma + comma.length());
    }
    this->acceptedEncoding.push_back(lineWithEncoding);
  }
  catch(CustomException& e){
    this->acceptedEncoding.push_back("*");
  }
}


void Request::print_whole_request() const{
  std::vector<std::string>::const_iterator linesIt = this->lines.begin();

  while(linesIt != this->lines.end()){
    std::cout << " - " << *linesIt << std::endl;
    ++linesIt;
  }
}

void Request::print_request_fields() const{
  std::cout << "method: " << this->method << std::endl;
  std::cout << "host: " << this->host << std::endl;
  std::cout << "port: " << this->port << std::endl;
  std::cout << "accepted-encoding: ";

  for(int i = 0; i < this->acceptedEncoding.size(); i++){
    std::cout << acceptedEncoding[i] << ", ";
  }
  std::cout << std::endl;
}
