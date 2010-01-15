#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "redhttpd.h"


#define DEFAULT_PORT      "8080"

int running = 1;


static void
print_help(char *pname)
{
    fprintf(stderr, "%s [option]\n"
        " -f [46]: specify family\n"
        " -a : specify bind address\n"
        " -p : specify port (default 9999)\n"
        " -h: help\n",
        pname);
}


static http_response_t *handle_homepage(http_request_t *request, void *user_data)
{
    const char* page =
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
        "<input type=\"submit\"></form></div>"
        "</body></html>";

    return http_response_new_with_content(page, strlen(page), "text/html");
}

static http_response_t *handle_query(http_request_t *request, void *user_data)
{
    http_response_t* response = http_response_new(200, NULL);
    FILE *socket = http_request_get_socket(request);

    http_response_add_header(response, "Content-Type", "text/html");
    http_request_send_response(request, response);
    
    fprintf(socket, "<html><body><h1>Query Page</h1>");

    fprintf(socket, "<pre>\n");
    fprintf(socket, "Method: %s\n", request->method);
    fprintf(socket, "URL: %s\n", request->url);
    fprintf(socket, "Path: %s\n", request->path);
    fprintf(socket, "Query: %s\n", request->query_string);
    fprintf(socket, "</pre>\n");
   
   	if (request->headers) {
		fprintf(socket, "<pre><b>Request Headers</b>\n");
		http_headers_send(&request->headers, socket);
		fprintf(socket, "</pre>\n");
   	}
   
   	if (response->headers) {
		fprintf(socket, "<pre><b>Response Headers</b>\n");
		http_headers_send(&response->headers, socket);
		fprintf(socket, "</pre>\n");
   	}
   	
   	if (request->arguments) {
		fprintf(socket, "<pre><b>Arguments</b>\n");
		http_headers_send(&request->arguments, socket);
		fprintf(socket, "</pre>\n");
	}
   	
   	if (request->content_buffer) {
		fprintf(socket, "<pre><b>Content</b>\n");
		fwrite(request->content_buffer, 1, request->content_length, socket);
		fprintf(socket, "</pre>\n");
	}
	
    fprintf(socket, "</body></html>");
    
    return response;
}


static http_response_t *handle_redirect(http_request_t *request, void *user_data)
{
    return http_response_new_redirect("/query");
}




int
main(int argc, char **argv)
{
    sa_family_t sopt_family = PF_UNSPEC;       // PF_UNSPEC, PF_INET, PF_INET6
    char        *sopt_host = "";               // nodename for getaddrinfo(3)
    char        *sopt_service = DEFAULT_PORT;  // service name: "pop", "110"
    http_server_t* server;
    int c;


    while ((c = getopt(argc, argv, "f:a:p:h")) != EOF) {
        switch (c) {
            case 'f':
                if (!strncmp("4", optarg,1)) {
                    sopt_family = PF_INET;
                } else if (!strncmp("6", optarg,1)) {
                    sopt_family = PF_INET6;
                } else {
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                }
            break;
            case 'a':
                sopt_host = strdup(optarg);
            break;
            case 'p':
                sopt_service = strdup(optarg);
            break;
            case 'h':
            default:
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            break;
        }
    }


    server = http_server_new();
    if (!server) {
        fprintf(stderr, "Failed to initialise HTTP server.\n");
        exit(EXIT_FAILURE);
    }
  
    http_server_add_handler(server, "GET", "/", handle_homepage, NULL);
    http_server_add_handler(server, "GET", "/query", handle_query, NULL);
    http_server_add_handler(server, "POST", "/query", handle_query, NULL);
    http_server_add_handler(server, "GET", "/redirect", handle_redirect, NULL);
    http_server_set_signature(server, "test_redhttpd/0.1");

    if (http_server_listen(server, sopt_host, sopt_service, sopt_family)) {
        fprintf(stderr, "Failed to create HTTP socket.\n");
        exit(EXIT_FAILURE);
    }

    while(running) {
        http_server_run(server);
    }

    http_server_free(server);

    return 0;
}
