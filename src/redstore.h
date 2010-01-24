#include "config.h"

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <redland.h>
#include "http/redhttpd.h"


#ifndef _REDSTORE_H_
#define _REDSTORE_H_


// ------- Constants -------
#define DEFAULT_PORT            "8080"
#define DEFAULT_STORAGE_NAME    "redstore"
#define DEFAULT_STORAGE_TYPE    "hashes"
#define DEFAULT_STORAGE_OPTIONS "hash-type='bdb',dir='.'"
#define DEFAULT_RESULTS_FORMAT  "application/sparql-results+xml"


// ------- Logging ---------
typedef enum {
    REDSTORE_DEBUG=1,   // Only display debug if verbose
    REDSTORE_INFO,      // Don't show info when quiet
    REDSTORE_ERROR,     // Always display
    REDSTORE_FATAL      // Quit if fatal
} RedstoreLogLevel;


#define redstore_debug( ... ) \
		redstore_log(REDSTORE_DEBUG, __VA_ARGS__ )

#define redstore_info( ... ) \
		redstore_log(REDSTORE_INFO, __VA_ARGS__ )

#define redstore_error( ... ) \
		redstore_log( REDSTORE_ERROR, __VA_ARGS__ )

#define redstore_fatal( ... ) \
		redstore_log( REDSTORE_FATAL, __VA_ARGS__ )



// ------- Structures -------

// FIXME: this should be got from Redland
typedef struct serialiser_info {
    const char *name;
    const char *label;
    const char *mime_type;
    const char *uri;
} serialiser_info_t;


// ------- Globals ---------
extern int quiet;
extern int verbose;
extern int running;
extern int query_count;
extern int request_count;
extern const char* storage_name;
extern const char* storage_type;
extern librdf_world* world;
extern librdf_storage* storage;
extern librdf_model* model;
extern serialiser_info_t serialiser_info[];


// ------- Prototypes -------

http_response_t* handle_sparql_query(http_request_t *request, void* user_data);
http_response_t* handle_page_home(http_request_t *request, void* user_data);
http_response_t* handle_page_query(http_request_t *request, void* user_data);
http_response_t* handle_page_info(http_request_t *request, void* user_data);
http_response_t* handle_page_formats(http_request_t *request, void* user_data);

http_response_t* redstore_error_page(int level, int code, const char* message);
void page_append_html_header(http_response_t *response, const char* title);
void page_append_html_footer(http_response_t *response);

http_response_t* handle_graph_index(http_request_t *request, void* user_data);
http_response_t* handle_graph_head(http_request_t *request, void* user_data);
http_response_t* handle_graph_get(http_request_t *request, void* user_data);
http_response_t* handle_graph_put(http_request_t *request, void* user_data);
http_response_t* handle_graph_delete(http_request_t *request, void* user_data);

http_response_t* load_stream_into_graph(librdf_stream *stream, librdf_uri *graph_uri);
http_response_t* handle_load_get(http_request_t *request, void* user_data);
http_response_t* handle_load_post(http_request_t *request, void* user_data);

http_response_t* format_bindings_query_result_librdf(http_request_t *request, librdf_query_results* results, const char* format_str);
http_response_t* format_bindings_query_result_html(http_request_t *request, librdf_query_results* results, const char* format_str);
http_response_t* format_bindings_query_result_text(http_request_t *request, librdf_query_results* results, const char* format_str);
http_response_t* format_bindings_query_result(http_request_t *request, librdf_query_results* results);

http_response_t* format_graph_stream_librdf(http_request_t *request, librdf_stream* stream, const char* format_str);
http_response_t* format_graph_stream_html(http_request_t *request, librdf_stream* stream, const char* format_str);
http_response_t* format_graph_stream_text(http_request_t *request, librdf_stream* stream, const char* format_str);
http_response_t* format_graph_stream(http_request_t *request, librdf_stream* stream);

http_response_t* handle_image_favicon(http_request_t *request, void* user_data);

void redstore_log( RedstoreLogLevel level, const char* fmt, ... );



#endif
