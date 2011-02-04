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



#ifndef _REDHTTP_PRIVATE_H_
#define _REDHTTP_PRIVATE_H_

#ifndef NI_MAXHOST
#define NI_MAXHOST (1025)
#endif

#ifndef NI_MAXSERV
#define NI_MAXSERV (32)
#endif


struct redhttp_header_s {
  char *key;
  char *value;
  struct redhttp_header_s *next;
};

struct redhttp_request_s {
  struct redhttp_header_s *headers;
  struct redhttp_header_s *arguments;
  struct redhttp_server_s *server;

  FILE *socket;
  char remote_addr[NI_MAXHOST];
  char remote_port[NI_MAXSERV];

  char server_addr[NI_MAXHOST];
  char server_port[NI_MAXSERV];

  char *url;
  char *method;
  char *version;
  void *user_data;

  char *path;
  char *path_glob;
  char *query_string;

  char *content_buffer;
  size_t content_length;

  struct redhttp_type_q_s *accept;
};

struct redhttp_response_s {
  struct redhttp_header_s *headers;

  unsigned int status_code;
  char *status_message;
  char *content_buffer;
  size_t content_length;

  void *user_data;

  int headers_sent;
};

struct redhttp_handler_s {
  char *method;
  char *path;
  struct redhttp_response_s *(*func) (struct redhttp_request_s * request, void *user_data);
  void *user_data;
  struct redhttp_handler_s *next;
};

struct redhttp_negotiate_s {
  char *type;
  unsigned char q;
  struct redhttp_negotiate_s *next;
};

struct redhttp_server_s {
  int sockets[FD_SETSIZE];
  int socket_count;
  int socket_max;

  int backlog_size;
  char *signature;

  struct redhttp_handler_s *handlers;
};

static inline char* redhttp_strndup(const char* str1, size_t str1_len)
{
  char* str2 = NULL;

  if (str1) {
    str2 = malloc(str1_len + 1);
    if (str2) {
      strncpy(str2, str1, str1_len);
      str2[str1_len] = '\0';
    }
  }

  return str2;
}

static inline char* redhttp_strdup(const char* str1)
{
  char* str2 = NULL;

  if (str1) {
    size_t str1_len = strlen(str1);
    str2 = redhttp_strndup(str1, str1_len);
  }

  return str2;
}

#endif
