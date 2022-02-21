#include "proxy.h"
#include "request.h"
#include "response.h"
#include "exception.h"
#include "customTime.h"

#include <sstream>

Proxy::~Proxy(){

}

int Proxy::get_new_sessionId(){
  int oldId = this->sessionId;

  pthread_mutex_lock(&sessionLock);
  this->sessionId++;
  pthread_mutex_unlock(&sessionLock);

  return oldId;
}

void Proxy::create_addrInfo(const char* _hostname, const char* _port, addrinfo_t** hostInfoList){
  addrinfo_t hostInfo;

  memset(&hostInfo, 0, sizeof(hostInfo));
  hostInfo.ai_family = AF_UNSPEC;
  hostInfo.ai_socktype = SOCK_STREAM;
  hostInfo.ai_flags = AI_PASSIVE;

  int status = getaddrinfo(
    _hostname,
    _port,
    &(hostInfo),
    hostInfoList
  );

  if(_port == NULL){
    get_random_port(hostInfoList);
  }

  if(status != 0){
    throw CustomException("error: cannot get address info for host");
  }
}

void Proxy::get_random_port(addrinfo_t** hostInfoList){
  sockaddr_in_t* addr = (sockaddr_in_t*)((*hostInfoList)->ai_addr);
  addr->sin_port = 0; 
}

int Proxy::create_socket(addrinfo_t* hostInfoList){
  int newSocket = socket(
    hostInfoList->ai_family,
    hostInfoList->ai_socktype,
    hostInfoList->ai_protocol
  );
  if(newSocket == -1){
    throw CustomException("error: cannot create socket");
  }

  return newSocket;
}

void Proxy::bind_socket(addrinfo_t* hostInfoList){
  int yes = 1;
  int status = setsockopt(
    this->socketFd, 
    SOL_SOCKET, 
    SO_REUSEADDR, 
    &yes, 
    sizeof(yes)
  );
  status = bind(
    this->socketFd,
    hostInfoList->ai_addr,
    hostInfoList->ai_addrlen
  );
  if(status == -1){
    throw CustomException("error: cannot bind socket");
  }
}

void Proxy::try_listen(){
  int status = listen(this->socketFd, 100);
  if(status == -1){
    throw CustomException("error: cannot listen on socket");
  }
}

ClientInfo* Proxy::accept_connection(){

  sockaddr_storage_t socketAddr;
  socklen_t socketAddrLen = sizeof(socketAddr);
  int clientConnFd = accept(this->socketFd, (sockaddr_t*)&socketAddr, &socketAddrLen);
  if(clientConnFd == -1){
    throw CustomException("error: cannot accept connection on socket");
  }

  sockaddr_in_t* s = (sockaddr_in_t*)&socketAddr;
  std::string clientIp = inet_ntoa(s->sin_addr);
  int clientPort = ntohs(s->sin_port);

  ClientInfo* clientInfo = new ClientInfo(clientConnFd, clientIp, clientPort, this->get_new_sessionId());
  return clientInfo;
}

void Proxy::create_socket_and_listen(const char* hostname, const char* port){
  addrinfo_t* hostInfoList = NULL;

  this->create_addrInfo(hostname, port, &hostInfoList);
  this->socketFd = this->create_socket(hostInfoList);
  this->bind_socket(hostInfoList);
  this->try_listen();

  freeaddrinfo(hostInfoList);
}

int Proxy::create_socket_and_connect(const char* hostname, const char* port){
  addrinfo_t* hostInfoList = NULL;

  create_addrInfo(hostname, port, &hostInfoList);
  int receivingSocketFd = create_socket(hostInfoList);

  int status = connect(
    receivingSocketFd,
    hostInfoList->ai_addr,
    hostInfoList->ai_addrlen
  );
  if(status == -1){
    throw CustomException("error: cannot connect to socket");
  }

  return receivingSocketFd;
}

