/*
    RedStore - a lightweight RDF triplestore powered by Redland
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

#ifdef __FreeBSD__
#include <sys/socket.h>
#endif

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

static redhttp_response_t *reset_error_buffer(redhttp_request_t * request, void *user_data)
{
  if (error_buffer) {
    raptor_free_stringbuffer(error_buffer);
    error_buffer = NULL;
  }

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
      response = redhttp_response_new_redirect(tmp, REDHTTP_MOVED_PERMANENTLY);
      free(tmp);
    }
  }

  return response;
}

static int redland_log_handler(void *user, librdf_log_message * log_msg)
{
  int level = librdf_log_message_level(log_msg);
  const char *message = librdf_log_message_message(log_msg);
  raptor_locator* locator = librdf_log_message_locator(log_msg);

  if (message) {
    redstore_log(level, message);
  }

  if (!error_buffer) {
    error_buffer = raptor_new_stringbuffer();
    if (!error_buffer) {
      redstore_error("raptor_new_stringbuffer returned NULL");
      return 1;
    }
  }

  if (level == LIBRDF_LOG_ERROR) {
    raptor_stringbuffer_append_string(error_buffer, (unsigned char*)message, 1);
    if (locator) {
      int byte = raptor_locator_byte(locator);
      int line = raptor_locator_line(locator);
      int column = raptor_locator_column(locator);

      if (byte > 0) {
        raptor_stringbuffer_append_string(error_buffer, (unsigned char*)", byte ", 1);
        raptor_stringbuffer_append_decimal(error_buffer, byte);
      }

      if (line > 0) {
        raptor_stringbuffer_append_string(error_buffer, (unsigned char*)", line ", 1);
        raptor_stringbuffer_append_decimal(error_buffer, line);
      }

      if (column > 0) {
        raptor_stringbuffer_append_string(error_buffer, (unsigned char*)", column ", 1);
        raptor_stringbuffer_append_decimal(error_buffer, column);
      }
    }
    raptor_stringbuffer_append_string(error_buffer, (unsigned char*)"\n", 1);
  }

  return 1;
}

// Custom 404 handler
static redhttp_response_t *handle_not_found(redhttp_request_t * request, void *user_data)
{
  return redstore_page_new_with_message(
    request, LIBRDF_LOG_DEBUG, REDHTTP_NOT_FOUND,
    "Unsupported path."
  );
}

static int redstore_load_input_file(librdf_model* model, const char* filename, const char* format)
{
  librdf_uri *uri = NULL;
  int result = 0;

  if (filename) {
    uri = librdf_new_uri_from_filename(world, filename);
    if (!uri) {
      redstore_error("Failed to convert filename into a URI");
      return -1;
    }

    redstore_info("Loading: %s", (char*)librdf_uri_as_string(uri));
    redstore_debug("Input format: %s", format);
    result = librdf_model_load(model, uri, format, NULL, NULL);
  }

  if (uri)
    librdf_free_uri(uri);

  return result;
}

static librdf_storage *redstore_setup_storage(const char *name, const char *type,
                                              const char *options, int new)
{
  librdf_storage *storage = NULL;
  librdf_hash *hash = NULL;
  const char *key_filter[] = { "password", NULL };

  // Create hash
  hash = librdf_new_hash_from_string(world, NULL, options);
  if (!hash) {
    redstore_error("Failed to create storage options hash");
    return NULL;
  }

  librdf_hash_put_strings(hash, "contexts", "yes");
  librdf_hash_put_strings(hash, "write", "yes");
  if (new)
    librdf_hash_put_strings(hash, "new", "yes");

  redstore_info("Storage name: %s", name);
  redstore_info("Storage type: %s", type);

  public_storage_options = librdf_hash_to_string(hash, key_filter);
  if (public_storage_options) {
    redstore_info("Storage options: %s", public_storage_options);
  } else {
    redstore_warn("Failed to convert storage options hash into a string.");
  }

  storage = librdf_new_storage_with_options(world, type, name, hash);
  if (!storage) {
    redstore_error("Failed to open %s storage '%s'", type, name);
  }

  if (hash)
    librdf_free_hash(hash);

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
  redhttp_server_add_handler(server, NULL, NULL, reset_error_buffer, NULL);
  redhttp_server_add_handler(server, "GET", "/query", handle_query, NULL);
  redhttp_server_add_handler(server, "GET", "/sparql", handle_sparql, NULL);
  redhttp_server_add_handler(server, "GET", "/sparql/", handle_sparql, NULL);
  redhttp_server_add_handler(server, "POST", "/query", handle_query, NULL);
  redhttp_server_add_handler(server, "POST", "/sparql", handle_sparql, NULL);
  redhttp_server_add_handler(server, "POST", "/sparql/", handle_sparql, NULL);
  redhttp_server_add_handler(server, "HEAD", "/data*", handle_data_head, NULL);
  redhttp_server_add_handler(server, "GET", "/data*", handle_data_get, NULL);
  redhttp_server_add_handler(server, "PUT", "/data*", handle_data_put, NULL);
  redhttp_server_add_handler(server, "POST", "/data*", handle_data_post, NULL);
  redhttp_server_add_handler(server, "DELETE", "/data*", handle_data_delete, NULL);
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
  redhttp_server_add_handler(server, NULL, NULL, handle_not_found, NULL);

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
  printf("   -s <type>       Set the graph storage type (default %s)\n", DEFAULT_STORAGE_TYPE);
  for (i = 0; 1; i++) {
    const char *help_name, *help_label;
    if (librdf_storage_enumerate(world, i, &help_name, &help_label))
      break;
    printf("      %-12s   %s\n", help_name, help_label);
  }
  printf("   -t <options>    Storage options\n");
  printf("   -n              Create a new store / replace old (default no)\n");
  printf("   -f <filename>   Input file to load at startup\n");
  printf("   -F <format>     Format of the input file (default guess)\n");
  for (i = 0; 1; i++) {
    const raptor_syntax_description* desc = librdf_parser_get_description(world, i);
    if (!desc)
      break;
    printf("      %-12s   %s\n", desc->names[0], desc->label);
  }
  printf("   -v              Enable verbose mode\n");
  printf("   -q              Enable quiet mode\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  redhttp_server_t *server = NULL;
  char *address = DEFAULT_ADDRESS;
  char *port = DEFAULT_PORT;
  const char *storage_options = NULL;
  const char *input_filename = NULL;
  const char *input_format = NULL;
  int storage_new = 0;
  int opt = -1;

  // Make STDOUT unbuffered - we use it for logging
  setbuf(stdout, NULL);

  // Initialise Redland
  world = librdf_new_world();
  if (!world) {
    redstore_fatal("Failed to initialise librdf world");
    // Exit straight away - nothing to clean up
    return -1;
  }
  librdf_world_open(world);
  librdf_world_set_logger(world, NULL, redland_log_handler);

  // Parse Switches
  while ((opt = getopt(argc, argv, "p:b:s:t:nf:F:vqh")) != -1) {
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
    case 'f':
      input_filename = optarg;
      break;
    case 'F':
      input_format = optarg;
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

  if (!verbose) {
    rasqal_world* rasqal = librdf_world_get_rasqal(world);
    // Lower warning level to get only more serious warnings
    rasqal_world_set_warning_level(rasqal, 25);
  }

  // Set default storage settings, if none given
  if (storage_name == NULL)
    storage_name = DEFAULT_STORAGE_NAME;
  if (storage_type == NULL) {
    storage_type = DEFAULT_STORAGE_TYPE;
    storage_options = DEFAULT_STORAGE_OPTIONS;
  }

  // Setup signal handlers
  signal(SIGTERM, termination_handler);
  signal(SIGINT, termination_handler);
  signal(SIGHUP, termination_handler);

  // Create HTTP server
  server = redstore_setup_http_server();
  if (!server) {
    redstore_fatal("Failed to initialise HTTP server.\n");
    goto cleanup;
  }

  // Create storage
  storage = redstore_setup_storage(storage_name, storage_type, storage_options, storage_new);
  if (!storage) {
    redstore_fatal("Failed to open librdf storage.");
    goto cleanup;
  }
  // Create model object
  model = librdf_new_model(world, storage, NULL);
  if (!model) {
    redstore_fatal("Failed to create librdf model for storage.");
    goto cleanup;
  }
  // Load startup input file
  if (redstore_load_input_file(model, input_filename, input_format)) {
    redstore_fatal("Failed to load input file.");
    goto cleanup;
  }
  // Create service description
  if (description_init()) {
    redstore_fatal("Failed to initialise Service Description.");
    goto cleanup;
  }
  // Start listening for connections
  redstore_info("Starting HTTP server on port %s", port);
  if (redhttp_server_listen(server, address, port, PF_UNSPEC)) {
    redstore_fatal("Failed to create HTTP server socket.");
    goto cleanup;
  }

  while (running) {
    redhttp_server_run(server);
  }


cleanup:
  description_free();

  // Free up memory used by the error buffer
  reset_error_buffer(NULL, NULL);

  // Clean up librdf
  if (model)
    librdf_free_model(model);
  if (storage)
    librdf_free_storage(storage);
  if (world)
    librdf_free_world(world);

  // Clean up redhttp
  if (public_storage_options)
    free(public_storage_options);
  if (server)
    redhttp_server_free(server);

  return exit_code;
}
