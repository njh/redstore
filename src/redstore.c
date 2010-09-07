/*
    RedStore - a lightweight RDF triplestore powered by Redland
    Copyright (C) 2010 Nicholas J Humfrey <njh@aelius.com>

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
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "redstore.h"


// ------- Globals -------
int quiet = 0;                  // Only display error messages
int verbose = 0;                // Increase number of logging messages
int running = 1;                // True while still running
unsigned long query_count = 0;
unsigned long import_count = 0;
unsigned long request_count = 0;
const char *storage_name = DEFAULT_STORAGE_NAME;
const char *storage_type = DEFAULT_STORAGE_TYPE;
librdf_world *world = NULL;
librdf_model *model = NULL;
librdf_storage *storage = NULL;

redhttp_negotiate_t *accepted_serialiser_types = NULL;
redhttp_negotiate_t *accepted_query_result_types = NULL;



static void termination_handler(int signum)
{
  switch (signum) {
  case SIGHUP:
    redstore_info("Got hangup signal.");
    break;
  case SIGTERM:
    redstore_info("Got termination signal.");
    break;
  case SIGINT:
    redstore_info("Got interupt signal.");
    break;
  }

  signal(signum, termination_handler);

  // Signal the main thead to stop
  running = 0;
}


void redstore_log(RedstoreLogLevel level, const char *fmt, ...)
{
  time_t t = time(NULL);
  char *time_str;
  va_list args;

  // Display the message level
  if (level == REDSTORE_DEBUG) {
    if (!verbose)
      return;
    printf("[DEBUG]  ");
  } else if (level == REDSTORE_INFO) {
    if (quiet)
      return;
    printf("[INFO]   ");
  } else if (level == REDSTORE_ERROR) {
    printf("[ERROR]  ");
  } else if (level == REDSTORE_FATAL) {
    printf("[FATAL]  ");
  } else {
    printf("[UNKNOWN]");
  }

  // Display timestamp
  time_str = ctime(&t);
  time_str[strlen(time_str) - 1] = 0; // remove \n
  printf("%s  ", time_str);

  // Display the error message
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);

  // If fatal then stop
  if (level == REDSTORE_FATAL) {
    if (running) {
      // Quit gracefully
      running = 0;
    } else {
      printf("Fatal error while quiting; exiting immediately.");
      exit(-1);
    }
  }
}

static redhttp_response_t *request_counter(redhttp_request_t * request, void *user_data)
{
  request_count++;
  return NULL;
}

static redhttp_response_t *request_log(redhttp_request_t * request, void *user_data)
{
  redstore_info("%s - %s %s",
                redhttp_request_get_remote_addr(request),
                redhttp_request_get_method(request), redhttp_request_get_path(request));
  return NULL;
}

static redhttp_response_t *remove_trailing_slash(redhttp_request_t * request, void *user_data)
{
  const char *path = redhttp_request_get_path(request);
  size_t path_len = strlen(path);
  redhttp_response_t *response = NULL;

  if (path[path_len - 1] == '/') {
    char *tmp = calloc(1, path_len);
    if (tmp) {
      strcpy(tmp, path);
      tmp[path_len - 1] = '\0';
      response = redhttp_response_new_redirect(tmp);
      free(tmp);
    }
  }

  return response;
}

static int redland_log_handler(void *user, librdf_log_message * msg)
{
  int level = librdf_log_message_level(msg);
  int code = librdf_log_message_code(msg);
  const char *message = librdf_log_message_message(msg);

  printf("redland_log_handler: code=%d level=%d message=%s\n", code, level, message);
  return 0;
}

static void redstore_build_accepted_type_list()
{
  raptor_world *raptor = librdf_world_get_raptor(world);
  unsigned int i, m;

  // FIXME: This should use the librdf API
  for (i = 0; 1; i++) {
    const raptor_syntax_description *desc = NULL;

    desc = raptor_world_get_serializer_description(raptor, i);
    if (!desc)
      break;

    for (m = 0; m < desc->mime_types_count; m++) {
      redhttp_negotiate_add(&accepted_serialiser_types,
                            desc->mime_types[m].mime_type,
                            desc->mime_types[m].mime_type_len, desc->mime_types[m].q);
    }
  }


  for (i = 0; 1; i++) {
    const char *mime_type;
    if (librdf_query_results_formats_enumerate(world, i, NULL, NULL, NULL, &mime_type))
      break;

    redhttp_negotiate_add(&accepted_query_result_types, mime_type, strlen(mime_type), 10);
  }
}


char *redstore_get_format(redhttp_request_t * request, redhttp_negotiate_t * supported)
{
  const char *format_arg;
  char *format_str = NULL;

  format_arg = redhttp_request_get_argument(request, "format");
  if (format_arg) {
    format_str = calloc(1, strlen(format_arg) + 1);
    strcpy(format_str, format_arg);
    redstore_debug("format_arg: %s", format_str);
  } else if (!format_str) {
    const char *accept_str = redhttp_request_get_header(request, "Accept");
    redhttp_negotiate_t *accept = redhttp_negotiate_parse(accept_str);
    format_str = redhttp_negotiate_choose(&supported, &accept);
    redstore_debug("accept: %s", accept_str);
    // FIXME: display list of supported formats
    redstore_debug("supported: %d", redhttp_negotiate_count(&supported));
    redstore_debug("chosen: %s", format_str);
    redhttp_negotiate_free(&accept);
  }

  return format_str;
}

int redstore_is_html_format(const char *str)
{
  if (strcmp(str, "html") == 0 ||
      strcmp(str, "text/html") == 0 || strcmp(str, "application/xhtml+xml") == 0)
    return 1;
  else
    return 0;
}

int redstore_is_text_format(const char *str)
{
  if (strcmp(str, "text") == 0 || strcmp(str, "text/plain") == 0)
    return 1;
  else
    return 0;
}

int redstore_is_nquads_format(const char *str)
{
  if (strcmp(str, "nquads") == 0 || strcmp(str, "application/x-nquads") == 0)
    return 1;
  else
    return 0;
}

static librdf_storage *redstore_setup_storage(const char *name, const char *type,
                                              const char *options, int new)
{
  librdf_storage *storage = NULL;
  librdf_hash *hash = NULL;
  const char *key_filter[] = { "password", NULL };
  char *debug_str = NULL;

  // Create hash
  hash = librdf_new_hash_from_string(world, NULL, options);
  if (!hash) {
    redstore_fatal("Failed to create storage options hash");
    return NULL;
  }

  librdf_hash_put_strings(hash, "contexts", "yes");
  librdf_hash_put_strings(hash, "write", "yes");
  if (new)
    librdf_hash_put_strings(hash, "new", "yes");

  redstore_info("Storage name: %s", name);
  redstore_info("Storage type: %s", type);

  debug_str = librdf_hash_to_string(hash, key_filter);
  if (debug_str) {
    redstore_info("Storage options: %s", debug_str);
  }

  storage = librdf_new_storage_with_options(world, type, name, hash);
  if (!storage) {
    redstore_fatal("Failed to open %s storage '%s'", type, name);
  }

  if (hash)
    librdf_free_hash(hash);
  if (debug_str)
    free(debug_str);

  return storage;
}

static redhttp_server_t *redstore_setup_http_server()
{
  redhttp_server_t *server = NULL;

  server = redhttp_server_new();
  if (!server)
    return server;

  // Configure routing
  redhttp_server_add_handler(server, NULL, NULL, request_counter, &request_count);
  redhttp_server_add_handler(server, NULL, NULL, request_log, NULL);
  redhttp_server_add_handler(server, "GET", "/query", handle_query, NULL);
  redhttp_server_add_handler(server, "GET", "/sparql", handle_sparql, NULL);
  redhttp_server_add_handler(server, "GET", "/sparql/", handle_sparql, NULL);
  redhttp_server_add_handler(server, "POST", "/query", handle_query, NULL);
  redhttp_server_add_handler(server, "POST", "/sparql", handle_sparql, NULL);
  redhttp_server_add_handler(server, "POST", "/sparql/", handle_sparql, NULL);
  redhttp_server_add_handler(server, "HEAD", "/data/*", handle_data_context_head, NULL);
  redhttp_server_add_handler(server, "GET", "/data/*", handle_data_context_get, NULL);
  redhttp_server_add_handler(server, "PUT", "/data/*", handle_data_context_put, NULL);
  redhttp_server_add_handler(server, "POST", "/data/*", handle_data_context_post, NULL);
  redhttp_server_add_handler(server, "DELETE", "/data/*", handle_data_context_delete, NULL);
  redhttp_server_add_handler(server, "GET", "/data", handle_data_get, NULL);
  redhttp_server_add_handler(server, "POST", "/data", handle_data_post, NULL);
  redhttp_server_add_handler(server, "DELETE", "/data", handle_data_delete, NULL);
  redhttp_server_add_handler(server, "GET", "/insert", handle_page_update_form, "Insert Triples");
  redhttp_server_add_handler(server, "POST", "/insert", handle_insert_post, NULL);
  redhttp_server_add_handler(server, "GET", "/delete", handle_page_update_form, "Delete Triples");
  redhttp_server_add_handler(server, "POST", "/delete", handle_delete_post, NULL);
  redhttp_server_add_handler(server, "GET", "/graphs", handle_graph_index, NULL);
  redhttp_server_add_handler(server, "GET", "/load", handle_page_load_form, NULL);
  redhttp_server_add_handler(server, "POST", "/load", handle_load_post, NULL);
  redhttp_server_add_handler(server, "GET", "/", handle_page_home, NULL);
  redhttp_server_add_handler(server, "GET", "/description", handle_description_get, NULL);
  redhttp_server_add_handler(server, "GET", "/favicon.ico", handle_image_favicon, NULL);
  redhttp_server_add_handler(server, "GET", "/robots.txt", handle_page_robots_txt, NULL);
  redhttp_server_add_handler(server, "GET", NULL, remove_trailing_slash, NULL);

  // Set the server signature
  redhttp_server_set_signature(server, PACKAGE_NAME "/" PACKAGE_VERSION);

  return server;
}

// Display how to use this program
static void usage()
{
  int i;
  printf("%s version %s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
  printf("Usage: %s [options] [<name>]\n", PACKAGE_TARNAME);
  printf("   -p <port>       Port number to run HTTP server on (default %s)\n", DEFAULT_PORT);
  printf("   -b <address>    Bind to specific address (default all)\n");
  printf("   -s <type>       Set the graph storage type\n");
  for (i = 0; 1; i++) {
    const char *help_name, *help_label;
    if (librdf_storage_enumerate(world, i, &help_name, &help_label))
      break;
    printf("      %-10s     %s", help_name, help_label);
    if (strcmp(help_name, DEFAULT_STORAGE_TYPE) == 0)
      printf(" (default)\n");
    else
      printf("\n");
  }
  printf("   -t <options>    Storage options\n");
  printf("   -n              Create a new store / replace old (default no)\n");
  printf("   -v              Enable verbose mode\n");
  printf("   -q              Enable quiet mode\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  redhttp_server_t *server = NULL;
  char *address = DEFAULT_ADDRESS;
  char *port = DEFAULT_PORT;
  const char *storage_options = DEFAULT_STORAGE_OPTIONS;
  int storage_new = 0;
  int opt = -1;

  // Make STDOUT unbuffered - we use it for logging
  setbuf(stdout, NULL);

  // Initialise Redland
  world = librdf_new_world();
  if (!world) {
    redstore_fatal("Failed to initialise librdf world");
    return -1;
  }
  librdf_world_open(world);
  librdf_world_set_logger(world, NULL, redland_log_handler);

  // Build list of accepted mime types
  redstore_build_accepted_type_list();

  // Parse Switches
  while ((opt = getopt(argc, argv, "p:b:s:t:nvqh")) != -1) {
    switch (opt) {
    case 'p':
      port = optarg;
      break;
    case 'b':
      address = optarg;
      break;
    case 's':
      storage_type = optarg;
      break;
    case 't':
      storage_options = optarg;
      break;
    case 'n':
      storage_new = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      quiet = 1;
      break;
    default:
      usage();
      break;
    }
  }

  // Check remaining arguments
  argc -= optind;
  argv += optind;
  if (argc == 1) {
    storage_name = argv[0];
  } else if (argc > 1) {
    usage();
  }
  // Validate parameters
  if (quiet && verbose) {
    redstore_error("Can't be quiet and verbose at the same time.");
    usage();
  }
  // Setup signal handlers
  signal(SIGTERM, termination_handler);
  signal(SIGINT, termination_handler);
  signal(SIGHUP, termination_handler);

  // Create HTTP server
  server = redstore_setup_http_server();
  if (!server) {
    redstore_fatal("Failed to initialise HTTP server.\n");
    return -1;
  }
  // Create storgae
  storage = redstore_setup_storage(storage_name, storage_type, storage_options, storage_new);
  if (!storage) {
    redstore_fatal("Failed to create storage.");
    return -1;
  }
  // Create model object
  model = librdf_new_model(world, storage, NULL);
  if (!model) {
    redstore_fatal("Failed to create model for storage.");
    return -1;
  }
  // Create service description
  if (description_init()) {
    redstore_fatal("Failed to initialise Service Description.");
    return -1;
  }
  // Start listening for connections
  redstore_info("Starting HTTP server on port %s", port);
  if (redhttp_server_listen(server, address, port, PF_UNSPEC)) {
    redstore_fatal("Failed to create HTTP server socket.");
    return -1;
  }

  while (running) {
    redhttp_server_run(server);
  }

  description_free();

  librdf_free_storage(storage);
  librdf_free_world(world);

  redhttp_negotiate_free(&accepted_serialiser_types);
  redhttp_negotiate_free(&accepted_query_result_types);
  redhttp_server_free(server);

  return 0; // FIXME: should return non-zero if there was a fatal error
}
