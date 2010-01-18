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

#include "redhttpd.h"


http_server_t* http_server_new(void)
{
    http_server_t* server = NULL;

    server = calloc(1, sizeof(http_server_t));
    if (!server) {
        perror("failed to allocate memory for http_server_t");
        return NULL;
    } else {
        int i;
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

    handler->method = method ? strdup(method) : NULL;
    handler->path = path ? strdup(path) : NULL;
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
            } else {
                http_server_handle_request(server, cs, sa);
                close(cs);
            }
        }
    }
}

static int match_route(http_handler_t *handler, http_request_t *request)
{
	size_t path_len;
	
	// Does the method match?
	if (!(handler->method == NULL || strcasecmp(handler->method, request->method)==0)) {
		return 0;
	}
	
	// Match any path?
	if (!handler->path) {
		return 1;
	}

	// Glob on the end of the path?
	path_len = strlen(handler->path);
	if (handler->path[path_len-1] == '*' && strncasecmp(handler->path, request->path, path_len-1)==0) {
		const char* glob = &request->path[path_len-1];
		http_request_set_path_glob(request, glob);
		return 1;
	} else if (strcasecmp(handler->path, request->path)==0) {
		// Matched whole path
		return 1;
	}

	// Failed to match
	return 0;
}

int http_server_handle_request(http_server_t* server, int socket, struct sockaddr *sa)
{
    http_request_t *request = NULL;
    http_response_t *response = NULL;
    http_handler_t *it = NULL;
    
    assert(server != NULL);
    assert(socket >= 0);

    request = http_request_new();
    if (!request) return -1;
    request->server = server;
    request->socket = fdopen(socket, "r+");;
    if (getnameinfo(sa, sa->sa_len,
         request->remote_addr, sizeof(request->remote_addr),
         request->remote_port, sizeof(request->remote_port),
         NI_NUMERICHOST | NI_NUMERICSERV))
    {
        perror("could not get numeric hostname of client");
        return -1;
    }    
    
    if (http_request_read_status_line(request)) {
        // Invalid request
        response = http_response_new_error_page(400, NULL);
    } else {
        if (strncmp(request->version, "0.9", 3)) {
            // Read in the headers
            while(!feof(request->socket)) {
                char* line = http_request_read_line(request);
                if (line == NULL || strlen(line)==0) break;
                http_headers_parse_line(&request->headers, line);
                free(line);
            }
            
            // Read in PUT/POST content
            if (strncasecmp(request->method, "POST", 4)==0) {
                char *content_type = http_headers_get(&request->headers, "Content-Type");
                char *content_length = http_headers_get(&request->headers, "Content-Length");
                int bytes_read = 0;

                if (content_type==NULL || content_length==NULL) {
                    response = http_response_new_error_page(400, NULL);
                } else if (strncasecmp(content_type, "application/x-www-form-urlencoded", 33)==0) {
                    request->content_length = atoi(content_length);
                    // FIXME: set maximum POST size
                    request->content_buffer = malloc(request->content_length);
                    // FIXME: check memory allocation succeeded
                    
                    bytes_read = fread(request->content_buffer, 1, request->content_length, request->socket);
                    if (bytes_read != request->content_length) {
                    	// FIXME: better response?
                        response = http_response_new_error_page(400, NULL);
                    } else {
                    	http_request_parse_arguments(request, request->content_buffer);
                    }
                }
            }
        }
    }
    
    
    
    // Dispatch the request
    if (!response) {
        for (it = server->handlers; it; it = it->next) {
            if (match_route(it, request))
            {
                response = it->func(request, it->user_data);
                if (response) break;
            }
        }
    }
    
    // No response? - generate one using the default handler
    if (!response)
        response = http_server_default_handler(server, request);

    // Send response
    http_request_send_response(request, response);

    http_request_free(request);
    http_response_free(response);

    // Success
    return 0;
}

http_response_t *http_server_default_handler(http_server_t* server, http_request_t *request)
{
    http_response_t *response = NULL;
    http_handler_t *it;
    
    assert(server != NULL);
    assert(request != NULL);
    
    // Is it a HEAD request?
    if (strncasecmp("HEAD",request->method,4)==0) {
        for (it = server->handlers; it; it = it->next) {
            if ((it->method && strcasecmp(it->method, "GET")==0) &&
                (it->path && strcasecmp(it->path, request->path)==0))
            {
                response = http_response_new(200, NULL);
                break;
            }
        }
        
        if (!response)
            response = http_response_new(404, NULL);

    } else {
        // Check if another method is allowed instead
        for (it = server->handlers; it; it = it->next) {
            if (it->path && strcasecmp(it->path, request->path)==0)
            {
                response = http_response_new_error_page(405, NULL);
                // FIXME: add list of allowed methods
                break;
            }
        }
    }

    // Must be a 404
    if (!response)
        response = http_response_new_error_page(404, NULL);

    return response;
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
