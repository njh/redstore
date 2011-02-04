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
#include <getopt.h>

#include "redhttp.h"


#define DEFAULT_PORT      "8080"

int running = 1;


static void print_help(char *pname)
{
  fprintf(stderr, "%s [option]\n"
          " -f [46]: specify family\n"
          " -a : specify bind address\n"
          " -p : specify port (default 9999)\n" " -h: help\n", pname);
}


static redhttp_response_t *handle_homepage(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/html");
  const char page[] =
      "<html><head><title>Homepage</title></head>"
      "<body><h1>Homepage</h1>"
      "<p>This is the homepage.</p>"
      "<div><h2>GET</h2><form action=\"/query\" method=\"get\">"
      "Enter a URL: <input name=\"url\" size=\"25\" value=\"http://www.example.com/\"/><br />"
      "Enter a Title: <input name=\"title\" size=\"25\" value=\"Example Website\"/><br />"
      "<input type=\"submit\"></form></div>"
      "<div><h2>POST</h2><form action=\"/query\" method=\"post\">"
      "Enter a URL: <input name=\"url\" size=\"25\" value=\"http://www.example.com/\"/><br />"
      "Enter a Title: <input name=\"title\" size=\"25\" value=\"Example Website\"/><br />"
      "<input type=\"submit\"></form></div></body></html>\n";

  redhttp_response_copy_content(response, page, sizeof(page));

  return response;
}

static redhttp_response_t *handle_query(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/html");
  FILE *socket = redhttp_request_get_socket(request);
  const char *accept_header;

  redhttp_response_send(response, request);

  fprintf(socket, "<html><body><h1>Query Page</h1>");

  fprintf(socket, "<pre>\n");
  fprintf(socket, "Method: %s\n", redhttp_request_get_method(request));
  fprintf(socket, "URL: %s\n", redhttp_request_get_url(request));
  fprintf(socket, "Path: %s\n", redhttp_request_get_path(request));
  fprintf(socket, "Path Glob: %s\n", redhttp_request_get_path_glob(request));
  fprintf(socket, "Query: %s\n", redhttp_request_get_query_string(request));
  fprintf(socket, "Server Name: %s\n", redhttp_request_get_server_name(request));
  fprintf(socket, "Server Address: %s\n", redhttp_request_get_server_addr(request));
  fprintf(socket, "Server Port: %s\n", redhttp_request_get_server_port(request));
  fprintf(socket, "Remote Address: %s\n", redhttp_request_get_remote_addr(request));
  fprintf(socket, "Remote Port: %s\n", redhttp_request_get_remote_port(request));
  fprintf(socket, "</pre>\n");

  if (redhttp_request_count_headers(request)) {
    fprintf(socket, "<pre><b>Request Headers</b>\n");
    redhttp_request_print_headers(request, socket);
    fprintf(socket, "</pre>\n");
  }

  accept_header = redhttp_request_get_header(request, "Accept");
  if (accept_header) {
    redhttp_negotiate_t *first = redhttp_negotiate_parse(accept_header);
    const char *type;
    int i, q;
    fprintf(socket, "<pre><b>Accept Headers</b>\n");
    for (i = 0; 1; i++) {
      if (redhttp_negotiate_get(&first, i, &type, &q))
        break;
      fprintf(socket, "%1.1f  %s\n", (float) q / 10, type);
    }
    fprintf(socket, "</pre>\n");
    redhttp_negotiate_free(&first);
  }

  if (redhttp_response_count_headers(response)) {
    fprintf(socket, "<pre><b>Response Headers</b>\n");
    redhttp_response_print_headers(response, socket);
    fprintf(socket, "</pre>\n");
  }

  if (redhttp_request_count_arguments(request)) {
    fprintf(socket, "<pre><b>Arguments</b>\n");
    redhttp_request_print_arguments(request, socket);
    fprintf(socket, "</pre>\n");
  }

  if (redhttp_request_get_content_buffer(request)) {
    char *buf = redhttp_request_get_content_buffer(request);
    size_t len = redhttp_request_get_content_length(request);
    fprintf(socket, "<pre><b>Content</b>\n");
    fwrite(buf, 1, len, socket);
    fprintf(socket, "</pre>\n");
  }

  fprintf(socket, "</body></html>");

  return response;
}


static redhttp_response_t *handle_redirect(redhttp_request_t * request, void *user_data)
{
  return redhttp_response_new_redirect("/query");
}

static redhttp_response_t *handle_logging(redhttp_request_t * request, void *user_data)
{
  printf("[%s:%s] %s: %s\n",
         redhttp_request_get_remote_addr(request),
         redhttp_request_get_remote_port(request),
         redhttp_request_get_method(request), redhttp_request_get_path(request)
      );
  return NULL;
}



int main(int argc, char **argv)
{
  sa_family_t sopt_family = PF_UNSPEC;  // PF_UNSPEC, PF_INET, PF_INET6
  char *sopt_host = NULL;       // nodename for getaddrinfo(3)
  char *sopt_service = DEFAULT_PORT;  // service name: "pop", "110"
  redhttp_server_t *server;
  int c;


  while ((c = getopt(argc, argv, "f:a:p:h")) != EOF) {
    switch (c) {
    case 'f':
      if (!strncmp("4", optarg, 1)) {
        sopt_family = PF_INET;
      } else if (!strncmp("6", optarg, 1)) {
        sopt_family = PF_INET6;
      } else {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
      }
      break;
    case 'a':
      sopt_host = optarg;
      break;
    case 'p':
      sopt_service = optarg;
      break;
    case 'h':
    default:
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    }
  }


  server = redhttp_server_new();
  if (!server) {
    fprintf(stderr, "Failed to initialise HTTP server.\n");
    exit(EXIT_FAILURE);
  }

  redhttp_server_add_handler(server, NULL, NULL, handle_logging, NULL);
  redhttp_server_add_handler(server, "GET", "/", handle_homepage, NULL);
  redhttp_server_add_handler(server, "GET", "/query*", handle_query, NULL);
  redhttp_server_add_handler(server, "POST", "/query", handle_query, NULL);
  redhttp_server_add_handler(server, "GET", "/redirect", handle_redirect, NULL);
  redhttp_server_set_signature(server, "test_redhttpd/0.1");

  if (redhttp_server_listen(server, sopt_host, sopt_service, sopt_family)) {
    fprintf(stderr, "Failed to create HTTP socket.\n");
    exit(EXIT_FAILURE);
  }

  printf("Listening on: %s\n", sopt_service);

  while (running) {
    redhttp_server_run(server);
  }

  redhttp_server_free(server);

  return 0;
}
