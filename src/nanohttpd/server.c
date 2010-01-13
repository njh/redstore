/* Nano HTTP Server */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>

#include "nanohttpd.h"

http_server_t* http_server_new()
{
    http_server_t* server = NULL;
    struct addrinfo hints;
    struct addrinfo *res, *res0;
    int err;
    int i;

    server = calloc(1, sizeof(http_server_t));
    if (!server) {
        perror("failed to allocate memory for http_server_t");
        return NULL;
    } else {
        server->socket_count = 0;
        server->socket_max = -1;
        for (i=0; i<FD_SETSIZE; i++) {
            server->sockets[i] = -1;
        }
        server->signature = NULL;
    }
        
    return server;
}

int http_server_listen(http_server_t* server, const char* host, const char* port, sa_family_t family)
{
    struct addrinfo hints;
    struct addrinfo *res, *res0;
    int err;
    int i;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    err = getaddrinfo(host, port, &hints, &res0);
    if (err) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    // try and open socket for each of the getaddrinfo() results
    for (res=res0 ; res && server->socket_count < FD_SETSIZE; res = res->ai_next) {
        char nameinfo_host[NI_MAXHOST];
        char nameinfo_serv[NI_MAXSERV];
        int true = 1;
        int sock;
    
        getnameinfo((struct sockaddr *)res->ai_addr, res->ai_addrlen,
              nameinfo_host, sizeof(nameinfo_host),
              nameinfo_serv, sizeof(nameinfo_serv),
              NI_NUMERICHOST|NI_NUMERICSERV);

        // Create a new socket
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock < 0) {
            fprintf(stderr, "socket() failed: %d\n", sock);
            continue;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(true)) < 0) {
            perror("setsockopt(SO_REUSEADDR");
        }

        // Bind the socket
        if (bind(sock, (struct sockaddr *)res->ai_addr, res->ai_addrlen) < 0) {
            fprintf(stderr, "bind() failed: %s: "
                "[%s]:%s\n", strerror(errno), nameinfo_host, nameinfo_serv);
            close(sock);
            continue;
        }

        // Start listening for connections
        if (listen(sock, HTTP_SERVER_BACKLOG_SIZE) < 0) {
            fprintf(stderr, "listen() failed: [%s]:%s\n", nameinfo_host, nameinfo_serv);
            close(sock);
            continue;
        } else {
            fprintf(stderr, "Listening on: [%s]:%s\n", nameinfo_host, nameinfo_serv);
        }

        if (sock > server->socket_max)
            server->socket_max = sock;
            
        server->sockets[server->socket_count] = sock;
        server->socket_count++;
    }
    freeaddrinfo(res0);

    if (server->socket_count == 0) {
        fprintf(stderr, "no sockets opened\n");
        return -1;
    }

    return 0;
}

void http_server_add_handler(http_server_t *server, const char* method, const char* path, http_handler_func func, void *user_data)
{
    http_handler_t *handler = calloc(1, sizeof(http_handler_t));
    http_handler_t *it;
    
    assert(method != NULL);
    assert(path != NULL);
    
    printf("Registering handler for %s : %s\n", method, path);

    handler->method = strdup(method);
    handler->path = strdup(path);
    handler->func = func;
    handler->user_data = user_data;
    handler->next = NULL;
    
    // append the new method to the list
    if (!server->handlers) {
        server->handlers = handler;
    } else {
        // get to the last member of the list
        for (it = server->handlers; it->next; it = it->next);
        it->next = handler;
    }
}

void http_server_run(http_server_t* server)
{
    struct sockaddr_storage ss;
    struct sockaddr *sa = (struct sockaddr *)&ss;
    socklen_t len = sizeof(ss);
    int nfds = server->socket_max + 1;
    fd_set rfd;
    int i, m;
    
    assert(server != NULL);

    FD_ZERO(&rfd);
    for (i = 0; i < server->socket_count; i++)
    {
        FD_SET(server->sockets[i], &rfd);
    }

    m = select(nfds, &rfd, NULL, NULL, NULL);
    if (m < 0) {
        if (errno == EINTR) return;
        perror("select");
        exit(EXIT_FAILURE);
    }

    for (i=0; i<server->socket_count; i++) {
        if (FD_ISSET(server->sockets[i], &rfd)) {
            int cs = accept(server->sockets[i], sa, &len);
            if (cs < 0) {
                perror("accept"); 
                exit(EXIT_FAILURE);
            }
            if (fork() == 0) {
                FILE* file = fdopen(cs, "r+");
                close(server->sockets[i]);
                http_server_handle_request(server, file);
                fclose(file);
                exit(EXIT_SUCCESS);
            }
            close(cs);
        }
    }
}

