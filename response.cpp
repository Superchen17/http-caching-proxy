#include "response.h"
#include "exception.h"
#include "customTime.h"
#include <sys/socket.h>

void Response::parse_rawHeader(std::string firstBatch){
  this->contentLength = -1;

  std::string headerEnd = "\r\n\r\n";
  size_t pos = firstBatch.find(headerEnd);
  this->rawHeader = firstBatch.substr(0, pos) + headerEnd;
}

Response::Response(std::string firstBatch){
  this->parse_rawHeader(firstBatch);
  this->from_str_to_vec(this->rawHeader);
  this->compute_contentLength();
  this->parse_date();
  this->parse_cacheControl();
  this->parse_lastModified();
  this->parse_eTag();
  this->parse_expires();
}

Response::~Response(){

}

std::string Response::parse_string_field(std::string lineHead, std::string lineEnd){
  std::string line = this->get_line_with_header(lineHead);
  std::string field = this->remove_line_header_and_end(line, lineHead, lineEnd);
  return field;
}

std::string Response::get_rawHeader(){
  return this->rawHeader;
}

size_t Response::get_headerLen(){
  return this->rawHeader.length();
}

int Response::get_contentLength(){
  return this->contentLength;
}

std::string Response::get_body(){
  return this->body;
}

std::string Response::get_lastModified(){
  return this->lastModified;
}

std::string Response::get_eTag(){
  return this->eTag;
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

void Response::parse_date(){
  try{
    this->date = this->parse_string_field("Date: ", "\r\n");
  }
  catch(CustomException& e){
    this->date = "";
  }
}

void Response::parse_cacheControl(){
  try{
    this->cacheControl = this->parse_string_field("Cache-Control: ", "\r\n");
  }
  catch(CustomException& e){
    this->cacheControl = "";
  }
}

void Response::parse_lastModified(){
  try{
    this->lastModified = this->parse_string_field("Last-Modified: ", "\r\n");
  }
  catch(CustomException& e){
    this->lastModified = "";
  }
}

void Response::parse_eTag(){
  try{
    this->eTag = this->parse_string_field("ETag: ", "\r\n");
  }
  catch(CustomException& e){
    this->eTag = "";
  }
}

void Response::parse_expires(){
  try{
    this->expires = this->parse_string_field("Expires: ", "\r\n");
  }
  catch(CustomException& e){
    this->expires = "";
  }
}

std::string Response::remove_line_header_and_end(std::string line, std::string lineHead, std::string lineEnd){
  
  // prune line header
  line = line.substr(lineHead.length());
  
  // prune line end
  size_t posLineEnd = line.find_first_of(lineEnd);
  line = line.substr(0, posLineEnd);

  return line;  
}

void Response::from_str_to_vec(std::string responseHeader){
  std::string delimiter = "\r\n";
  size_t pos = 0;
  std::string line;
  std::vector<std::string> lines;

  while((pos = responseHeader.find(delimiter)) != std::string::npos){
    line = responseHeader.substr(0, pos);
    if(!line.empty()){
      this->lines.push_back(line);
    }
    responseHeader = responseHeader.substr(pos + delimiter.length());
  }
}

std::string Response::get_line_with_header(std::string header){
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

bool Response::is_cacheable(){
  std::string noCacheWord = "no-cache";
  size_t posNoCacheWord = this->cacheControl.find(noCacheWord);
  if(posNoCacheWord == std::string::npos){
    return true;
  }
  return false;
}

bool Response::need_revalidation(){
  // if no cache-control header, must revalidate
  if(this->cacheControl == ""){
    return true;
  }

  std::string maxAgeWord = "max-age=";
  std::string mustRevalWord = "must-revalidate";
  size_t posMaxAge, posMustRevalWord;

  // check if cache-control has must-revalidate
  if((posMustRevalWord=this->cacheControl.find(mustRevalWord)) != std::string::npos){
    return true;
  }

  // check if expire header exist
  if(this->expires != ""){
    std::tm* tmExpireTime = CustomTime::convert_string_time_to_tm(this->expires);
    std::time_t timeNow = std::time(0);
    std::time_t timeExpire = mktime(tmExpireTime);

    if(timeNow > timeExpire){ // need revalidate if now > expire time
      delete tmExpireTime;
      return true;
    }
    else{
      delete tmExpireTime;
      return false;
    }
  }
  // check if cache-control has max-age
  else if((posMaxAge=this->cacheControl.find(maxAgeWord)) != std::string::npos){
    std::string maxAgeStr = this->cacheControl.substr(posMaxAge + maxAgeWord.length());

    std::tm* tmDate = CustomTime::convert_string_time_to_tm(this->date);
    std::time_t timeNow = std::time(0);
    std::time_t timeCached = mktime(tmDate);

    // need revalidate if now > (cached time + max age)
    if(timeNow > timeCached + std::stoi(maxAgeStr)){
      delete tmDate;
      return true;
    }
    else{
      delete tmDate;
      return false;
    }
  }
  return true;
}

std::string Response::get_response(){
  return this->rawHeader + this->body;
}

void Response::print_response_headers(){
  std::cout << "date: " << this->date <<  std::endl;
  std::cout << "cache-control: " << this->cacheControl <<  std::endl;
  std::cout << "last-modified: " << this->lastModified <<  std::endl;
  std::cout << "etag: " << this->eTag <<  std::endl;
  std::cout << "expires: " << this->expires <<  std::endl;
  std::cout << "content-length: " << this->contentLength <<  std::endl;
}
