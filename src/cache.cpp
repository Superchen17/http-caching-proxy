#include "cache.h"

size_t Cache::RequestKeyHash::operator()(const Request& r) const{
  std::string hashStr = r.get_method() + r.get_host() + r.get_port();
  std::vector<std::string> acceptedEncoding = r.get_acceptedEncoding();
  for(int i = 0; i < acceptedEncoding.size(); i++){
    hashStr += acceptedEncoding[i];
  }
  return std::hash<std::string>()(hashStr);
}

void Cache::replace_using_fifo(){
  Request request = this->fifoQueue.front();
  fifoQueue.pop();
  this->evict_from_store(request);
}

void Cache::add_entry_to_store(const Request& request, const Response& response){
  if(this->store.find(request) == this->store.end()){
    if(this->store.size() == this->maxCacheSize){ // remove first entry if cache full
      this->replace_using_fifo();
    }
    this->store.insert({request, response});
    this->fifoQueue.push(request);
  }
}

void Cache::evict_from_store(const Request& request){
  if(this->store.find(request) != this->store.end())
  {
    this->store.erase(request);
  }
}

bool Cache::exist_in_store(const Request& request) const{
  if(this->store.find(request) == this->store.end()){
    return false;
  }
  else{
    return true;
  }
}

Response Cache::get_cached_response(const Request& request) const{
  auto storeIt = this->store.find(request);
  return storeIt->second;
}

void Cache::print_store() const{
  std::unordered_map<Request, Response>::const_iterator storeIt = this->store.begin();
  while(storeIt != this->store.end()){
    storeIt->first.print_request_fields();
    ++storeIt;
  }
}