Response Proxy::get_revalidation_result_from_remote(Request& request, Response& cachedResp, int remoteFd, ClientInfo* clientInfo){
  std::string newRequestStr = request.get_raw_request();
  std::string lineEnd = "\r\n";

  newRequestStr = newRequestStr.substr(0, newRequestStr.find("\r\n\r\n"));
  newRequestStr += lineEnd;

  if(cachedResp.get_eTag() != ""){
    newRequestStr += ("If-None-Match: " + cachedResp.get_eTag() + lineEnd);
  }
  if(cachedResp.get_lastModified() != ""){
    newRequestStr += ("If-Modified-Since: " + cachedResp.get_lastModified() + lineEnd);
  }
  newRequestStr += lineEnd;

  Request newRequest(newRequestStr);
  Response newResp = Proxy::get_response_from_remote(newRequest, remoteFd, clientInfo);
  
  return newResp;
}

Response Proxy::get_response_from_remote(Request& request, int remoteFd, ClientInfo* clientInfo){
  int sessionId = clientInfo->get_sessionId();

  std::stringstream ss;
  ss << "Requesting " << request.get_header_first_line() 
    << " from " << request.get_host();
  Proxy::print_to_log(sessionId, ss.str(), DEBUG);

  send(remoteFd, request.get_raw_request().c_str(), request.get_raw_request().length(), 0);

  char respChars[MAX_BUFFER_SIZE] = {0};
  int respCharsLen = recv(remoteFd, respChars, MAX_BUFFER_SIZE, 0);
  std::string respStr(respChars, respCharsLen);
  Response resp(respStr);
  if(resp.get_contentLength() != -1){
    resp.fetch_rest_body_from_remote(remoteFd, respStr);
  }

  ss.str(std::string());
  ss << "Received " << resp.get_header_first_line()
    << " from " << request.get_host();
  Proxy::print_to_log(sessionId, ss.str(), DEBUG);
  return resp;
}

void Proxy::relay_chunks_remote_to_client(int remoteFd, int clientFd){
  while(true){
    char respChars[MAX_BUFFER_SIZE] = {0};
    int size = recv(remoteFd, respChars, MAX_BUFFER_SIZE, 0);
    if(size <= 0){
      break;
    }
    send(clientFd, respChars, size, 0);
  }
}

void Proxy::try_respond_with_cache(Request& request, Response& resp, int sessionId, int remoteFd, ClientInfo* clientInfo){
  int needRevalidate = resp.need_revalidation();
  if(needRevalidate > 0){ // check if need to revalidate
    if(needRevalidate == 1){
      Proxy::print_to_log(sessionId, "in cache, requires validation", DEBUG);
    }
    else if(needRevalidate == 2){
      Proxy::print_to_log(sessionId, "in cache, but expired at" + resp.get_expiration_time(), DEBUG);
    }
    Response validateResp = Proxy::get_revalidation_result_from_remote(request, resp, remoteFd, clientInfo);
    std::string notModified = "HTTP/1.1 304 Not Modified";
    if(validateResp.get_rawHeader().find(notModified) == std::string::npos){ // check if modified
      resp = validateResp; // send fresh response to client
      pthread_mutex_lock(&cacheLock);
      cache.evict_from_store(request); // evict stale cache
      cache.add_entry_to_store(request, resp);// add fresh response to cache
      pthread_mutex_unlock(&cacheLock);
    }
  }
  else{
    Proxy::print_to_log(sessionId, "in cache, valid", DEBUG);
  }
}

void Proxy::try_cache_response_from_remote(Request& request, Response& resp, int sessionId){
  int isCacheable = resp.is_cacheable();
  switch(isCacheable){
    case -2:
      Proxy::print_to_log(sessionId, "not cacheable because cache-control=private", DEBUG);
      break;
    case -1:
      Proxy::print_to_log(sessionId, "not cacheable because response not 200", DEBUG);
      break;
    case 0:
      Proxy::print_to_log(sessionId, "not cacheable because cache-control=no-cache", DEBUG);
      break;
    case 1:
      Proxy::print_to_log(sessionId, "cached, expires at " + resp.get_expiration_time(), DEBUG);
      pthread_mutex_lock(&cacheLock);
      cache.add_entry_to_store(request, resp);
      pthread_mutex_unlock(&cacheLock);
      break;
    case 2:
      Proxy::print_to_log(sessionId, "cached, but requires re-validation", DEBUG);
      pthread_mutex_lock(&cacheLock);
      cache.add_entry_to_store(request, resp);
      pthread_mutex_unlock(&cacheLock);
      break;
    default:
      Proxy::print_to_log(sessionId, "not cacheable because other issues", DEBUG);
      break;
  }
}

