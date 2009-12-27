
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>

#include <microhttpd.h>


#define DEFAULT_PORT 8080


typedef struct http_request {
    void *cls;
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *version;
    const char *upload_data;
    size_t *upload_data_size;
    void **con_cls;
} http_request_t;


typedef struct MHD_Response http_response_t;



int handle_homepage(http_request_t *request);
int handle_favicon(http_request_t *request);
int handle_error(http_request_t *request, unsigned int status);
int handle_html_page(http_request_t *request, unsigned int status, 
                     const char* title, const char* page);
int handle_static_data(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type);