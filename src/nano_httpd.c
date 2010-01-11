/* Nano HTTP Server */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>

#include "nano_httpd.h"




http_server_t* http_new_server()
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
        return -1;
    }

	return 0;
}


void http_free_response(http_response_t* response)
{
	if (response->message) free(response->message);
	if (response->content_buffer) free(response->content_buffer);
	
	free(response);
}

http_response_t* http_new_response()
{
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) {
        perror("failed to allocate memory for http_response_t");
        return NULL;
    } else {
        memset(response, 0, sizeof(http_response_t));
    }
		
    return response;
}

http_response_t* http_new_response_with_options(int status, char* message)
{
    http_response_t* response = http_new_response();
    if (response) {
		response->status = status;
		
		if (message==NULL) {
			switch(status) {
				case 200: message = "OK"; break;
				case 201: message = "Created"; break;
				case 202: message = "Accepted"; break;
				case 204: message = "No Content"; break;
				case 301: message = "Moved Permanently"; break;
				case 302: message = "Moved Temporarily"; break;
				case 304: message = "Not Modified"; break;
				case 400: message = "Bad Request"; break;
				case 401: message = "Unauthorized"; break;
				case 403: message = "Forbidden"; break;
				case 404: message = "Not Found"; break;
				case 500: message = "Internal Server Error"; break;
				case 501: message = "Not Implemented"; break;
				case 502: message = "Bad Gateway"; break;
				case 503: message = "Service Unavailable"; break;
				default: message = "Unknown"; break;
			}
		}
		response->message = strdup(message);
    }
		
    return response;
}

void http_response_content_append(http_response_t* response, char* string)
{
	// Does the buffer already exist?
	if (!response->content_buffer) {
		response->content_buffer_size = BUFSIZ + strlen(string);
		response->content_buffer = malloc(response->content_buffer_size);
		response->content_length = 0;
	}

	// Is the buffer big enough?
	if (response->content_buffer_size - response->content_length < strlen(string)) {
		response->content_buffer_size += strlen(string) + BUFSIZ;
		response->content_buffer = realloc(response->content_buffer, response->content_buffer_size);
	}
	
	// Perform the append
	strcat(response->content_buffer + response->content_length, string);
	response->content_length += strlen(string);
}

