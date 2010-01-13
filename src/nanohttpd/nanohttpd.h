
#include <sys/socket.h>



#define HTTP_SERVER_BACKLOG_SIZE  (5)

typedef struct http_header {
    char *key;
    char *value;
    struct http_header *next;
} http_header_t;

typedef struct http_request {
    http_header_t *headers;

    FILE* socket;
    char *url;
    char *method;
    char *version;
    void* user_data;
    
    int response_sent;
    
} http_request_t;

typedef struct http_response {
    http_header_t *headers;

    unsigned int status_code;
    char *status_message;
    char *content_buffer;
    size_t content_buffer_size;
    size_t content_length;
} http_response_t;

typedef http_response_t*(* http_handler_func)(http_request_t *request);

typedef struct http_handler {
    char *method;
    char *path;
    http_handler_func func;
    struct http_handler *next;
} http_handler_t;

typedef struct http_server {
    int sockets[FD_SETSIZE];
    int socket_count;
    int socket_max;
    
    char* server_signature;
    
    http_handler_t *handlers;
} http_server_t;



void http_headers_send(http_header_t** first, FILE* socket);
void http_headers_add(http_header_t** first, const char* key, const char* value);
char* http_message_get_header(http_header_t** first, const char* key);
void http_headers_parse_line(http_header_t** first, const char* line);
void http_headers_free(http_header_t** first);

http_request_t* http_request_new();
char* http_request_get_line(http_request_t *request);
int http_request_send_response(http_request_t *request, http_response_t *response);
int http_request_process(FILE* file /*, client address */);
void http_request_free(http_request_t* request);

http_response_t* http_response_new(int status, const char* message);
void http_response_content_append(http_response_t* response, const char* string);
void http_response_set_content(http_response_t* response, const char* data, size_t length, const char* type);
http_response_t* http_response_error_page(int code, const char* explanation);
void http_response_free(http_response_t* response);

http_server_t* http_server_new();
int http_server_listen(http_server_t* server, const char* host, const char* port, sa_family_t family);
void http_server_add_handler(http_server_t *server, const char* method, const char* path, http_handler_func func);
void http_server_run(http_server_t* server);
void http_server_set_signature(http_server_t* server, const char* signature);
char* http_server_get_signature(http_server_t* server);
void http_server_free(http_server_t* server);


//char* http_uri_escape(char *arg);
