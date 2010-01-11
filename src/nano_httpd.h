
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

typedef struct http_request {
    FILE* socket;
    char *url;
    char *method;
    char *version;
    void* user_data;
    
    int response_sent;
    
} http_request_t;

typedef struct http_response {
    unsigned int status;
    char *message;
	char *content_buffer;
    size_t content_buffer_size;
    size_t content_length;
    char *content_type;
} http_response_t;



http_server_t* http_new_server();
int http_server_listen(http_server_t* server, const char* host, const char* port, sa_family_t family);
void http_server_run(http_server_t* server);
void http_free_server(http_server_t* server);


char* httpd_escape_uri(char *arg);