http_response_t*  http_response_error_page(int code, char* explanation)
{
	http_response_t* response = http_new_response_with_options(code, NULL);
	char *code_str = malloc(BUFSIZ);

	// FIXME: check for memory allocation error

	// FIXME: assert(code>=100 && code<1000);
	snprintf(code_str, BUFSIZ, "%d ", code);

	http_response_content_append(response, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
	http_response_content_append(response, "<html><head><title>");
	http_response_content_append(response, code_str);
	http_response_content_append(response, response->message);
	http_response_content_append(response, "</title></head>\n");
	http_response_content_append(response, "<body><h1>");
	http_response_content_append(response, code_str);
	http_response_content_append(response, response->message);
	http_response_content_append(response, "</h1>\n");
	
	if (explanation) {
		http_response_content_append(response, "<p>");
		http_response_content_append(response, explanation);
		http_response_content_append(response, "</p>\n");
	}
	http_response_content_append(response, "</body></html>\n");
	
	free(code_str);
	
	return response;
}


static char* http_request_get_line(http_request_t *request)
{
	char *buffer = malloc(BUFSIZ);
	int buffer_size = BUFSIZ;
	int buffer_count = 0;
	int c;

	// FIXME: check memory was allocated
	
	buffer[0] = '\0';

	while (1) {
		// FIXME: is fgetc really slow way of doing things?
		int c = fgetc(request->socket);
		if (c<=0) {
			free(buffer);
			return NULL;
		}

		if (c == '\r') {
			buffer[buffer_count] = '\0';
		} else if (c == '\n') {
			buffer[buffer_count] = '\0';
			break;
		} else {
			buffer[buffer_count] = c;
		}
		
		buffer_count++;
		
		// Expand buffer ?
		if (buffer_count > (buffer_size - 1)) {
			buffer_size *= 2;
			buffer = realloc(buffer, buffer_size);
			// FIXME: check memory was allocated
		}
	}

	return buffer;
}


http_request_t* http_new_request()
{
    http_request_t* request = malloc(sizeof(http_request_t));
    if (!request) {
        perror("failed to allocate memory for http_request_t");
        return NULL;
    } else {
        memset(request, 0, sizeof(http_request_t));
    }
		
    return request;
}

void http_free_request(http_request_t* request)
{
	if (request->method) free(request->method);
	if (request->url) free(request->url);
	if (request->version) free(request->version);
	
	if (request->socket) fclose(request->socket);
	
	free(request);
}


static int http_read_request_line(http_request_t *request)
{
	char *line, *ptr;
	char *method = NULL;
	char *url = NULL;
	char *version = NULL;
	
	line = http_request_get_line(request);
	if (line == NULL || strlen(line) == 0) {
		// FAIL!
		return 400;
	}

	// Skip whitespace at the start
	for (ptr = line; isspace(*ptr); ptr++)
		continue;
		
	// Find the end of the method
	method = ptr;
	while (isalpha(*ptr))
		ptr++;
	*ptr++ = '\0';

	// Find the start of the url
	while (isspace(*ptr) && *ptr != '\n')
		ptr++;
	if (*ptr == '\n' || *ptr == '\0') {
		free(line);
		return 400;
	}
	url = ptr;

	// Find the end of the url
	ptr = &url[strlen(url)];
	while (isspace(*ptr))
		ptr--;
	*ptr = '\0';
	while (!isspace(*ptr) && ptr > url)
		ptr--;
	
	// Is there a version string at the end?
	if (ptr > url && strncasecmp("HTTP/",&ptr[1],5)==0) {
		version = &ptr[6];
		while (isspace(*ptr) && ptr > url)
			ptr--;
		ptr[1] = '\0';
	} else {
		version = "0.9";
	}
	
	// Is the URL valid?
	if (strlen(url)==0) {
		free(line);
		return 400;
	}

	request->method = strdup(method);
	request->url = strdup(url);
	request->version = strdup(version);

	free(line);

	// Success
	return 0;
}

static int http_read_request(http_request_t* request)
{
	if (http_read_request_line(request)) {
		// FIXME: return with 400
		return 400;
	}

	printf("%s: %s\n", request->method, request->url);

	if (strncmp(request->version, "0.9", 3)) {
		// Read in the headers
		while(!feof(request->socket)) {
			char* line = http_request_get_line(request);
			if (line == NULL) break;
			if (strlen(line)==0) {
				free(line);
				break;
			}
			printf("header: %s\n", line);
			free(line);
		}
	}

    return 0;
}


int http_request_send_response(http_request_t *request, http_response_t *response)
{
	if (strncmp(request->version, "0.9", 3)) {
		fprintf(request->socket, "HTTP/1.0 %d %s\r\n", response->status, response->message);
		fprintf(request->socket, "\r\n");
	}
	
	if (response->content_buffer) {
		fprintf(request->socket, response->content_buffer);
	}
	
	request->response_sent = 1;
}


static int http_request_process(FILE* file /*, client address */)
{
	http_request_t* request = http_new_request();
	http_response_t* response = NULL;

	if (!request) return -1;
	request->socket = file;
	// FIXME: store client address
	
	if (http_read_request(request)) {
    	response = http_response_error_page(400, NULL);
	} else {
    	response = http_response_error_page(200, "Everything is OK");
    	printf("method: %s\n", request->method);
    	printf("url: %s\n", request->url);
    	printf("version: %s\n", request->version);
   	}
   	
   	// Send response
   	http_request_send_response(request, response);

	http_free_request(request);

	// Success
	return 0;
}

void http_server_run(http_server_t* server)
{
    struct sockaddr_storage ss;
    struct sockaddr *sa = (struct sockaddr *)&ss;
    socklen_t len = sizeof(ss);
    int nfds = server->socket_max + 1;
    fd_set rfd;
    int i, m;

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
                http_request_process(file);
                fclose(file);
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

