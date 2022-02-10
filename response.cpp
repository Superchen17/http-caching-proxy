#include "response.h"
#include "exception.h"
#include <sys/socket.h>

void Response::parse_rawHeader(std::string firstBatch){
  this->contentLength = -1;

  std::string headerEnd = "\r\n\r\n";
  size_t pos = firstBatch.find(headerEnd);
  this->rawHeader = firstBatch.substr(0, pos) + headerEnd;
}

Response::Response(std::string firstBatch){
  this->parse_rawHeader(firstBatch);
  this->compute_contentLength();
}

Response::~Response(){

}

std::string Response::get_rawHeader(){
  return this->rawHeader;
}

size_t Response::get_headerLen(){
  return this->rawHeader.length();
}

std::string Response::get_body(){
  return this->body;
}

void Response::compute_contentLength(){
  std::string rawHeaderCopy = this->rawHeader;
  std::string clHeader = "Content-Length: ";
  std::string lineEnd = "\r\n";
  size_t posLineEnd;

  while((posLineEnd = rawHeaderCopy.find(lineEnd)) != std::string::npos){
    std::string line = rawHeaderCopy.substr(0, posLineEnd);
    size_t posClHeader;
    if((posClHeader = line.find(clHeader)) != std::string::npos){
      std::string clStr = line.substr(clHeader.length(), line.find(lineEnd));
      int cl = 0;
      for(int i = 0; i < clStr.length(); i++){
        cl = cl*10 + (clStr[i] - '0');
      }
      this->contentLength = cl;
      break;
    } 
    rawHeaderCopy = rawHeaderCopy.substr(posLineEnd + lineEnd.length());
  }
}

void Response::fetch_rest_body_from_remote(int remoteFd, std::string firstBatch){
  
  int remainLen = this->contentLength;

  // get part of body from the first batch
  std::string headerEnd = "\r\n\r\n";
  this->body = firstBatch.substr(firstBatch.find(headerEnd) + headerEnd.length());

  remainLen -= this->body.length();

  // get the rest of the body
  while(remainLen != 0){
    char batchChars[65536] = {};
    int batchLen = recv(remoteFd, batchChars, 65536, 0);
    std::string batchStr(batchChars, batchLen);
    this->body.append(batchStr);
    remainLen -= batchLen;
  }
}

bool Response::is_chunked(){
  std::string chunkWord = "chunked";
  size_t pos;
  if((pos = this->rawHeader.find(chunkWord)) != std::string::npos){
    return true;
  }
  return false;
}

std::string Response::get_response(){
  return this->rawHeader + this->body;
}

