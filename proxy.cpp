#include "proxy.h"
#include "request.h"
#include "response.h"
#include "exception.h"

Proxy::~Proxy(){

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

  ClientInfo* clientInfo = new ClientInfo(clientConnFd, clientIp, clientPort);
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

Response Proxy::get_response_from_remote(Request& request, int remoteFd){
  send(remoteFd, request.get_raw_request().c_str(), request.get_raw_request().length(), 0);

  char respChars[65536] = {0};
  int respCharsLen = recv(remoteFd, respChars, 65536, 0);
  std::string respStr(respChars, respCharsLen);
  Response resp(respStr);
  resp.fetch_rest_body_from_remote(remoteFd, respStr);

  return resp;
}

void Proxy::process_get_request(Request& request, ClientInfo* clientInfo){
  int remoteFd = create_socket_and_connect(
    request.get_host().c_str(), request.get_port().c_str()
  );
  
  Response resp;
  if(cache.exist_in_store(request)){ // if in cache
    // determine if need to revalidate
    resp = cache.get_cached_response(request);
    bool needReval = resp.need_revalidation();
    std::cout << needReval << std::endl;
  }
  else{ // if not in cache
    std::cout << "not in cache " << std::endl;
    resp = Proxy::get_response_from_remote(request, remoteFd);
    cache.add_entry_to_store(request, resp);
  }
  // Response resp = Proxy::get_response_from_remote(request, remoteFd);
  
  send(clientInfo->get_clientFd(), resp.get_response().c_str(), resp.get_response().length(), 0);
}

void Proxy::process_post_request(Request& request, ClientInfo* clientInfo){
  
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
        char respChars[65536] = {0};
        int respCharsLen;
        if((respCharsLen = recv(fds[i], respChars, 65536, 0)) > 0){
          if((respCharsLen = send(fds[(i + 1) % fds.size()], respChars, respCharsLen, 0)) <= 0){
            return;
          }
        }
        else{
          return;
        }
      }
    }  
  }
}

void* Proxy::handle_client(void* _clientInfo){
  // std::cout << "starting handling request... " << std::endl;
  ClientInfo* clientInfo = (ClientInfo*) _clientInfo;

  int clientFd = clientInfo->get_clientFd();

  char requestChars[65536] = {0};
  int requestCharsLen = recv(clientInfo->get_clientFd(), requestChars, 65536, 0);

  try{
    std::string requestStr(requestChars);
    Request request(requestStr);

    if(request.get_method() == "GET"){
      Proxy::process_get_request(request, clientInfo);
    }
    else if(request.get_method() == "POST"){

    }
    else if(request.get_method() == "CONNECT"){
      Proxy::process_connect_request(request, clientInfo);
    }
    else{

    }
    close(clientFd);
    
  }
  catch(CustomException& e){
    std::cerr << e.what() << std::endl;
  }
  catch(std::exception& e){
    std::cerr << "error: unexpected" << std::endl;
  }

  // std::cout << "finished handling request" << std::endl;
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
  Proxy p("127.0.0.1", "12345");
  p.run();
}
