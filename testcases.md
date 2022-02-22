# Test cases

## GET
### **success requests**
#### check response
- send a `GET` request to [`http://www.rabihyounes.com/#`](http://www.rabihyounes.com/#), passes if the page is displayed.
#### check chunked response
- send a `GET` request to [`https://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx`](https://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx), passes if the page is displayed.
#### cache revalidation pass with `eTag` and `Last-Modified`
- send two `GET` request to [`http://nginx.org/en/docs/http/ngx_http_core_module.html`](http://nginx.org/en/docs/http/ngx_http_core_module.html), 
check the log, passes if a `200` and a `304` codes are returned consecutively. 
#### cache revalidation pass and fail with `max-age`
- use the testing site [`http://httpbin.org/`](http://httpbin.org/) and demand a response with header `cache-control: max-age=10`. Send three `GET` request, with the second 5 seconds after the first, and the third 11 seconds after the second. Check the log, passes if a `200`, a `304` and a `200` codes are returned consecutively. 
#### cache eviction when full
- use the testing site [`http://httpbin.org/`](http://httpbin.org/) and demand three different `GET` responses. Set the macro `MAX_CACHE_SIZE` in [cache.h](src/cache.h) to 2. Send these three requests consecutively and on the second round, check the log, passes if a `200` (instead of a `304`) is returned. 

### **client side errors**
- send a `PATCH` request to [`http://www.rabihyounes.com/#`](http://www.rabihyounes.com/#), passes if a `405` error is returned.
- send a `GET` request to [`http://www.rabihyoune.com/#`](http://www.rabihyoune.com/#), passes if a `404` error is returned. 
- send a `GET` request to [`http://www.rabihyounes.com/#`](http://www.rabihyounes.com/#) but without the `host` header (untick from Postman), passes if a `400` error is returned. 

## POST
- send a `POST` request to ['http://httpbin.org/forms/post'](http://httpbin.org/forms/post) with the form, passes if a `200` is returned. 

## CONNECT
- send a request to [`google.com`](https://www.google.com), passes if the landing page is displayed.