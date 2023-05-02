Check out [gsracz.com](https://gsracz.com) to get served fresh to order

```
 server_start()                        process_connection()
 +------------+     connection thread  +------------------------+
 |            |   +------------------->|                        |
 |  socket()  |   |                    |  recv_request()        |
 |  bind()    |   |                    |  http_parse()          |
 |  listen()  |   |                    |  http_handle_request() |---+
 |            |   |                    |                        |   |
 |  accept()  |   |               +--->|  send_response()       |   |
 |  pthread() |---+               |    |                        |   |
 |            |                   |    +------------------------+   |
 +------------+                   |                                 |
                                  |     http_handle_request()       |
                                  |	   +--------------------+       |       
                                  |    |                    |<------+    content
                                  |	   |  http_validate()   |            +---------------------+
                                  |	   |  http_route()      | route      |                     |  
                                  |	   |  http_fill_body()  |-------+--->|  content_sendfile() |
                                  |	   |  http_format()     |       |    |                     |
                                  +----|                    |       +--->|  content_readfile() |
                                       +--------------------+       |    |                     |
                                                                    +--->|  content_error()    |
                                                                    |    |                     |
                                                                    +--->|  content_perlin()   |
                                                                    |    |                     |
                                                                    +--->|  content_sha256()   |
                                                                         |                     |
                                                                         |         ...         |
                                                                         |                     |
                                                                         +---------------------+
```