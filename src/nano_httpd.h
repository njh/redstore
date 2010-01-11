
#include <sys/socket.h>



#define HTTP_SERVER_BACKLOG_SIZE  (5)


typedef struct http_server {
    fd_set rfd;
    int sockets[FD_SETSIZE];
    int socket_count;
    int socket_max;
    
} http_server_t;

typedef struct http_route {
    const char *method;
    const char *route;
} http_route_t;

typedef struct http_request {
    int socket;
    const char *url;
    const char *method;
    const char *version;
    void* user_data;
} http_request_t;

typedef struct http_response {

    unsigned int status;
} http_response_t;



http_server_t* http_new_server(const char* host, const char* port, sa_family_t family);
void http_server_run(http_server_t* server);
void http_free_server(http_server_t* server);


char* httpd_escape_uri(char *arg);
