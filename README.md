# HTTP Proxy Server
A caching HTTP proxy server with FIFO cache replacement policy.

## Features
### Allowed Request Methods
- GET
- POST
- CONNECT

### Client-Side Error Handling
- 400 (malformed requests and other errors)
- 404 (not found)
- 405 (method not allowed)

### Response Caching Header Supported
- cache-control
- eTag
- Last-Modified

## Production Deployment
1. In [**`proxy.h`**](src/proxy.h), set `#define DEBUG 0` to enable the proxy to write to log
2. In [**`cache.h`**](src/cache.h), set `#define MAX_CACHE_SIZE XXX` where "XXX" is the desired cache size
3. Build and run the project in detached container with 
```
sudo docker-compose up --build -d
```