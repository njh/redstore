#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>

#include "httpd.h"


http_server_t* http_new_server(const char* host, const char* port, sa_family_t family)
{
    http_server_t* server = NULL;
    struct addrinfo hints;
    struct addrinfo *res, *res0;
    int err;
    int i;

    server = malloc(sizeof(http_server_t));
    if (!server) {
        perror("failed to allocate memory for http_server_t");
        return NULL;
    } else {
        memset(server, 0, sizeof(http_server_t));
        server->socket_count = 0;
        server->socket_max = -1;
        for (i=0; i<FD_SETSIZE; i++) {
            server->sockets[i] = -1;
        }
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
	
    err = getaddrinfo(host, port, &hints, &res0);
    if (err) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        free(server);
        return NULL;
    }

    /* try and open socket for each of the getaddrinfo() results */
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
        exit(EXIT_FAILURE);
    }

    // Initialise the file descriptor set for use later
    FD_ZERO(&server->rfd);
    for (i = 0; i < server->socket_count; i++)
    {
        FD_SET(server->sockets[i], &server->rfd);
    }
		
    return server;
}


static void read_write(int s)
{
	char buf[BUFSIZ];
  int n;
  
	while ((n = read(s, buf, sizeof(buf))) > 0 &&
	       write(s, buf, n) > 0) {
			buf[n] = '\0';
			fprintf(stderr, "%s", buf);
	}

	close(s);
}


void http_server_run(http_server_t* server)
{
    struct sockaddr_storage ss;
    struct sockaddr *sa = (struct sockaddr *)&ss;
    socklen_t len = sizeof(ss);
    fd_set rfd = server->rfd;
    int nfds = server->socket_max + 1;
    int i, m;

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
                close(server->sockets[i]);
                read_write(cs);
                close(cs);
                exit(EXIT_SUCCESS);
            }
            close(cs);
        }
    }
}


void http_free_server(http_server_t* server)
{
    int i;

    for (i=0;i<server->socket_count; i++) {
        close(server->sockets[i]);
    }
    
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


/* NOTE: content will be freed after the response is sent */

/*
http_response_t* new_http_response(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type)
{
    http_response_t* response = malloc(sizeof(response));
    if (!response) return NULL;
    
    response->status = status;

    // Add headers
    MHD_add_response_header(response->mhd_response, "Content-Type", content_type);
    //MHD_add_response_header(response->mhd_response, "Last-Modified", BUILD_TIME);
    MHD_add_response_header(response->mhd_response, "Server", PACKAGE_NAME "/" PACKAGE_VERSION);

    return response;
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

