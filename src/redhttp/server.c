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

#define _POSIX_C_SOURCE 1

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

#include "redhttp_private.h"
#include "redhttp.h"


redhttp_server_t *redhttp_server_new(void)
{
  redhttp_server_t *server = NULL;

  server = calloc(1, sizeof(redhttp_server_t));
  if (!server) {
    perror("failed to allocate memory for redhttp_server_t");
    return NULL;
  } else {
    int i;
    server->socket_count = 0;
    server->socket_max = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
      server->sockets[i] = -1;
    }
    server->signature = NULL;
    server->backlog_size = DEFAUT_HTTP_SERVER_BACKLOG_SIZE;
  }

  return server;
}

int redhttp_server_listen(redhttp_server_t * server, const char *host,
                          const char *port, sa_family_t family)
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
  for (res = res0; res && server->socket_count < FD_SETSIZE; res = res->ai_next) {
    char nameinfo_host[NI_MAXHOST];
    char nameinfo_serv[NI_MAXSERV];
    int true = 1;
    int sock;

    getnameinfo((struct sockaddr *) res->ai_addr, res->ai_addrlen,
                nameinfo_host, sizeof(nameinfo_host),
                nameinfo_serv, sizeof(nameinfo_serv), NI_NUMERICHOST | NI_NUMERICSERV);

    // Create a new socket
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
      if (server->socket_count < 1)
        fprintf(stderr, "bind() failed: %s: [%s]:%s\n",
                strerror(errno), nameinfo_host, nameinfo_serv);
      continue;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(true)) < 0) {
      if (server->socket_count < 1)
        fprintf(stderr, "setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
    }
    // Bind the socket
    if (bind(sock, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0) {
      if (server->socket_count < 1)
        fprintf(stderr, "bind() failed: %s: [%s]:%s\n",
                strerror(errno), nameinfo_host, nameinfo_serv);
      close(sock);
      continue;
    }
    // Start listening for connections
    if (listen(sock, server->backlog_size) < 0) {
      if (server->socket_count < 1)
        fprintf(stderr, "listen() failed: %s: [%s]:%s\n",
                strerror(errno), nameinfo_host, nameinfo_serv);
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
    return -1;
  }

  return 0;
}

void redhttp_server_add_handler(redhttp_server_t * server, const char *method,
                                const char *path, redhttp_handler_func func, void *user_data)
{
  redhttp_handler_t *handler = calloc(1, sizeof(redhttp_handler_t));
  redhttp_handler_t *it;

  if (!handler)
    return;

  handler->method = redhttp_strdup(method);
  handler->path = redhttp_strdup(path);
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

void redhttp_server_run(redhttp_server_t * server)
{
  struct sockaddr_storage ss;
  struct sockaddr *sa = (struct sockaddr *) &ss;
  socklen_t len = sizeof(ss);
  int nfds = server->socket_max + 1;
  fd_set rfd;
  int i, m;

  assert(server != NULL);

  FD_ZERO(&rfd);
  for (i = 0; i < server->socket_count; i++) {
    FD_SET(server->sockets[i], &rfd);
  }

  m = select(nfds, &rfd, NULL, NULL, NULL);
  if (m < 0) {
    if (errno == EINTR)
      return;
    perror("select");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < server->socket_count; i++) {
    if (FD_ISSET(server->sockets[i], &rfd)) {
      int cs = accept(server->sockets[i], sa, &len);
      if (cs < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      } else {
        redhttp_server_handle_request(server, cs, sa, len);
        close(cs);
      }
    }
  }
}

static int match_route(redhttp_handler_t * handler, redhttp_request_t * request)
{
  size_t path_len;

  // Does the method match?
  if (!(handler->method == NULL || strcmp(handler->method, request->method) == 0)) {
    return 0;
  }
  // Match any path?
  if (!handler->path) {
    return 1;
  }
  // Glob on the end of the path?
  path_len = strlen(handler->path);
  if (handler->path[path_len - 1] == '*'
      && strncmp(handler->path, request->path, path_len - 1) == 0) {
    const char *glob = &request->path[path_len - 1];
    redhttp_request_set_path_glob(request, glob);
    return 1;
  } else if (strcmp(handler->path, request->path) == 0) {
    // Matched whole path
    return 1;
  }
  // Failed to match
  return 0;
}

static int get_server_addr(redhttp_request_t * request, int socket)
{
  struct sockaddr_storage ss;
  struct sockaddr *sa = (struct sockaddr *) &ss;
  socklen_t sa_len = sizeof(ss);

  if (getsockname(socket, sa, &sa_len)) {
    return -1;
  }

  if (getnameinfo(sa, sa_len,
                  request->server_addr, sizeof(request->server_addr),
                  request->server_port, sizeof(request->server_port),
                  NI_NUMERICHOST | NI_NUMERICSERV)) {
    return -1;
  }

  // Success
  return 0;
}

int redhttp_server_handle_request(redhttp_server_t * server, int socket,
                                  struct sockaddr *sa, size_t sa_len)
{
  redhttp_request_t *request = NULL;
  redhttp_response_t *response = NULL;

  assert(server != NULL);
  assert(socket >= 0);

  request = redhttp_request_new();
  if (!request)
    return -1;
  request->server = server;
  request->socket = fdopen(socket, "r+");;
  if (getnameinfo(sa, sa_len,
                  request->remote_addr, sizeof(request->remote_addr),
                  request->remote_port, sizeof(request->remote_port),
                  NI_NUMERICHOST | NI_NUMERICSERV)) {
    perror("could not get numeric hostname of client");
    redhttp_request_free(request);
    return -1;
  }

  if (get_server_addr(request, socket)) {
    perror("could not get numeric hostname of server");
    redhttp_request_free(request);
    return -1;
  }

  if (redhttp_request_read(request)) {
    // Invalid request
    response = redhttp_response_new_error_page(REDHTTP_BAD_REQUEST, NULL);
  }
  // Dispatch the request
  if (!response)
    response = redhttp_server_dispatch_request(server, request);

  // Send response
  redhttp_response_send(response, request);

  redhttp_request_free(request);
  redhttp_response_free(response);

  // Success
  return 0;
}

redhttp_response_t *redhttp_server_dispatch_request(redhttp_server_t * server,
                                                    redhttp_request_t * request)
{
  redhttp_response_t *response = NULL;
  redhttp_handler_t *it;

  assert(server != NULL);
  assert(request != NULL);

  for (it = server->handlers; it; it = it->next) {
    if (match_route(it, request)) {
      response = it->func(request, it->user_data);
      if (response)
        return response;
    }
  }

  // Is it a HEAD request?
  if (strncmp("HEAD", request->method, 4) == 0) {
    for (it = server->handlers; it; it = it->next) {
      if ((it->method && strcmp(it->method, "GET") == 0) &&
          (it->path && strcmp(it->path, request->path) == 0)) {
        response = redhttp_response_new(REDHTTP_OK, NULL);
        break;
      }
    }

    if (!response)
      response = redhttp_response_new(REDHTTP_NOT_FOUND, NULL);

  } else {
    // Check if another method is allowed instead
    for (it = server->handlers; it; it = it->next) {
      if (it->path && strcmp(it->path, request->path) == 0) {
        response = redhttp_response_new_error_page(REDHTTP_METHOD_NOT_ALLOWED, NULL);
        // FIXME: add list of allowed methods
        break;
      }
    }
  }

  // Must be a 404
  if (!response)
    response = redhttp_response_new_error_page(REDHTTP_NOT_FOUND, NULL);

  return response;
}

void redhttp_server_set_signature(redhttp_server_t * server, const char *signature)
{
  assert(server != NULL);

  if (server->signature)
    free(server->signature);

  server->signature = redhttp_strdup(signature);
}


const char *redhttp_server_get_signature(redhttp_server_t * server)
{
  assert(server != NULL);
  return server->signature;
}

void redhttp_server_set_backlog_size(redhttp_server_t * server, int backlog_size)
{
  server->backlog_size = backlog_size;
}

int redhttp_server_get_backlog_size(redhttp_server_t * server)
{
  return server->backlog_size;
}

void redhttp_server_free(redhttp_server_t * server)
{
  redhttp_handler_t *it, *next;
  int i;

  assert(server != NULL);

  for (i = 0; i < server->socket_count; i++) {
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
