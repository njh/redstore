
#include <sys/socket.h>
#include <netdb.h>



#define HTTP_SERVER_BACKLOG_SIZE  (5)

typedef struct http_header {
    char *key;
    char *value;
    struct http_header *next;
} http_header_t;

typedef struct http_request {
    http_header_t *headers;
    http_header_t *arguments;
    struct http_server *server;
    
    FILE* socket;
    char remote_addr[NI_MAXHOST];
    char remote_port[NI_MAXSERV];

    char *url;
    char *method;
    char *version;
    void* user_data;
    
    char* path;
    char* path_glob;
    char* query_string;
    
    char* content_buffer;
    size_t content_length;
} http_request_t;

typedef struct http_response {
    http_header_t *headers;

    unsigned int status_code;
    char *status_message;
    char *content_buffer;
    size_t content_buffer_size;
    size_t content_length;
    
    int headers_sent;
} http_response_t;

typedef http_response_t*(* http_handler_func)(http_request_t *request, void *user_data);

typedef struct http_handler {
    char *method;
    char *path;
    http_handler_func func;
    void *user_data;
    struct http_handler *next;
} http_handler_t;

typedef struct http_server {
    int sockets[FD_SETSIZE];
    int socket_count;
    int socket_max;
    
    char* signature;
    
    http_handler_t *handlers;
} http_server_t;



void http_headers_send(http_header_t** first, FILE* socket);
void http_headers_add(http_header_t** first, const char* key, const char* value);
char* http_headers_get(http_header_t** first, const char* key);
void http_headers_parse_line(http_header_t** first, const char* line);
void http_headers_free(http_header_t** first);

http_request_t* http_request_new(void);
char* http_request_read_line(http_request_t *request);
char* http_request_get_header(http_request_t *request, const char* key);
char* http_request_get_argument(http_request_t *request, const char* key);
void http_request_set_path_glob(http_request_t *request, const char* path_glob);
const char* http_request_get_path_glob(http_request_t *request);
void http_request_parse_arguments(http_request_t *request, const char *input);
FILE* http_request_get_socket(http_request_t *request);
int http_request_read_status_line(http_request_t *request);
void http_request_send_response(http_request_t *request, http_response_t *response);
void http_request_free(http_request_t* request);

http_response_t* http_response_new(int status, const char* message);
http_response_t* http_response_new_error_page(int code, const char* explanation);
http_response_t* http_response_new_redirect(const char* url);
http_response_t* http_response_new_with_content(const char* data, size_t length, const char* type);
void http_response_content_append(http_response_t* response, const char* fmt, ...);
void http_response_add_header(http_response_t* response, const char* key, const char* value);
void http_response_add_time_header(http_response_t* response, const char* key, time_t timer);
void http_response_set_content(http_response_t* response, const char* data, size_t length, const char* type);
void http_response_free(http_response_t* response);

http_server_t* http_server_new(void);
int http_server_listen(http_server_t* server, const char* host, const char* port, sa_family_t family);
void http_server_add_handler(http_server_t *server, const char* method, const char* path, http_handler_func func, void *user_data);
void http_server_run(http_server_t* server);
int http_server_handle_request(http_server_t* server, int socket, struct sockaddr *sa);
http_response_t *http_server_default_handler(http_server_t* server, http_request_t *request);
void http_server_set_signature(http_server_t* server, const char* signature);
const char* http_server_get_signature(http_server_t* server);
void http_server_free(http_server_t* server);


char* http_url_unescape(const char* escaped);
char* http_url_escape(const char *arg);
