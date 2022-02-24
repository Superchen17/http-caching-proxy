# Danger Log

## Deployment
1. Certain flags (e.g. `DEBUG`, `MAX_BUFFER_SIZE`, `MAX_CACHE_SIZE` etc.) are defined through macro, which require manual modification of the source code during deployment. This issue is currently under investigation.
2. It is highly discouraged to send personal sensitive information through the proxy due to it unknown tunnelling behaviour. 

## Testing
1. All testing is done through [**Postman**](https://www.postman.com/), which generates slightly different requests than the ones by the browsers. The test results may not fully reflect the behaviour of the proxy during production. 

## Concurrency
1. The proxy does not limit the number of threads allowed to spawn. There may be a problem if too many requests are generated at once considering the proxy has no auto-scaling capability.
2. The cache lock has a fine granularity, which means the lock is released between each prob (e.g. check if exist) and action (eviction) to enhance performance. It is expected that the store can be modified between these steps, so extra check is performed in the methods before modification. 

## Failure Handling
1. The proxy is running as a detached container, which requires no TTY persistance. However, the program requires manual restart upon each hypervisor reboot. 
2. The program has an [`Exception`](src/exception.h) class to warn about potential errors at various places. These errors are caught accordingly. However, at the place where unexcepted exceptions have been thrown, a general catch (`catch(...)`) is implemented to prevent throwing out of main. With each exception thrown, a corresponding client-side `4xx` response is sent to the client without modifying the cache store. There is no exception thrown at caching level. All these together makes the program strong exception guarantee. 

## TCP Packet Sizes
1. It is expected that the maximum TCP packet size is 65535 bytes, the size of the sent and received messages are hardcoded accordingly. 

## Requests
1. It is expected that the request sizes are less than 65535 bytes, thus the proxy only receive the client request once. May be problematic if the request size exceeds this value. 
2. Cachine information in the request header is ignored to simplify the implementation. 
3. For a `POST` request, the body may not be sent fully by the proxy if its size exceeds 65535 bytes.  

## Responses
1. The response headers are parsed from the first TCP packet from the remote. If the header is longer than 65535 bytes, there may be the problem of not acquiring the header properly.
2. If the response has no header `Content-Length`, it is impossible for the proxy to determine when to stop listening from remote. 
3. For http client side error, only status code `400`, `404` and `405` are handled, any other error code will be caught and replied with a status code of `400`. 
4. A response is regarded as malformed if has neither the `Content-Length` header nor the `Transfer-Encoding: chunked` header. A status code `502` is returned to the client upon a malformed response.

## Cache Store
1. The cache only cares about the total number of entries, not the total size. This may cause memory problem when each response is exceeding large.
2. The cache uses the [`Request`](src/request.h) class as the hashmap key. A custom hashing function is implemented for `Request`, however, due to the limited number header values used to compute the hash, collision may occur. 
3. A cache entry is only evicted if it fails revalidation, or being the first in the FIFO queue when the store is full. Thus, the store is always almost full and may have significant overhead adding and evicting caches upon each request.
4. The entries in the store are not written to a file. All cached responses will be lost upon proxy shutdown.  

## Cache Revalidation
1. Only limited number of headers are supported, namely `cache-control`, `eTag` and `Last-Modified`.
2. If the header `cache-control` contains the word `must-revalidate`, any other comma-seperated values (e.g. `max-age`) are ignored. 
3. If a response becomes stale but passes revalidation, it is retained in the store and would need revalidation upon each request hit. 
4. If a response has no `cache-control` header, revalidation is required upon each request hit. 