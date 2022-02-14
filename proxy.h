#ifndef __PROXY_H__
#define __PROXY_H__

#include "clientInfo.h"
#include "request.h"
#include "cache.h"

#include <sys/socket.h>
#include <utility>

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct addrinfo addrinfo_t;
typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr_storage sockaddr_storage_t;

/**
 * @brief proxy class
 * 
 */
class Proxy{
  private:
    int socketFd;
    Cache cache;

  public:
    Proxy(const char* hostname, const char* port):socketFd(-1){
      this->create_socket_and_listen(hostname, port);
      std::cout << "proxy running on host " << hostname << ", port " << port << std::endl;
    }
    ~Proxy();

    static void create_addrInfo(const char* _hostname, const char* _port, addrinfo_t** hostInfoList);
    static void get_random_port(addrinfo_t** hostInfoList);
    static int create_socket(addrinfo_t* hostInfoList);
    void bind_socket(addrinfo_t* hostInfoList);
    void try_listen();

    ClientInfo* accept_connection();
    void create_socket_and_listen(const char* hostname, const char* port);
    static int create_socket_and_connect(const char* hostname, const char* port);
    void run();

    static void* handle_client(void* _clientInfo);

    static void process_get_request(Request& request, ClientInfo* clientInfo);
    static void process_post_request(Request& request, ClientInfo* clientInfo);
    static void process_connect_request(Request& request, ClientInfo* clientInfo);
};

#endif