int http_server_handle_request(http_server_t* server, FILE* file /*, client address */)
{
    http_request_t *request = http_request_new();
    http_response_t *response = NULL;
    http_handler_t *it;
    
    assert(server != NULL);
    assert(file != NULL);

    if (!request) return -1;
    request->socket = file;
    // FIXME: store client address
    
    if (http_request_read_status_line(request)) {
        // Invalid request
        response = http_response_error_page(400, NULL);
    } else {
        if (strncmp(request->version, "0.9", 3)) {
            // Read in the headers
            while(!feof(request->socket)) {
                char* line = http_request_read_line(request);
                if (line == NULL || strlen(line)==0) break;
                http_headers_parse_line(&request->headers, line);
                free(line);
            }
        }
    }
    
    // Dispatch the request
    if (!response) {
        printf("Request: %s %s\n", request->method, request->url);
        for (it = server->handlers; it; it = it->next) {
            if (strcasecmp(it->method, request->method)==0 && 
                strcasecmp(it->path, request->url)==0)
            {
                response = it->func(request, it->user_data);
            }
        }
    }
    
    // No response?
    if (!response) {
        response = http_response_error_page(404, NULL);
    }

    // Send response
    if (server->signature)
        http_headers_add(&response->headers, "Server", server->signature);
    http_request_send_response(request, response);

    http_request_free(request);
    http_response_free(response);

    // Success
    return 0;
}

void http_server_set_signature(http_server_t* server, const char* signature)
{
    assert(server != NULL);
    assert(signature != NULL);

    if (server->signature)
        free(server->signature);
    server->signature = strdup(signature);
}


const char* http_server_get_signature(http_server_t* server)
{
    assert(server != NULL);
    return server->signature;
}

void http_server_free(http_server_t* server)
{
    http_handler_t *it, *next;
    int i;

    assert(server != NULL);

    for (i=0;i<server->socket_count; i++) {
        close(server->sockets[i]);
    }

    for (it = server->handlers; it; it = next) {
        next = it->next;
        free(it->method);
        free(it->path);
        free(it);
    }

    if (server->signature)
        free(server->signature);

    free(server);
}


/*
char* httpd_escape_uri(char *arg)
{ 
    FILE* stream = NULL;
    char *escaped = NULL;
    size_t escaped_size = 0;
    int i; 

    stream = open_memstream(&escaped, &escaped_size);
    if(!stream) return NULL;

    for (i = 0; arg[i]; i++) {
        char c = arg[i];
        if (c == ' ') {
            fputc('+', stream); 
        } else if (
            (c == 0x2D) || (c == 0x2E) || // Hyphen-Minus, Full Stop
            ((0x30 <= c) && (c <= 0x39)) || // Digits [0-9]
            ((0x41 <= c) && (c <= 0x5A)) || // Uppercase [A-Z]
            ((0x61 <= c) && (c <= 0x7A))    // Lowercase [a-z]
        ) { 
            fputc(c, stream); 
        } else {
            fprintf(stream, "%%%02X", c);
        }
    }
    
    fclose(stream);
    
    return escaped;
}
*/


#define ERROR_MESSAGE_BUFFER_SIZE   (1024)

/*
http_response_t* http_error(http_request_t *request, unsigned int status)
{
    char *message = (char*)malloc(ERROR_MESSAGE_BUFFER_SIZE);
    char *title;
    
    if (status == REDSTORE_HTTP_NOT_FOUND) {
        title = "Not Found";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The requested URL %s was not found on this server.</p>", request->url);
    } else if (status == REDSTORE_HTTP_METHOD_NOT_ALLOWED) {
        title = "Method Not Allowed";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The requested method %s is not allowed for the URL %s.</p>", request->method, request->url);
    } else if (status == REDSTORE_HTTP_INTERNAL_SERVER_ERROR) {
        title = "Internal Server Error";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The server encountered an unexpected condition which prevented it from fulfilling the request.</p>");
    } else if (status == REDSTORE_HTTP_NOT_IMPLEMENTED) {
        title = "Not Implemented";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>Sorry, this functionality has not been implemented.</p>");
    } else {
        title = "Unknown Error";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>An unknown error (%d) occurred.</p>", status);
    }

    return handle_html_page(request, status, title, message);
}
*/

/*
http_response_t* http_redirect(http_request_t *request, char* url)
{
    char *message = (char*)malloc(ERROR_MESSAGE_BUFFER_SIZE);
    http_response_t* response;
    
    snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The document has moved <a href=\"%s\">here</a>.</p>", url);
    response = handle_html_page(request, REDSTORE_HTTP_MOVED_PERMANENTLY, "Moved Permanently", message);
    //MHD_add_response_header(response->mhd_response, MHD_HTTP_HEADER_LOCATION, url);
    free(url);

    return response;
}
*/

