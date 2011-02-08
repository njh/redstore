/*
    RedHTTP - a lightweight HTTP server library
    Copyright (C) 2010-2011 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>


#ifndef _REDHTTP_H_
#define _REDHTTP_H_

#define DEFAUT_HTTP_SERVER_BACKLOG_SIZE  (16)

enum redhttp_status_code {
  REDHTTP_OK = 200,
  REDHTTP_CREATED = 201,
  REDHTTP_ACCEPTED = 202,
  REDHTTP_NO_CONTENT = 204,

  REDHTTP_MOVED_PERMANENTLY = 301,
  REDHTTP_MOVED_TEMPORARILY = 302,
  REDHTTP_SEE_OTHER = 303,
  REDHTTP_NOT_MODIFIED = 304,

  REDHTTP_BAD_REQUEST = 400,
  REDHTTP_UNAUTHORIZED = 401,
  REDHTTP_FORBIDDEN = 403,
  REDHTTP_NOT_FOUND = 404,
  REDHTTP_METHOD_NOT_ALLOWED = 405,
  REDHTTP_NOT_ACCEPTABLE = 406,

  REDHTTP_INTERNAL_SERVER_ERROR = 500,
  REDHTTP_NOT_IMPLEMENTED = 501,
  REDHTTP_BAD_GATEWAY = 502,
  REDHTTP_SERVICE_UNAVAILABLE = 503
};


typedef struct redhttp_header_s redhttp_header_t;
typedef struct redhttp_request_s redhttp_request_t;
typedef struct redhttp_response_s redhttp_response_t;
typedef struct redhttp_handler_s redhttp_handler_t;
typedef struct redhttp_server_s redhttp_server_t;
typedef struct redhttp_negotiate_s redhttp_negotiate_t;

typedef redhttp_response_t *(*redhttp_handler_func) (redhttp_request_t * request, void *user_data);


void redhttp_headers_print(redhttp_header_t ** first, FILE * socket);
void redhttp_headers_add(redhttp_header_t ** first, const char *key, const char *value);
int redhttp_headers_count(redhttp_header_t ** first);
const char *redhttp_headers_get(redhttp_header_t ** first, const char *key);
int redhttp_headers_get_index(redhttp_header_t ** first, unsigned int index, const char**key, const char** value);
void redhttp_headers_parse_line(redhttp_header_t ** first, const char *line);
void redhttp_headers_free(redhttp_header_t ** first);

redhttp_request_t *redhttp_request_new(void);
redhttp_request_t *redhttp_request_new_with_args(const char *method,
                                                 const char *url, const char *version);
char *redhttp_request_read_line(redhttp_request_t * request);
const char *redhttp_request_get_header(redhttp_request_t * request, const char *key);
int redhttp_request_count_headers(redhttp_request_t * request);
void redhttp_request_print_headers(redhttp_request_t * request, FILE * socket);
void redhttp_request_add_header(redhttp_request_t * request, const char *key, const char *value);
int redhttp_request_count_arguments(redhttp_request_t * request);
void redhttp_request_print_arguments(redhttp_request_t * request, FILE * socket);
const char *redhttp_request_get_argument(redhttp_request_t * request, const char *key);
int redhttp_request_get_argument_index(redhttp_request_t * request, unsigned int index, const char**key, const char** value);
void redhttp_request_set_path_glob(redhttp_request_t * request, const char *path_glob);
const char *redhttp_request_get_path_glob(redhttp_request_t * request);
void redhttp_request_parse_arguments(redhttp_request_t * request, const char *input);
void redhttp_request_set_method(redhttp_request_t * request, const char *method);
const char *redhttp_request_get_method(redhttp_request_t * request);
void redhttp_request_set_url(redhttp_request_t * request, const char *url);
const char *redhttp_request_get_url(redhttp_request_t * request);
void redhttp_request_set_path(redhttp_request_t * request, const char *path);
const char *redhttp_request_get_path(redhttp_request_t * request);
void redhttp_request_set_version(redhttp_request_t * request, const char *version);
const char *redhttp_request_get_version(redhttp_request_t * request);
void redhttp_request_set_query_string(redhttp_request_t * request, const char *query_string);
const char *redhttp_request_get_query_string(redhttp_request_t * request);
const char *redhttp_request_get_remote_addr(redhttp_request_t * request);
const char *redhttp_request_get_remote_port(redhttp_request_t * request);
const char *redhttp_request_get_server_name(redhttp_request_t * request);
const char *redhttp_request_get_server_addr(redhttp_request_t * request);
const char *redhttp_request_get_server_port(redhttp_request_t * request);
void redhttp_request_set_socket(redhttp_request_t * request, FILE * socket);
FILE *redhttp_request_get_socket(redhttp_request_t * request);
void redhttp_request_set_socket(redhttp_request_t * request, FILE * socket);
char *redhttp_request_get_content_buffer(redhttp_request_t * request);
size_t redhttp_request_get_content_length(redhttp_request_t * request);
int redhttp_request_read_status_line(redhttp_request_t * request);
int redhttp_request_read(redhttp_request_t * request);
void redhttp_request_free(redhttp_request_t * request);

redhttp_response_t *redhttp_response_new(int status, const char *message);
redhttp_response_t *redhttp_response_new_with_type(int status, const char *message,
                                                   const char *type);
redhttp_response_t *redhttp_response_new_error_page(int code, const char *explanation);
redhttp_response_t *redhttp_response_new_redirect(const char *url);
int redhttp_response_count_headers(redhttp_response_t * response);
void redhttp_response_print_headers(redhttp_response_t * response, FILE * socket);
const char *redhttp_response_get_header(redhttp_response_t * response, const char *key);
void redhttp_response_add_header(redhttp_response_t * response, const char *key, const char *value);
void redhttp_response_add_time_header(redhttp_response_t * response, const char *key, time_t timer);
void redhttp_response_copy_content(redhttp_response_t * response,
                                   const char *content, size_t length);
void redhttp_response_set_content(redhttp_response_t * response, char *buffer, size_t length);
void redhttp_response_send(redhttp_response_t * response, redhttp_request_t * request);
int redhttp_response_get_status_code(redhttp_response_t * response);
const char *redhttp_response_get_status_message(redhttp_response_t * response);
char *redhttp_response_get_content_buffer(redhttp_response_t * response);
size_t redhttp_response_get_content_length(redhttp_response_t * response);
void *redhttp_response_get_user_data(redhttp_response_t * response);
void redhttp_response_set_user_data(redhttp_response_t * response, void *user_data);
void redhttp_response_free(redhttp_response_t * response);

redhttp_server_t *redhttp_server_new(void);
int redhttp_server_listen(redhttp_server_t * server, const char *host,
                          const char *port, sa_family_t family);
void redhttp_server_add_handler(redhttp_server_t * server, const char *method,
                                const char *path, redhttp_handler_func func, void *user_data);
void redhttp_server_run(redhttp_server_t * server);
int redhttp_server_handle_request(redhttp_server_t * server, int socket,
                                  struct sockaddr *sa, size_t sa_len);
redhttp_response_t *redhttp_server_dispatch_request(redhttp_server_t * server,
                                                    redhttp_request_t * request);
void redhttp_server_set_signature(redhttp_server_t * server, const char *signature);
const char *redhttp_server_get_signature(redhttp_server_t * server);
void redhttp_server_set_backlog_size(redhttp_server_t * server, int backlog_size);
int redhttp_server_get_backlog_size(redhttp_server_t * server);
void redhttp_server_free(redhttp_server_t * server);

char *redhttp_negotiate_choose(redhttp_negotiate_t ** server, redhttp_negotiate_t ** client);
redhttp_negotiate_t *redhttp_negotiate_parse(const char *str);
int redhttp_negotiate_get(redhttp_negotiate_t ** first, int i, const char **type, int *q);
int redhttp_negotiate_count(redhttp_negotiate_t ** first);
void redhttp_negotiate_sort(redhttp_negotiate_t ** first);
void redhttp_negotiate_add(redhttp_negotiate_t ** first, const char *type, size_t type_len, int q);
void redhttp_negotiate_free(redhttp_negotiate_t ** first);

char *redhttp_url_unescape(const char *escaped);
char *redhttp_url_escape(const char *arg);


#endif