void Proxy::process_get_request(Request& request, ClientInfo* clientInfo){
  int remoteFd = create_socket_and_connect(
    request.get_host().c_str(), request.get_port().c_str()
  );
  int sessionId = clientInfo->get_sessionId();
  
  Response resp;
  if(cache.exist_in_store(request)){ // if in cache
    resp = cache.get_cached_response(request);
    Proxy::try_respond_with_cache(request, resp, sessionId, remoteFd, clientInfo);
  }
  else{ // if not in cache
    Proxy::print_to_log(sessionId, "not in cache", DEBUG);
    resp = Proxy::get_response_from_remote(request, remoteFd, clientInfo);
    if(resp.is_chunked() == true){ // if chunked, stream back to client, 
                                   // do not wait for entire response, do not cache
      Proxy::print_to_log(sessionId, "not cacheable because chunked", DEBUG);
      Proxy::relay_chunks_remote_to_client(remoteFd, clientInfo->get_clientFd());
      Proxy::print_to_log(sessionId, "Tunnel closed", DEBUG);
      return;
    }
    // if not chunked, try cache response and relay back to client
    Proxy::try_cache_response_from_remote(request, resp, sessionId);
  }
  
  Proxy::print_to_log(sessionId, "Responding " + resp.get_header_first_line(), DEBUG);
  send(clientInfo->get_clientFd(), resp.get_response().c_str(), resp.get_response().length(), 0);
}

void Proxy::process_post_request(Request& request, ClientInfo* clientInfo){
  int remoteFd = create_socket_and_connect(
    request.get_host().c_str(), request.get_port().c_str()
  );
  int sessionId = clientInfo->get_sessionId();

  Response resp = Proxy::get_response_from_remote(request, remoteFd, clientInfo);

  Proxy::print_to_log(sessionId, "Responding " + resp.get_header_first_line(), DEBUG);
  send(clientInfo->get_clientFd(), resp.get_response().c_str(), resp.get_response().length(), 0);
}

void Proxy::process_connect_request(Request& request, ClientInfo* clientInfo){
  int remoteFd = create_socket_and_connect(
    request.get_host().c_str(), request.get_port().c_str()
  );
  int clientFd = clientInfo->get_clientFd();
  
  int maxFd;
  if(clientFd > remoteFd){
    maxFd = clientFd + 1;
  }
  else{
    maxFd = remoteFd + 1;
  }
  
  // send 200 OK to client
  std::string initialResponse = "HTTP/1.1 200 OK\r\n\r\n";
  send(clientInfo->get_clientFd(), initialResponse.c_str(), initialResponse.length(), 0);

  // IO multiplexing and relay
  std::vector<int> fds;
  fds.push_back(clientFd);
  fds.push_back(remoteFd);

  fd_set fdSet;
  while(true){
    FD_ZERO(&fdSet);
    for(int i = 0; i < fds.size(); i++){
      FD_SET(fds[i], &fdSet);
    }
    select(maxFd, &fdSet, NULL, NULL, NULL); 

    for(int i = 0; i < fds.size(); i++){
      if(FD_ISSET(fds[i], &fdSet)){
        char respChars[MAX_BUFFER_SIZE] = {0};
        int respCharsLen;
        if((respCharsLen = recv(fds[i], respChars, MAX_BUFFER_SIZE, 0)) > 0){
          if((respCharsLen = send(fds[(i + 1) % fds.size()], respChars, respCharsLen, 0)) <= 0){
            Proxy::print_to_log(clientInfo->get_sessionId(), "Tunnel closed", DEBUG);
            return;
          }
        }
        else{
          Proxy::print_to_log(clientInfo->get_sessionId(), "Tunnel closed", DEBUG);
          return;
        }
      }
    }  
  }
}

