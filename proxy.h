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
#include <ctime>

/**
 * @brief set to 1 to print log to console, else set to 0
 * 
 */
#define DEBUG 1

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

Cache cache;

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
    int sessionId;

  public:
    Proxy(const char* hostname, const char* port):socketFd(-1), sessionId(1000){
      this->create_socket_and_listen(hostname, port);
      std::cout << "proxy running on host " << hostname << ", port " << port << std::endl;
    }
    ~Proxy();

    int get_new_sessionId();

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
    static void handle_exception(ClientInfo* clientInfo, std::string errorCode);

    static Response get_revalidation_result_from_remote(Request& request, Response& cachedResp, int remoteFd, ClientInfo* clientInfo);
    static Response get_response_from_remote(Request& request, int remoteFd, ClientInfo* clientInfo);

    /**
     * @brief deliver chunked response from server to client
     * 
     * @param remoteFd server file descriptor
     * @param clientFd client file descriptor
     */
    static void relay_chunks_remote_to_client(int remoteFd, int clientFd);

    /**
     * @brief handler for http GET request
     * 
     * @param request client request
     * @param clientInfo client connection info
     */
    static void process_get_request(Request& request, ClientInfo* clientInfo);

    /**
     * @brief handler for http POST request
     * 
     * @param request client request
     * @param clientInfo client connection info
     */
    static void process_post_request(Request& request, ClientInfo* clientInfo);

    /**
     * @brief handler for https CONNECT request
     * 
     * @param request client request
     * @param clientInfo client connection info
     */
    static void process_connect_request(Request& request, ClientInfo* clientInfo);

    /**
     * @brief print to log
     * 
     * @param message message to be recoreed
     * @param debug if true, print to console
     */
    static void print_to_log(int sId, std::string message, bool debug);
};

#endif