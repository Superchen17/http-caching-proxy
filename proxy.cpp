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

void* Proxy::handle_client(void* _clientInfo){
  std::cout << "starting handling request... " << std::endl;
  ClientInfo* clientInfo = (ClientInfo*) _clientInfo;

  int clientFd = clientInfo->get_clientFd();

  char requestChars[65536] = {0};
  int requestCharsLen = recv(clientInfo->get_clientFd(), requestChars, 65536, 0);

  try{
    std::string requestStr(requestChars);
    Request request(requestStr);

    if(request.get_method() == "GET"){

      int remoteFd = create_socket_and_connect(
        request.get_host().c_str(), request.get_port().c_str()
      );
      send(remoteFd, requestChars, requestCharsLen, 0);

      char respChars[65536] = {0};
      int respCharsLen = recv(remoteFd, respChars, 65536, 0);
      std::string respStr(respChars, respCharsLen);
      Response resp(respStr);
      resp.fetch_rest_body_from_remote(remoteFd, respStr);

      send(clientInfo->get_clientFd(), resp.get_response().c_str(), resp.get_response().length(), 0);
    }
    else if(request.get_method() == "POST"){

    }
    else if(request.get_method() == "CONNECT"){

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

  std::cout << "finished handling request" << std::endl;
  delete clientInfo;
  return NULL;
}

void Proxy::handle_method_connect(){

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