void Proxy::handle_exception(ClientInfo* clientInfo, std::string errorCode){
  std::string httpversion = "HTTP/1.1";
  std::string lineEnd= "\r\n\r\n";

  std::unordered_map<std::string, std::string> errorMap = {
    {"400", "400 Bad Request"},
    {"404", "404 Not Found"},
    {"405", "405 Method Not Allowed"}
  };
  std::string errorResponse = httpversion + " " + errorMap[errorCode] + lineEnd;

  Proxy::print_to_log(clientInfo->get_sessionId(), errorMap[errorCode], DEBUG);
  send(clientInfo->get_clientFd(), errorResponse.c_str(), errorResponse.length(), 0);
}

void Proxy::print_to_log(int sid, std::string message, bool debug){
  std::stringstream ss;
  ss << sid << ": " << message << std::endl;

  if(debug){ // print to console if in debug mode
    std::cout << ss.str();
  }
  else{// write to log if not in debug mode
    pthread_mutex_lock(&logLock);
    proxyLog.open(LOG_PATH, std::ios_base::app);
    if(!proxyLog){
      throw CustomException("Error: cannot open log file");
    }
    proxyLog << ss.str();
    proxyLog.close();
    pthread_mutex_unlock(&logLock);
  }  
}

void* Proxy::handle_client(void* _clientInfo){
  ClientInfo* clientInfo = (ClientInfo*) _clientInfo;

  int clientFd = clientInfo->get_clientFd();
  int sessionId = clientInfo->get_sessionId();

  char requestChars[MAX_BUFFER_SIZE] = {0};
  int requestCharsLen = recv(clientInfo->get_clientFd(), requestChars, MAX_BUFFER_SIZE, 0);
  Request request;

  try{ // if request is malformed, return 400
    std::string requestStr(requestChars, requestCharsLen);
    request = Request(requestStr);
  }
  catch(CustomException& e){ 
    std::string badRequestResponse = "HTTP/1.1 400 Bad Request\r\n\r\n";
    Proxy::handle_exception(clientInfo, "400");

    close(clientFd);
    delete clientInfo;
    return NULL;
  }

  try{
    std::stringstream ss;
    ss << request.get_header_first_line() 
      << " from " << clientInfo->get_clientAddr() 
      << " @ " << CustomTime::get_current_time_in_str();
    Proxy::print_to_log(sessionId, ss.str(), DEBUG);
  }
  catch(CustomException& e){
    Proxy::print_to_log(sessionId, e.what(), DEBUG);
  }

  if(request.get_method() == "GET"){
    try{ // process request and return response
      Proxy::process_get_request(request, clientInfo);
    }
    catch(CustomException& e){ // if error, return 404
      Proxy::handle_exception(clientInfo, "404");
    }
  }
  else if(request.get_method() == "POST"){
    try{
      Proxy::process_post_request(request, clientInfo);
    }
    catch(CustomException& e){
      Proxy::handle_exception(clientInfo, "400");
    }
    catch(...){ // handle a rare error: basic_string::_M_create, to be investigated
      Proxy::handle_exception(clientInfo, "404");
    }
  }
  else if(request.get_method() == "CONNECT"){
    try{
      Proxy::process_connect_request(request, clientInfo);
    }
    catch(CustomException& e){ // if error, return 404
      Proxy::handle_exception(clientInfo, "404");
    }
  }
  else{ // if method not in [GET, POST, CONNECT], return 405
    Proxy::handle_exception(clientInfo, "405");
  }

  close(clientFd);
  delete clientInfo;
  return NULL;
}

void Proxy::run(){
  while(1){
    ClientInfo* clientInfo = this->accept_connection();
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, clientInfo);  
  }
}

int main(){
  Proxy p(NULL, "12345");
  p.run();
}
