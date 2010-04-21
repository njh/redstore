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

#include "redstore_config.h"

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

#include <redland.h>
#include "redhttp/redhttp.h"


#ifndef _REDSTORE_H_
#define _REDSTORE_H_


// ------- Constants -------
#define DEFAULT_PORT            "8080"
#define DEFAULT_ADDRESS         (NULL)
#define DEFAULT_STORAGE_NAME    "redstore"
#define DEFAULT_STORAGE_TYPE    "memory"
#define DEFAULT_STORAGE_OPTIONS ""
#define DEFAULT_QUERY_LANGUAGE  "laqrs"
#define DEFAULT_GRAPH_FORMAT    "rdfxml"
#define DEFAULT_PARSE_FORMAT    "ntriples"
#define DEFAULT_RESULTS_FORMAT  "xml"


// ------- Logging ---------
typedef enum {
    REDSTORE_DEBUG = 1,         // Only display debug if verbose
    REDSTORE_INFO,              // Don't show info when quiet
    REDSTORE_ERROR,             // Always display
    REDSTORE_FATAL              // Quit if fatal
} RedstoreLogLevel;


#define redstore_debug( ... ) \
		redstore_log(REDSTORE_DEBUG, __VA_ARGS__ )

#define redstore_info( ... ) \
		redstore_log(REDSTORE_INFO, __VA_ARGS__ )

#define redstore_error( ... ) \
		redstore_log( REDSTORE_ERROR, __VA_ARGS__ )

#define redstore_fatal( ... ) \
		redstore_log( REDSTORE_FATAL, __VA_ARGS__ )



// ------- Globals ---------
extern int quiet;
extern int verbose;
extern int running;
extern unsigned long query_count;
extern unsigned long import_count;
extern unsigned long request_count;
extern const char *storage_name;
extern const char *storage_type;
extern librdf_world *world;
extern librdf_storage *storage;
extern librdf_model *model;

extern librdf_storage *sd_storage;
extern librdf_model *sd_model;
extern librdf_node *service_node;
extern librdf_uri *saddle_ns_uri;
extern librdf_uri *sd_ns_uri;
extern char *accepted_serialiser_types;
extern char *accepted_query_result_types;


// ------- Callbacks ---------

typedef redhttp_response_t *(*redstore_stream_processor)(redhttp_request_t * request, librdf_stream * stream, librdf_node * graph);


// ------- Prototypes -------

int description_init(void);
redhttp_response_t *handle_description_get(redhttp_request_t * request, void *user_data);
int description_update(void);
void description_free(void);

redhttp_response_t *handle_page_home(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_page_query_form(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_page_update_form(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_page_load_form(redhttp_request_t * request, void *user_data);

redhttp_response_t *handle_query(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_sparql(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_page_robots_txt(redhttp_request_t * request, void *user_data);

redhttp_response_t *redstore_error_page(int level, int code, const char *message);
redhttp_response_t *redstore_page_new(const char *title);
int redstore_page_append_string(redhttp_response_t * response, const char *str);
int redstore_page_append_decimal(redhttp_response_t * response, int decimal);
int redstore_page_append_strings(redhttp_response_t * response, ...);
int redstore_page_append_escaped(redhttp_response_t * response, const char *str, char quote);
void redstore_page_end(redhttp_response_t * response);

void page_append_html_header(redhttp_response_t * response, const char *title);
void page_append_html_footer(redhttp_response_t * response);

redhttp_response_t *handle_graph_index(redhttp_request_t * request, void *user_data);

redhttp_response_t *handle_data_get(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_delete(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_post(redhttp_request_t * request, void *user_data);

redhttp_response_t *handle_data_context_head(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_context_get(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_context_put(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_context_post(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_data_context_delete(redhttp_request_t * request, void *user_data);


redhttp_response_t *load_stream_into_graph(redhttp_request_t * request, librdf_stream * stream,
                                           librdf_node * graph);
redhttp_response_t *clear_and_load_stream_into_graph(redhttp_request_t * request, librdf_stream * stream, librdf_node * graph);
redhttp_response_t *parse_data_from_buffer(redhttp_request_t * request, unsigned char* buffer, size_t content_length, const char *parser_name, const char* graph_uri_str, redstore_stream_processor stream_proc);
redhttp_response_t *parse_data_from_request_body(redhttp_request_t * request, const char* graph_uri_str, redstore_stream_processor stream_proc);
redhttp_response_t *handle_load_post(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_insert_post(redhttp_request_t * request, void *user_data);
redhttp_response_t *handle_delete_post(redhttp_request_t * request, void *user_data);

redhttp_response_t *format_bindings_query_result_librdf(redhttp_request_t *
                                                        request,
                                                        librdf_query_results *
                                                        results, const char *format_str);
redhttp_response_t *format_bindings_query_result_html(redhttp_request_t *
                                                      request,
                                                      librdf_query_results *
                                                      results, const char *format_str);
redhttp_response_t *format_bindings_query_result_text(redhttp_request_t *
                                                      request,
                                                      librdf_query_results *
                                                      results, const char *format_str);
redhttp_response_t *format_bindings_query_result(redhttp_request_t * request,
                                                 librdf_query_results * results);

redhttp_response_t *format_graph_stream_librdf(redhttp_request_t * request,
                                               librdf_stream * stream, const char *format_str);
redhttp_response_t *format_graph_stream_html(redhttp_request_t * request,
                                             librdf_stream * stream, const char *format_str);
redhttp_response_t *format_graph_stream_nquads(redhttp_request_t * request,
                                               librdf_stream * stream, const char *format_str);
redhttp_response_t *format_graph_stream(redhttp_request_t * request, librdf_stream * stream);

redhttp_response_t *handle_image_favicon(redhttp_request_t * request, void *user_data);

void redstore_log(RedstoreLogLevel level, const char *fmt, ...);

char *redstore_get_format(redhttp_request_t * request, const char *supported);
int redstore_is_html_format(const char *str);
int redstore_is_text_format(const char *str);
int redstore_is_nquads_format(const char *str);




#ifndef RAPTOR_V2_AVAILABLE

#define raptor_iostream_write_bytes(ptr, size, nmemb, iostr) raptor_iostream_write_bytes(iostr, ptr, size, nmemb)
#define raptor_iostream_write_byte(byte, iostr) raptor_iostream_write_byte(iostr, byte)
#define raptor_iostream_counted_string_write(string, len, iostr) raptor_iostream_write_counted_string(iostr, string, len)
#define raptor_iostream_string_write(string, iostr) raptor_iostream_write_string(iostr, string)

#define raptor_string_ntriples_write(str, len, delim, iostr) raptor_iostream_write_string_ntriples(iostr, str, len, delim)

#endif                          /* !RAPTOR_V2_AVAILABLE */


#endif
