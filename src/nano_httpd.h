
#include <sys/socket.h>



#define HTTP_SERVER_BACKLOG_SIZE  (5)


typedef struct http_server {
    int sockets[FD_SETSIZE];
    int socket_count;
    int socket_max;
    
    char* server_signature;
    
} http_server_t;

typedef struct http_route {
    const char *method;
    const char *route;
} http_route_t;

typedef struct http_header {
    char *key;
    char *value;
} http_header_t;

typedef struct http_message {
    http_header_t *headers;
    size_t header_count;
} http_message_t;

typedef struct http_request {
    http_message_t message;

    FILE* socket;
    char *url;
    char *method;
    char *version;
    void* user_data;
    
    int response_sent;
    
} http_request_t;

typedef struct http_response {
    http_message_t message;

    unsigned int status_code;
    char *status_message;
    char *content_buffer;
    size_t content_buffer_size;
    size_t content_length;
} http_response_t;



http_server_t* http_new_server();
int http_server_listen(http_server_t* server, const char* host, const char* port, sa_family_t family);
void http_server_run(http_server_t* server);
void http_free_server(http_server_t* server);


char* httpd_escape_uri(char *arg);
